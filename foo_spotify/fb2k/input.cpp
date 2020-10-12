#include <stdafx.h>

#include <backend/libspotify_backend.h>
#include <backend/libspotify_wrapper.h>
#include <backend/spotify_instance.h>
#include <backend/spotify_object.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_objects_all.h>
#include <fb2k/config.h>
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
    InputSpotify() = default;
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
    LibSpotify_Backend& GetInitializedLibSpotify();

private:
    bool usingLibSpotify_ = false;
    bool hasDecoder_ = false;

    std::optional<t_input_open_reason> openedReason_;

    wrapper::Ptr<sp_track> track_;
    std::unordered_multimap<std::string, std::string> trackMeta_;

    int channels_{};
    int sampleRate_{};
    int bitRate_{};
};
} // namespace

namespace
{

InputSpotify::~InputSpotify()
{
    try
    {
        Finalize();
    }
    catch ( const std::exception& )
    {
    }
}

void InputSpotify::Finalize()
{
    if ( !usingLibSpotify_ )
    {
        return;
    }
    usingLibSpotify_ = false;

    auto& lsBackend = SpotifyInstance::Get().GetLibSpotify_Backend();
    {
        if ( hasDecoder_ )
        {
            lsBackend.ReleaseDecoder( this );
        }
    }

    if ( track_ )
    {
        lsBackend.ExecSpMutex( [&] {
            track_.Release();
        } );
    }

    lsBackend.UnregisterBackendUser( *this );
}

void InputSpotify::open( service_ptr_t<file> m_file, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort )
{
    openedReason_ = p_reason;

    if ( p_reason == input_open_info_write )
    {
        throw exception_io_denied_readonly();
    }

    auto& waBackend = SpotifyInstance::Get().GetWebApi_Backend();

    SpotifyObject so( p_path );
    const auto track = waBackend.GetTrack( so.id, p_abort );
    trackMeta_ = waBackend.GetMetaForTracks( nonstd::span<const std::unique_ptr<const WebApi_Track>>( &track, 1 ) )[0];

    if ( p_reason == input_open_info_read )
    { // don't use LibSpotify stuff if not required
        return;
    }

    auto& lsBackend = GetInitializedLibSpotify();

    auto pSession = lsBackend.GetInitializedSpSession( p_abort );

    lsBackend.ExecSpMutex( [&] {
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
        // TODO: clean up here
        const auto done = lsBackend.ExecSpMutex( [&] {
            const auto sp = sp_track_error( track_ );
            if ( SP_ERROR_OK == sp )
            {
                return true;
            }
            else if ( SP_ERROR_IS_LOADING == sp )
            {
                return false;
            }
            else
            {
                throw qwr::QwrException( fmt::format( "sp_track_error failed: {}", sp_error_message( sp ) ) );
            }
        } );

        if ( done )
        {
            break;
        }

        p_abort.sleep( 0.05 );
    }

    bitRate_ = [] {
        switch ( config::preferred_bitrate )
        {
        case config::BitrateSettings::Bitrate96k:
        {
            return 96;
        }
        case config::BitrateSettings::Bitrate160k:
        {
            return 160;
        }
        case config::BitrateSettings::Bitrate320k:
        {
            return 320;
        }
        default:
        {
            assert( false );
            return 320;
        }
        }
    }();
}

void InputSpotify::get_info( t_int32 subsong, file_info& p_info, abort_callback& p_abort )
{
    if ( subsong )
    {
        throw std::exception( "This track does not have any sub-songs" );
    }

    sptf::fb2k::FillFileInfoWithMeta( trackMeta_, p_info );
    if ( openedReason_ == input_open_decode )
    { // Use exact length when possible
        auto& lsBackend = GetInitializedLibSpotify();
        lsBackend.ExecSpMutex( [&] {
            p_info.set_length( sp_track_duration( track_ ) / 1000.0 );
        } );
    }
}

foobar2000_io::t_filestats InputSpotify::get_file_stats( abort_callback& p_abort )
{
    return t_filestats{};
}

void InputSpotify::decode_initialize( t_int32 subsong, unsigned p_flags, abort_callback& p_abort )
{
    if ( subsong )
    {
        throw std::exception( "This track does not have any sub-songs" );
    }

    if ( !( p_flags & input_flag_playback ) )
    {
        throw exception_io_denied();
    }

    auto& lsBackend = GetInitializedLibSpotify();
    lsBackend.AcquireDecoder( this );
    hasDecoder_ = true;

    lsBackend.GetAudioBuffer().clear();
    auto pSession = lsBackend.GetInitializedSpSession( p_abort );

    lsBackend.ExecSpMutex( [&] {
        const auto sp = sp_session_player_load( pSession, track_ );
        if ( sp != SP_ERROR_OK )
        {
            throw qwr::QwrException( fmt::format( "sp_session_player_load failed: {}", sp_error_message( sp ) ) );
        }

        sp_session_player_play( pSession, true );
    } );
}

bool InputSpotify::decode_run( audio_chunk& p_chunk, abort_callback& p_abort )
{
    bool isEof = false;
    const auto dataReader = [&]( const AudioBuffer::AudioChunkHeader& header,
                                 const uint16_t* data ) {
        if ( header.eof )
        {
            isEof = true;
            return;
        }
        channels_ = header.channels;
        sampleRate_ = header.sampleRate;
        p_chunk.set_data_fixedpoint( data,
                                     header.size * sizeof( uint16_t ),
                                     header.sampleRate,
                                     header.channels,
                                     16,
                                     audio_chunk::channel_config_stereo );
    };

    auto& lsBackend = GetInitializedLibSpotify();
    auto& buf = lsBackend.GetAudioBuffer();

    if ( !buf.read( dataReader ) )
    {
        buf.wait_for_data( p_abort );
        if ( !buf.read( dataReader ) )
        { // wait was aborted, ending playback
            isEof = true;
        }
    }

    if ( isEof )
    {
        lsBackend.ReleaseDecoder( this );
        hasDecoder_ = false;
        return false;
    }

    return true;
}

void InputSpotify::decode_seek( double p_seconds, abort_callback& p_abort )
{
    auto& lsBackend = GetInitializedLibSpotify();
    lsBackend.GetAudioBuffer().clear(); // TODO: remove?
    auto pSession = lsBackend.GetInitializedSpSession( p_abort );
    lsBackend.ExecSpMutex( [&] {
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
    p_out.info_set_bitrate( bitRate_ );

    return true;
}

bool InputSpotify::decode_get_dynamic_info_track( file_info& p_out, double& p_timestamp_delta )
{
    return false;
}

void InputSpotify::decode_on_idle( abort_callback& p_abort )
{
    (void)p_abort;
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

LibSpotify_Backend& InputSpotify::GetInitializedLibSpotify()
{
    auto& lsBackend = SpotifyInstance::Get().GetLibSpotify_Backend();
    if ( !usingLibSpotify_ )
    {
        lsBackend.RegisterBackendUser( *this );
        usingLibSpotify_ = true;
    }
    return lsBackend;
}

} // namespace

namespace
{

input_factory_t<InputSpotify> g_input;

} // namespace
