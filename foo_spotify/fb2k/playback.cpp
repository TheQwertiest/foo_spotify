#include <stdafx.h>

#include "playback.h"

#include <backend/libspotify_backend.h>

namespace sptf::fb2k
{

std::mutex PlayCallbacks::mutex_;
LibSpotify_Backend* PlayCallbacks::pLsBackend_ = nullptr;

PlayCallbacks::PlayCallbacks()
{
}

PlayCallbacks::~PlayCallbacks()
{
}

void PlayCallbacks::Initialize( LibSpotify_Backend& lsBackend )
{
    std::lock_guard lg( mutex_ );
    pLsBackend_ = &lsBackend;
}

void PlayCallbacks::Finalize()
{
    std::lock_guard lg( mutex_ );
    pLsBackend_ = nullptr;
}

unsigned PlayCallbacks::get_flags()
{
    return ( flag_on_playback_stop | flag_on_playback_pause );
}

void PlayCallbacks::on_playback_pause( bool isPaused )
{
    std::lock_guard lg( mutex_ );

    if ( !pLsBackend_ )
    {
        return;
    }

    auto pSession = pLsBackend_->GetWhateverSpSession();
    pLsBackend_->ExecSpMutex( [&] {
        sp_session_player_play( pSession, !isPaused );
    } );
}

void PlayCallbacks::on_playback_stop( play_control::t_stop_reason reason )
{
    if ( play_control::stop_reason_starting_another == reason )
    {
        return;
    }

    std::lock_guard lg( mutex_ );

    if ( !pLsBackend_ )
    {
        return;
    }

    auto pSession = pLsBackend_->GetWhateverSpSession();
    pLsBackend_->ExecSpMutex( [&] {
        sp_session_player_unload( pSession );
    } );
}

} // namespace sptf::fb2k

namespace
{

play_callback_static_factory_t<sptf::fb2k::PlayCallbacks> g_play_callback_static;

}
