#pragma once

#include <mutex>

namespace sptf
{

class LibSpotify_Backend;

}

namespace sptf::fb2k
{

class PlayCallbacks
    : public play_callback_static
{
public:
    PlayCallbacks();
    ~PlayCallbacks();

    static void Initialize( LibSpotify_Backend& lsBackend );
    static void Finalize();

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
    static std::mutex mutex_;
    static LibSpotify_Backend* pLsBackend_;
};

} // namespace sptf::fb2k
