#include <stdafx.h>

#include <backend/libspotify_backend.h>
#include <backend/libspotify_wrapper.h>
#include <backend/spotify_object.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_objects_all.h>
#include <fb2k/file_info_filler.h>

#include <qwr/string_helpers.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <vector>

using namespace sptf;

namespace
{

// input_impl::input_impl
class InputSpotify
    : public input_stubs
    , public LibSpotify_BackendUser
{
public:
    InputSpotify();
    ~InputSpotify();

    // SpotifyBackendUser
    void Finalize() override;

    // input_impl::open
    void open( service_ptr_t<file> m_file, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort );

    // input_info_reader::get_info
    void get_info( t_int32 subsong, file_info& p_info, abort_callback& p_abort );

    // input_info_reader::get_file_stats
    t_filestats get_file_stats( abort_callback& p_abort );

    // input_decoder::initialize
    void decode_initialize( t_int32 subsong, unsigned p_flags, abort_callback& p_abort );

    // input_decoder::run
    bool decode_run( audio_chunk& p_chunk, abort_callback& p_abort );

    // input_decoder::seek
    void decode_seek( double p_seconds, abort_callback& p_abort );

    // input_decoder::can_seek
    bool decode_can_seek();

    // input_decoder::get_dynamic_info
    bool decode_get_dynamic_info( file_info& p_out, double& p_timestamp_delta );

    // input_decoder::get_dynamic_info_track
    bool decode_get_dynamic_info_track( file_info& p_out, double& p_timestamp_delta );

    // input_decoder::on_idle
    void decode_on_idle( abort_callback& p_abort );

    // input_impl::retag_set_info
    void retag_set_info( t_int32 subsong, const file_info& p_info, abort_callback& p_abort );

    // input_impl::retag_commit
    void retag_commit( abort_callback& p_abort );

    // input_info_reader::get_subsong_count
    t_uint32 get_subsong_count();

    // input_info_reader::get_subsong
    t_uint32 get_subsong( unsigned song );

    // input_entry::is_our_content_type
    static bool g_is_our_content_type( const char* p_content_type );

    // input_entry::is_our_path
    static bool g_is_our_path( const char* p_full_path, const char* p_extension );

    // input_entry::get_guid
    static GUID g_get_guid();

    // input_entry::get_name
    static const char* g_get_name();

private:
    LibSpotify_Backend& lsBackend_;
    WebApi_Backend& waBackend_;

    std::mutex statusMutex_;
    std::atomic_bool isInitialized_{ true };

    bool hasDecoder_ = false;

    wrapper::Ptr<sp_track> track_;
    std::unordered_multimap<std::string, std::string> trackMeta_;

    t_filestats stats_;
    int channels_;
    int sampleRate_;
};
} // namespace

namespace
{

// TODO: reduce amount and duration of locks
// TODO: delay backend loading until it's actually needed
InputSpotify::InputSpotify()
    : lsBackend_( LibSpotify_Backend::Instance() )
    , waBackend_( WebApi_Backend::Instance() )
{
    lsBackend_.RegisterBackendUser( *this );
}

InputSpotify::~InputSpotify()
{
    Finalize();
    lsBackend_.UnregisterBackendUser( *this );
}

void InputSpotify::Finalize()
{
    {
        std::lock_guard lk( statusMutex_ );
        if ( hasDecoder_ )
        {
            lsBackend_.ReleaseDecoder( this );
        }
        isInitialized_ = false;
    }

    lsBackend_.ExecSpMutex( [&] {
        track_.Release();
    } );
}

void InputSpotify::open( service_ptr_t<file> m_file, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort )
{
    std::lock_guard lk( statusMutex_ );
    if ( !isInitialized_ )
    {
        // TODO: azaza
        throw std::exception( "azaza" );
    }

    // TODO: handle `input_open_info_read`, i.e. with decode disabled
    // TODO: add proper path handling: handle both url and uri, convert it to some struct

    if ( p_reason == input_open_info_write )
    {
        throw exception_io_denied_readonly();
    }

    auto pSession = lsBackend_.GetInitializedSpSession( p_abort );

    SpotifyObject so( p_path );
    const auto track = waBackend_.GetTrack( so.id, p_abort );
    trackMeta_ = waBackend_.GetMetaForTracks( nonstd::span<const std::unique_ptr<const WebApi_Track>>( &track, 1 ) )[0];

    lsBackend_.ExecSpMutex( [&] {
        wrapper::Ptr<sp_link> link( sp_link_create_from_string( so.ToUri().c_str() ) );
        if ( !link )
        {
            throw exception_io_data( "Couldn't parse url" );
        }

        track_.Release();

        switch ( sp_link_type( link ) )
        {
        case SP_LINKTYPE_TRACK:
        {
            track_ = sp_link_as_track( link );
            break;
        }
        default:
        {
            throw exception_io_data( "Only track links should be passed to input" );
        }
        }
    } );

    while ( true )
    {
        const auto done = lsBackend_.ExecSpMutex( [&] {
            const sp_error e = sp_track_error( track_ );
            if ( SP_ERROR_OK == e )
            {
                return true;
            }
            else
            {
                if ( SP_ERROR_IS_LOADING != e )
                    assertSucceeds( "Preloading track", e );
                return false;
            }
        } );

        if ( done )
        {
            break;
        }

        p_abort.sleep( 0.05 );
    }
}

void InputSpotify::get_info( t_int32 subsong, file_info& p_info, abort_callback& p_abort )
{
    std::lock_guard lk( statusMutex_ );
    if ( !isInitialized_ || subsong )
    {
        // TODO: azaza
        throw std::exception( "azaza" );
    }

    lsBackend_.ExecSpMutex( [&] {
        p_info.set_length( sp_track_duration( track_ ) / 1000.0 );
    } );

    //FillFileInfoWithMeta( trackMeta_, p_info );
}

foobar2000_io::t_filestats InputSpotify::get_file_stats( abort_callback& p_abort )
{
    return stats_;
}

void InputSpotify::decode_initialize( t_int32 subsong, unsigned p_flags, abort_callback& p_abort )
{
    std::lock_guard lk( statusMutex_ );
    if ( !isInitialized_ || subsong )
    {
        // TODO: azaza
        throw std::exception( "azaza" );
    }

    if ( ( p_flags & input_flag_playback ) == 0 )
        throw exception_io_denied();

    lsBackend_.AcquireDecoder( this );
    hasDecoder_ = true;

    lsBackend_.GetAudioBuffer().clear();
    auto pSession = lsBackend_.GetInitializedSpSession( p_abort );

    lsBackend_.ExecSpMutex( [&] {
        assertSucceeds( "load track (including region check)", sp_session_player_load( pSession, track_ ) );
        sp_session_player_play( pSession, true );
    } );
}

bool InputSpotify::decode_run( audio_chunk& p_chunk, abort_callback& p_abort )
{
    std::lock_guard lk( statusMutex_ );
    if ( !isInitialized_ )
    {
        // TODO: azaza
        throw std::exception( "azaza" );
    }

    const auto dataReader = [&]( const AudioBuffer::AudioChunkHeader& header,
                                 const uint16_t* data ) {
        channels_ = header.channels;
        sampleRate_ = header.sampleRate;
        p_chunk.set_data_fixedpoint( data,
                                     header.size * sizeof( uint16_t ),
                                     header.sampleRate,
                                     header.channels,
                                     16,
                                     audio_chunk::channel_config_stereo );
    };

    auto& buf = lsBackend_.GetAudioBuffer();

    while ( !buf.read( dataReader ) )
    {
        if ( buf.has_ended() )
        {
            lsBackend_.ReleaseDecoder( this );
            hasDecoder_ = false;
            return false;
        }
        else
        { // TODO: replace with proper cond_var wait
            if ( !buf.has_data() )
            {
                p_abort.sleep( 0.1 );
            }
        }
    }

    return true;
}

void InputSpotify::decode_seek( double p_seconds, abort_callback& p_abort )
{
    std::lock_guard lk( statusMutex_ );
    if ( !isInitialized_ )
    {
        // TODO: azaza
        throw std::exception( "azaza" );
    }

    lsBackend_.GetAudioBuffer().clear(); // TODO: remove?
    auto pSession = lsBackend_.GetInitializedSpSession( p_abort );
    lsBackend_.ExecSpMutex( [&] {
        sp_session_player_seek( pSession, static_cast<int>( p_seconds * 1000 ) );
    } );
}

bool InputSpotify::decode_can_seek()
{
    return true;
}

bool InputSpotify::decode_get_dynamic_info( file_info& p_out, double& p_timestamp_delta )
{
    p_out.info_set_int( "CHANNELS", channels_ );
    p_out.info_set_int( "SAMPLERATE", sampleRate_ );
    return true;
}

bool InputSpotify::decode_get_dynamic_info_track( file_info& p_out, double& p_timestamp_delta )
{
    return false;
}

void InputSpotify::decode_on_idle( abort_callback& p_abort )
{
}

void InputSpotify::retag_set_info( t_int32 subsong, const file_info& p_info, abort_callback& p_abort )
{
    throw exception_io_data();
}

void InputSpotify::retag_commit( abort_callback& p_abort )
{
    throw exception_io_data();
}

bool InputSpotify::g_is_our_content_type( const char* p_content_type )
{
    return false;
}

bool InputSpotify::g_is_our_path( const char* p_full_path, const char* p_extension )
{ // accept only filtered input
    return std::string_view( p_full_path )._Starts_with( "spotify:track:" );
}

t_uint32 InputSpotify::get_subsong_count()
{
    return 1;
}

t_uint32 InputSpotify::get_subsong( unsigned song )
{
    return song;
}

GUID InputSpotify::g_get_guid()
{
    return guid::input;
}

const char* InputSpotify::g_get_name()
{
    return SPTF_NAME ": decoder";
}

} // namespace

namespace
{

input_factory_t<InputSpotify> g_input;

} // namespace
