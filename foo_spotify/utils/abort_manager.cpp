#include <stdafx.h>

#include "abort_manager.h"

#include <qwr/thread_helpers.h>

namespace sptf
{

AbortManager::AbortManager()
{
    StartThread();
}

void AbortManager::Finalize()
{
    StopThread();
}

void AbortManager::RemoveTask( uint32_t taskId )
{
    std::unique_lock lock( mutex_ );

    const auto itTask = idToTask_.find( taskId );
    assert( itTask != idToTask_.cend() );
    const auto handle = itTask->second.pAbort;
    idToTask_.erase( itTask );

    const auto itIds = abortToIds_.find( handle );
    assert( itIds != abortToIds_.cend() );
    auto& ids = itIds->second;

    const auto itId = std::find( ids.rbegin(), ids.rend(), taskId );
    assert( itId != ids.rend() );

    ids.erase( std::next( itId ).base() );
    if ( ids.empty() )
    {
        abortToIds_.erase( itIds );
    }
}

void AbortManager::StartThread()
{
    pThread_ = std::make_unique<std::thread>( &AbortManager::EventLoop, this );
    qwr::SetThreadName( *pThread_, "SPTF Abort Handler" );
}

void AbortManager::StopThread()
{
    if ( !pThread_ )
    {
        return;
    }

    {
        std::unique_lock lock( mutex_ );
        isTimeToDie_ = true;
    }
    eventCv_.notify_all();

    if ( pThread_->joinable() )
    {
        pThread_->join();
    }

    pThread_.reset();
}

void AbortManager::EventLoop()
{
    while ( true )
    {
        std::unique_lock lock( mutex_ );
        eventCv_.wait_for( lock, std::chrono::seconds( 2 ), [&] { return ( isTimeToDie_ || !abortToIds_.empty() ); } );

        if ( isTimeToDie_ )
        {
            for ( auto& [id, task]: idToTask_ )
            {
                std::invoke( *( task.task ) );
            }
            return;
        }

        for ( const auto [pAbort, ids]: abortToIds_ )
        {
            if ( pAbort->is_aborting() )
            {
                for ( const auto id: ranges::views::reverse( ids ) )
                {
                    assert( idToTask_.count( id ) );
                    std::invoke( *( idToTask_[id].task ) );
                }
            }
        }
    }
}

AbortManager::AbortableScope::AbortableScope( size_t taskId, AbortManager& parent )
    : parent_( parent )
    , taskId_( taskId )
{
}

AbortManager::AbortableScope::AbortableScope( AbortableScope&& other )
    : parent_( other.parent_ )
    , isValid_( other.isValid_ )
    , taskId_( other.taskId_ )
{
    other.isValid_ = false;
}

AbortManager::AbortableScope::~AbortableScope()
{
    if ( isValid_ )
    {
        parent_.RemoveTask( taskId_ );
    }
}

} // namespace sptf
