
#include <stdafx.h>

#include "rps_limiter.h"

#include <backend/spotify_instance.h>
#include <utils/abort_manager.h>
#include <utils/sleeper.h>

#include <qwr/final_action.h>

namespace
{

std::chrono::milliseconds GetTimestampInMs()
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() ).time_since_epoch();
}

} // namespace

namespace sptf
{

RpsLimiter::RpsLimiter( size_t limitPerSecond )
    : timeStampsContainer_( limitPerSecond, GetTimestampInMs() - std::chrono::seconds( 2 ) )
    , timeStamps_( timeStampsContainer_.begin(), timeStampsContainer_.end(), timeStampsContainer_.begin(), timeStampsContainer_.size() )
{
}

void RpsLimiter::WaitForRequestAvailability( abort_callback& abort )
{
    if ( abort.is_aborting() )
    {
        return;
    }

    auto& am = SpotifyInstance::Get().GetAbortManager();
    const auto abortableScope = am.GetAbortableScope( [&] {
        {
            std::lock_guard lock( mutex_ );
            timeToDie_ = true;
        }
        cv_.notify_all();
    },
                                                      abort );

    std::unique_lock lock( mutex_ );

    const auto nowInMs = GetTimestampInMs();
    auto timeDiff = nowInMs - timeStamps_.front();
    if ( timeDiff > std::chrono::seconds( 1 ) )
    {
        timeStamps_.emplace_back( nowInMs );
        return;
    }

    const auto requestIdx = curRequestIdx_++;
    incomingRequests_.emplace_back( requestIdx );
    const auto curIt = std::prev( incomingRequests_.end() );
    const qwr::final_action autoEraseRequest( [&] {
        incomingRequests_.erase( curIt );
    } );

    while ( true )
    {
        FB2K_console_formatter() << SPTF_UNDERSCORE_NAME " (debug):\n"
                                 << fmt::format( "throttling for {} milliseconds", timeDiff.count() );

        bool hasEvent = cv_.wait_for( lock, timeDiff, [&] {
            return timeToDie_ || ( incomingRequests_.front() == requestIdx );
        } );

        if ( hasEvent )
        {
            return;
        }

        timeDiff = GetTimestampInMs() - timeStamps_.front();
    }
}

} // namespace sptf
