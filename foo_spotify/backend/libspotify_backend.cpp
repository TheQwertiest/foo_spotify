#include <stdafx.h>

#include "libspotify_backend.h"

#include <backend/libspotify_key.h>
#include <backend/spotify_instance.h>
#include <fb2k/advanced_config.h>
#include <ui/ui_not_auth.h>
#include <utils/abort_manager.h>
#include <utils/cred_prompt.h>

#include <component_paths.h>

#include <libspotify/api.h>
#include <qwr/abort_callback.h>
#include <qwr/error_popup.h>
#include <qwr/fb2k_adv_config.h>
#include <qwr/thread_helpers.h>

#include <filesystem>
#include <fstream>
#include <tuple>

// see https://github.com/mopidy/mopidy-spotify for tips and stuff

namespace fs = std::filesystem;

using namespace sptf;

namespace sptf
{

LibSpotify_Backend::LibSpotify_Backend( AbortManager& abortManager )
    : abortManager_( abortManager )
    , audioBuffer_( abortManager )
{
    if ( const auto settingsPath = path::LibSpotifySettings(); !fs::exists( settingsPath ) )
    {
        fs::create_directories( settingsPath );
    }
    // TODO: add fs error/exception checks

    const auto cachePath = path::LibSpotifyCache().u8string();
    const auto settingsPath = path::LibSpotifySettings().u8string();

    config_.api_version = SPOTIFY_API_VERSION,
    config_.cache_location = cachePath.c_str();
    config_.settings_location = settingsPath.c_str();
    config_.application_key = g_appkey;
    config_.application_key_size = sizeof( g_appkey );
    config_.user_agent = "foobar2000-foo_spotify-" SPTF_VERSION;
    config_.userdata = this;
    config_.callbacks = &callbacks_;

    const auto proxyUrl = qwr::fb2k::config::GetValue( sptf::config::advanced::network_proxy );
    const auto proxyUsername = qwr::fb2k::config::GetValue( sptf::config::advanced::network_proxy_username );
    const auto proxyPassword = qwr::fb2k::config::GetValue( sptf::config::advanced::network_proxy_password );
    if ( !proxyUrl.empty() )
    {
        config_.proxy = proxyUrl.c_str();
        if ( !proxyUsername.empty() && !proxyPassword.empty() )
        {
            config_.proxy_username = proxyUsername.c_str();
            config_.proxy_password = proxyPassword.c_str();
        }
    }

    /*
    config_.tracefile = ...;
    */

#define SPTF_ASSIGN_CALLBACK( callbacks, name )                                                      \
    callbacks.name = []( sp_session * pSession, auto... args ) -> auto                               \
    { /** sp_session_userdata is assumed to be thread safe. */                                       \
        return static_cast<LibSpotify_Backend*>( sp_session_userdata( pSession ) )->name( args... ); \
    }

    // dummy callbacks are needed to avoid libspotify crashes on sp_session_release
#define SPTF_ASSIGN_DUMMY_CALLBACK( callbacks, name ) \
    callbacks.name = []( auto... ) {}

    SPTF_ASSIGN_CALLBACK( callbacks_, logged_in );
    SPTF_ASSIGN_DUMMY_CALLBACK( callbacks_, logged_out );
    /*
    SPTF_ASSIGN_DUMMY_CALLBACK( callbacks_, connection_error )
    */
    SPTF_ASSIGN_CALLBACK( callbacks_, message_to_user );
    SPTF_ASSIGN_CALLBACK( callbacks_, notify_main_thread );
    SPTF_ASSIGN_CALLBACK( callbacks_, music_delivery );
    SPTF_ASSIGN_CALLBACK( callbacks_, play_token_lost );
    SPTF_ASSIGN_CALLBACK( callbacks_, log_message );
    SPTF_ASSIGN_CALLBACK( callbacks_, end_of_track );
    /*
    SPTF_ASSIGN_DUMMY_CALLBACK( callbacks_, streaming_error )
    SPTF_ASSIGN_DUMMY_CALLBACK( callbacks_, start_playback )
    SPTF_ASSIGN_DUMMY_CALLBACK( callbacks_, stop_playback )
    SPTF_ASSIGN_DUMMY_CALLBACK( callbacks_, get_audio_buffer_stats )
    */
    SPTF_ASSIGN_DUMMY_CALLBACK( callbacks_, offline_status_updated );
    SPTF_ASSIGN_DUMMY_CALLBACK( callbacks_, offline_error );
    SPTF_ASSIGN_DUMMY_CALLBACK( callbacks_, credentials_blob_updated );
    SPTF_ASSIGN_CALLBACK( callbacks_, connectionstate_updated );

#undef SPTF_ASSIGN_DUMMY_CALLBACK
#undef SPTF_ASSIGN_CALLBACK

    // TODO: check if `sp_playlist_add_callbacks` works when implementing playlist handling

    {
        std::lock_guard lock( apiMutex_ );
        const auto sp = sp_session_create( &config_, &pSpSession_ );
        if ( sp != SP_ERROR_OK )
        {
            throw qwr::QwrException( fmt::format( "sp_session_create failed: {}", sp_error_message( sp ) ) );
        }
    }

    StartEventLoopThread();
}

void LibSpotify_Backend::Finalize()
{
    StopEventLoopThread();

    {
        std::lock_guard lk( backendUsersMutex_ );

        for ( auto& pInput: backendUsers_ )
        {
            pInput->Finalize();
        }
    }

    {
        std::lock_guard lock( apiMutex_ );
        sp_session_player_unload( pSpSession_ );
        sp_session_release( pSpSession_ );
    }
}

void LibSpotify_Backend::RegisterBackendUser( LibSpotify_BackendUser& input )
{
    std::lock_guard lk( backendUsersMutex_ );

    assert( !backendUsers_.count( &input ) );
    backendUsers_.emplace( &input );
}

void LibSpotify_Backend::UnregisterBackendUser( LibSpotify_BackendUser& input )
{
    std::lock_guard lk( backendUsersMutex_ );

    assert( backendUsers_.count( &input ) );
    backendUsers_.erase( &input );
}

sp_session* LibSpotify_Backend::GetInitializedSpSession( abort_callback& p_abort )
{
    assert( pSpSession_ );
    if ( !Relogin( p_abort ) )
    {
        ::fb2k::inMainThread( [&] {
            playback_control::get()->stop();
            ui::NotAuthHandler::Get().ShowDialog();
        } );
        throw qwr::QwrException( "Failed to get authenticated Spotify session" );
    }

    return pSpSession_;
}

sp_session* LibSpotify_Backend::GetWhateverSpSession()
{
    assert( pSpSession_ );
    return pSpSession_;
}

bool LibSpotify_Backend::Relogin( abort_callback& abort )
{
    {
        std::lock_guard lock( loginMutex_ );

        if ( loginStatus_ == LoginStatus::logged_out )
        {
            return false;
        }

        if ( loginStatus_ == LoginStatus::uninitialized )
        {
            loginStatus_ = LoginStatus::login_in_process;

            const auto spRet = [&] {
                std::lock_guard lock( apiMutex_ );
                return sp_session_relogin( pSpSession_ );
            }();
            if ( spRet == SP_ERROR_NO_CREDENTIALS )
            {
                loginStatus_ = LoginStatus::logged_out;
                return false;
            }
        }
    }

    auto retStatus = WaitForLoginStatusUpdate( abort );
    return ( retStatus.has_value() ? *retStatus : false );
}

bool LibSpotify_Backend::LoginWithUI( HWND hWnd )
{
    // TODO: add abortable as a method argument
    assert( core_api::assert_main_thread() );

    {
        std::lock_guard lock( loginMutex_ );
        assert( loginStatus_ == LoginStatus::logged_out );
        loginStatus_ = LoginStatus::login_in_process;
        isLoginBad_ = false;
    }

    std::optional<bool> retStatus;
    do
    {
        const auto wasLoginBad = [&] {
            std::lock_guard lock( loginMutex_ );
            return isLoginBad_;
        }();

        auto cpr = ShowCredentialsDialog( hWnd, wasLoginBad ? sp_error_message( SP_ERROR_BAD_USERNAME_OR_PASSWORD ) : nullptr );
        if ( cpr->cancelled )
        {
            {
                std::lock_guard lock( loginMutex_ );
                loginStatus_ = LoginStatus::logged_out;
            }
            loginCv_.notify_all();
            return false;
        }

        {
            std::lock_guard lock( apiMutex_ );
            sp_session_login( pSpSession_, cpr->un.data(), cpr->pw.data(), true, nullptr );
        }

        qwr::TimedAbortCallback tac( fmt::format( "{}: {}", SPTF_UNDERSCORE_NAME, "LibSpotify wait for login update" ) );
        retStatus = WaitForLoginStatusUpdate( tac );
    } while ( retStatus.has_value() && !*retStatus );

    return ( retStatus.has_value() ? *retStatus : false );
}

void LibSpotify_Backend::LogoutAndForget( abort_callback& abort )
{
    {
        std::lock_guard lgLogin( loginMutex_ );
        if ( loginStatus_ != LoginStatus::logged_in )
        {
            return;
        }

        loginStatus_ = LoginStatus::logout_in_process;
    }

    {
        std::lock_guard lgApi( apiMutex_ );
        sp_session_logout( pSpSession_ );
        sp_session_forget_me( pSpSession_ );
    }

    WaitForLoginStatusUpdate( abort );
}

std::optional<bool> LibSpotify_Backend::WaitForLoginStatusUpdate( abort_callback& abort )
{
    const auto abortableScope = abortManager_.GetAbortableScope( [&] { loginCv_.notify_all(); }, abort );

    std::unique_lock lock( loginMutex_ );

    loginCv_.wait( lock, [&] {
        return ( ( loginStatus_ != LoginStatus::login_in_process
                   && loginStatus_ != LoginStatus::logout_in_process )
                 || abort.is_aborting() );
    } );
    if ( abort.is_aborting() )
    {
        return std::nullopt;
    }

    return ( loginStatus_ == LoginStatus::logged_in );
}

std::string LibSpotify_Backend::GetLoggedInUserName()
{
    {
        std::lock_guard lock( loginMutex_ );
        if ( loginStatus_ != LoginStatus::logged_in )
        {
            return "<error: user is not logged in>";
        }
    }

    std::lock_guard lock( apiMutex_ );
    // TODO: add sp_user_display_name
    const char* userName = sp_session_user_name( pSpSession_ );
    if ( !userName )
    {
        return "<error: user name could not be fetched>";
    }

    return userName;
}

void LibSpotify_Backend::EventLoopThread()
{
    int nextTimeout = INFINITE;
    while ( true )
    {
        {
            std::unique_lock lock( workerMutex_ );

            while ( !hasEvents_ && !shouldStopEventLoop_ )
            {
                const auto ret = eventLoopCv_.wait_for( lock, std::chrono::milliseconds( nextTimeout ) );
                if ( std::cv_status::timeout == ret )
                {
                    break;
                }
            }

            if ( shouldStopEventLoop_ )
            {
                return;
            }
            hasEvents_ = false;
        }

        std::lock_guard lock( apiMutex_ );
        sp_session_process_events( pSpSession_, &nextTimeout );
    }
}

void LibSpotify_Backend::StartEventLoopThread()
{
    assert( !pWorker_ );
    pWorker_ = std::make_unique<std::thread>( &LibSpotify_Backend::EventLoopThread, this );
    qwr::SetThreadName( *pWorker_, "SPTF Event Loop" );
}

void LibSpotify_Backend::StopEventLoopThread()
{
    if ( !pWorker_ )
    {
        return;
    }

    {
        std::unique_lock<std::mutex> lock( workerMutex_ );
        shouldStopEventLoop_ = true;
    }
    eventLoopCv_.notify_all();

    if ( pWorker_->joinable() )
    {
        pWorker_->join();
    }
    pWorker_.reset();
}

void LibSpotify_Backend::AcquireDecoder( void* owner )
{
    std::lock_guard lk( decoderOwnerMutex_ );
    if ( pDecoderOwner_ && pDecoderOwner_ != owner )
    {
        throw exception_io_data( "Someone else is already decoding: Spotify does not support multiple concurrent decoders" );
    }

    pDecoderOwner_ = owner;
}

void LibSpotify_Backend::ReleaseDecoder( void* owner )
{
    (void)owner;

    std::lock_guard lk( decoderOwnerMutex_ );
    assert( owner == pDecoderOwner_ );
    pDecoderOwner_ = nullptr;
}

AudioBuffer& LibSpotify_Backend::GetAudioBuffer()
{
    return audioBuffer_;
}

void LibSpotify_Backend::log_message( const char* error )
{
    FB2K_console_formatter() << SPTF_UNDERSCORE_NAME " (log): " << error;
}

void LibSpotify_Backend::logged_in( sp_error error )
{
    if ( SP_ERROR_OK == error )
    {
        return;
    }

    if ( SP_ERROR_BAD_USERNAME_OR_PASSWORD == error )
    {
        std::lock_guard lock( loginMutex_ );
        if ( loginStatus_ == LoginStatus::login_in_process )
        {
            isLoginBad_ = true;
            loginStatus_ = LoginStatus::logged_out;
        }
    }
    else
    {
        {
            std::lock_guard lock( loginMutex_ );
            loginStatus_ = LoginStatus::uninitialized;
        }
        qwr::ReportErrorWithPopup( SPTF_NAME, fmt::format( "Failed to login:\n{}", sp_error_message( error ) ) );
    }
}

void LibSpotify_Backend::message_to_user( const char* message )
{
    qwr::ReportErrorWithPopup( SPTF_NAME, message );
}

void LibSpotify_Backend::notify_main_thread()
{
    {
        std::unique_lock lock( workerMutex_ );
        hasEvents_ = true;
    }

    eventLoopCv_.notify_all();
}

int LibSpotify_Backend::music_delivery( const sp_audioformat* format, const void* frames, int num_frames )
{
    if ( !num_frames )
    {
        audioBuffer_.clear();
        return 0;
    }

    if ( !audioBuffer_.write( AudioBuffer::AudioChunkHeader{ (uint16_t)format->sample_rate,
                                                             (uint16_t)format->channels,
                                                             ( uint16_t )( num_frames * format->channels ) },
                              static_cast<const uint16_t*>( frames ) ) )
    {
        return 0;
    }

    return num_frames;
}

void LibSpotify_Backend::end_of_track()
{
    audioBuffer_.write_end();
}

void LibSpotify_Backend::play_token_lost()
{
    qwr::ReportErrorWithPopup( SPTF_NAME, "Playback has been paused because your Spotify account is being used somewhere else." );
}

void LibSpotify_Backend::connectionstate_updated()
{
    switch ( sp_session_connectionstate( pSpSession_ ) )
    {
    case SP_CONNECTION_STATE_LOGGED_IN:
    case SP_CONNECTION_STATE_OFFLINE:
    {
        {
            std::lock_guard lock( loginMutex_ );
            loginStatus_ = LoginStatus::logged_in;
        }
        loginCv_.notify_all();
        break;
    }
    case SP_CONNECTION_STATE_LOGGED_OUT:
    {
        {
            std::lock_guard lock( loginMutex_ );
            if ( loginStatus_ == LoginStatus::login_in_process )
            { // possible when invalid user/pass are supplied
                break;
            }
            loginStatus_ = LoginStatus::logged_out;
        }
        loginCv_.notify_all();
        break;
    }
    default:
    {
        break;
    }
    }
}

} // namespace sptf
