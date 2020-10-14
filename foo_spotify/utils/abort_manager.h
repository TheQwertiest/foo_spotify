#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>

namespace sptf
{

class AbortManager
{
private:
    using Task = std::function<void()>;

    struct AbortableTask
    {
        std::unique_ptr<Task> task;
        abort_callback* pAbort;
    };

public:
    class AbortableScope
    {
        friend class AbortManager;

    public:
        AbortableScope( AbortableScope&& other );
        ~AbortableScope();

    private:
        AbortableScope( size_t taskId, AbortManager& parent );

    private:
        AbortManager& parent_;
        bool isValid_ = true;
        size_t taskId_;
    };

public:
    AbortManager();
    AbortManager( const AbortManager& other ) = delete;
    AbortManager( AbortManager&& other ) = delete;
    ~AbortManager() = default;

    void Finalize();

    template <typename T>
    AbortableScope GetAbortableScope( T&& task, abort_callback& abort )
    {
        return AbortableScope( AddTask( std::forward<T>( task ), abort ), *this );
    }

private:
    void StartThread();
    void StopThread();

    void EventLoop();

    template <typename T>
    size_t AddTask( T&& task, abort_callback& abort )
    {
        static_assert( std::is_invocable_v<T> );
        static_assert( std::is_move_constructible_v<T> || std::is_copy_constructible_v<T> );

        const auto taskId = [&] {
            std::unique_lock lock( mutex_ );

            const auto taskId = [&] {
                auto id = idCounter_;
                while ( idToTask_.count( id ) )
                {
                    id = ++idCounter_;
                }
                return id;
            }();

            if constexpr ( !std::is_copy_constructible_v<T> && std::is_move_constructible_v<T> )
            {
                auto taskLambda = [taskWrapper = std::make_shared<T>( std::forward<T>( task ) )] {
                    std::invoke( *taskWrapper );
                };
                idToTask_.try_emplace( taskId, AbortableTask{ std::make_unique<Task>( taskLambda ), &abort } );
            }
            else
            {
                idToTask_.try_emplace( taskId, AbortableTask{ std::make_unique<Task>( std::forward<T>( task ) ), &abort } );
            }

            return abortToIds_[&abort].emplace_back( taskId );
        }();

        eventCv_.notify_all();

        return taskId;
    }

    void RemoveTask( uint32_t taskId );

private:
    std::unique_ptr<std::thread> pThread_;

    std::mutex mutex_;
    std::condition_variable eventCv_;
    bool isTimeToDie_ = false;

    size_t idCounter_ = 0;
    std::unordered_map<size_t, AbortableTask> idToTask_;
    std::unordered_map<abort_callback*, std::vector<size_t>> abortToIds_;
};

} // namespace sptf
