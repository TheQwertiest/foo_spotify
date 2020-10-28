
#include <stdafx.h>

#include "rps_limiter.h"

#include <backend/spotify_instance.h>
#include <fb2k/advanced_config.h>
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

RpsLimiter::RpsLimiter( size_t limit, std::chrono::seconds limitPeriod )
    : shouldLogWebApiDebug_( config::advanced::logging_webapi_debug )
    , limitPeriod_( limitPeriod )
    , limitCount_( limit )
    , timeStampsContainer_( limit, GetTimestampInMs() - 2 * limitPeriod )
    , timeStamps_( timeStampsContainer_.begin(), timeStampsContainer_.end(), timeStampsContainer_.begin(), timeStampsContainer_.size() )
{
}

void RpsLimiter::WaitForRequestAvailability( abort_callback& abort )
{
    if ( abort.is_aborting() )
    {
        return;
    }

    std::atomic_bool timeToDie = false;
    auto& am = SpotifyInstance::Get().GetAbortManager();
    const auto abortableScope = am.GetAbortableScope( [&] {
        timeToDie = true;
        cv_.notify_all();
    },
                                                      abort );

    std::unique_lock lock( mutex_ );

    const auto nowInMs = GetTimestampInMs();
    auto nextAvailableTime = timeStamps_.front() + limitPeriod_;
    if ( incomingRequests_.empty() && nowInMs >= nextAvailableTime )
    {
        timeStamps_.emplace_back( nowInMs );
        return;
    }

    auto waitTime = nextAvailableTime - nowInMs;

    const auto requestIdx = curRequestIdx_++;
    incomingRequests_.emplace_back( requestIdx );
    const auto curIt = std::prev( incomingRequests_.end() );
    const qwr::final_action autoEraseRequest( [&] {
        incomingRequests_.erase( curIt );
    } );

    while ( true )
    {
        if ( shouldLogWebApiDebug_ )
        {
            FB2K_console_formatter() << SPTF_UNDERSCORE_NAME " (debug):\n"
                                     << fmt::format( "throttling for {} milliseconds", waitTime.count() );
        }

        bool hasEvent = cv_.wait_for( lock, waitTime, [&] {
            return timeToDie || ( incomingRequests_.front() == requestIdx && GetTimestampInMs() >= nextAvailableTime );
        } );

        if ( hasEvent )
        {
            if ( !timeToDie )
            {
                timeStamps_.emplace_back( GetTimestampInMs() );
            }
            return;
        }

        nextAvailableTime = GetTimestampInMs() + limitPeriod_ / limitCount_;
        waitTime = limitPeriod_ / limitCount_;
    }
}

} // namespace sptf
