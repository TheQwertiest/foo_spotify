#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>

namespace sptf
{

#if 0

class AbortManager
{
private:
    using Task = std::function<void()>;

public:
    static AbortManager& Instance();

    void Finalize();

    template <typename T>
    void AddTask( T&& task )
    {
        static_assert( std::is_invocable_v<T> );
        static_assert( std::is_move_constructible_v<T> || std::is_copy_constructible_v<T> );

        if (size == MAXIMUM_WAIT_OBJECTS)
        {
            throw TOOMUCH;
        }

        if ( canExecute_ )
        {
            std::invoke( task );
        }
        else
        {
            if constexpr ( !std::is_copy_constructible_v<T> && std::is_move_constructible_v<T> )
            {
                auto taskLambda = [taskWrapper = std::make_shared<T>( std::forward<T>( task ) )] {
                    std::invoke( *taskWrapper );
                };
                tasks_.emplace( std::make_unique<Task>( taskLambda ) );
            }
            else
            {
                tasks_.emplace( std::make_unique<Task>( task ) );
            }
        }
    }

private:
    AbortManager(){

    };

    void EventLoop()
    {
        while ( true )
        {
            std::unique_lock lock( mutex_ );
            eventCv_.wait_for( lock, std::chrono::seconds( 1 ), [&] { return ( isTimeToDie_ || !handles_.empty() ); } );

            if (isTimeToDie_)
            {
                //TODO: invoke all
                for (auto& callback : callbacks_)
                {
                    callback();
                }
                callbacks_.clear();
                handles_.clear();
                return;
            }

            auto ret = WaitForMultipleObjects( handles_.size(), handles_.data(), FALSE, 0 );
            switch ( ret )
            {
            case WAIT_TIMEOUT:
            {
                continue;
            }
            case WAIT_FAILED:
            {
                 // TODO: add log
                    hasExploded_ = true;
                return;
            }
            default:
                assert( !( ret >= WAIT_ABANDONED_0 && ret < WAIT_ABANDONED_0 + handles_.size() ) );
                break;
            }

            callbacks_[ret]();
            remove_callback_and_update_idx( ret );
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable eventCv_;
    bool isTimeToDie_ = false;
    std::vector<HANDLE> handles_;
    bool hasExploded_ = false;
};

#endif

} // namespace sptf
