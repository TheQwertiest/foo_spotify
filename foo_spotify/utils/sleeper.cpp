#include <stdafx.h>

#include "sleeper.h"

#include <backend/spotify_instance.h>
#include <utils/abort_manager.h>

#include <condition_variable>
#include <mutex>

namespace sptf
{

bool SleepFor( const std::chrono::milliseconds& duration, abort_callback& abort )
{
    std::mutex mtx;
    std::condition_variable cv;
    bool timeToDie = false;

    auto& am = SpotifyInstance::Get().GetAbortManager();
    auto abortableScope = am.GetAbortableScope( [&] {
        {
            std::lock_guard lock( mtx );
            timeToDie = true;
        }
        cv.notify_all();
    },
                                                abort );

    std::unique_lock lock( mtx );
    return !cv.wait_for( lock, duration, [&] { return timeToDie; } );
}

} // namespace sptf
