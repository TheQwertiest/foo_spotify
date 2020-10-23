#pragma once

#include <nonstd/ring_span.hpp>

#include <chrono>
#include <list>
#include <mutex>
#include <vector>

namespace sptf
{

class RpsLimiter
{
public:
    RpsLimiter( size_t limit );
    ~RpsLimiter() = default;

    void WaitForRequestAvailability( abort_callback& abort );

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool timeToDie_ = false;

    std::vector<std::chrono::milliseconds> timeStampsContainer_;
    nonstd::ring_span<
        std::chrono::milliseconds,
        nonstd::null_popper<std::chrono::milliseconds>>
        timeStamps_;

    size_t curRequestIdx_ = 0;
    // TODO: replace list with queue
    std::list<size_t> incomingRequests_;
};

} // namespace sptf
