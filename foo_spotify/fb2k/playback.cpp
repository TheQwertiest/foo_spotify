#include <stdafx.h>

#include <backend/libspotify_backend.h>

using namespace sptf;

namespace
{

class PlayCallbacksSpotify
    : public play_callback_static
    , public LibSpotifyBackendUser
{
public:
    PlayCallbacksSpotify();
    ~PlayCallbacksSpotify();

    // SpotifyBackendUser
    void Finalize() override;

    // play_callback_static
    unsigned get_flags() override;
    void on_playback_pause( bool isPaused ) override;
    void on_playback_stop( play_control::t_stop_reason reason ) override;

    void on_playback_starting( play_control::t_track_command p_command, bool p_paused ) override{};
    void on_playback_new_track( metadb_handle_ptr p_track ) override{};
    void on_playback_seek( double p_time ) override{};
    void on_playback_edited( metadb_handle_ptr p_track ) override{};
    void on_playback_dynamic_info( const file_info& p_info ) override{};
    void on_playback_dynamic_info_track( const file_info& p_info ) override{};
    void on_playback_time( double p_time ) override{};
    void on_volume_change( float p_new_val ) override{};

private:
    LibSpotifyBackend& spotifyBackend_;

    std::mutex statusMutex_;
    std::atomic_bool isInitialized_{ true };
};

} // namespace

namespace
{

PlayCallbacksSpotify::PlayCallbacksSpotify()
    : spotifyBackend_( LibSpotifyBackend::Instance() )
{
    spotifyBackend_.RegisterBackendUser( *this );
}

PlayCallbacksSpotify::~PlayCallbacksSpotify()
{
    Finalize();
    spotifyBackend_.UnregisterBackendUser( *this );
}

void PlayCallbacksSpotify::Finalize()
{
    std::lock_guard lk( statusMutex_ );
    isInitialized_ = false;
}

unsigned PlayCallbacksSpotify::get_flags()
{
    return flag_on_playback_stop | flag_on_playback_pause;
}

void PlayCallbacksSpotify::on_playback_pause( bool isPaused )
{
    std::lock_guard lk( statusMutex_ );
    if ( !isInitialized_ )
    {
        return;
    }

    auto pSession = spotifyBackend_.GetWhateverSpSession();
    spotifyBackend_.ExecSpMutex( [&] {
        sp_session_player_play( pSession, !isPaused );
    } );
}

void PlayCallbacksSpotify::on_playback_stop( play_control::t_stop_reason reason )
{
    std::lock_guard lk( statusMutex_ );
    if ( !isInitialized_ )
    {
        return;
    }

    if ( play_control::stop_reason_starting_another == reason )
    {
        return;
    }

    auto pSession = spotifyBackend_.GetWhateverSpSession();
    spotifyBackend_.ExecSpMutex( [&] {
        sp_session_player_unload( pSession );
    } );
}

} // namespace

namespace
{

play_callback_static_factory_t<PlayCallbacksSpotify> g_play_callback_static;

}
