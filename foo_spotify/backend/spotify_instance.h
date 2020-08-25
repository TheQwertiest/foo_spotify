#pragma once

#include <mutex>

namespace sptf
{

class AbortManager;
class LibSpotify_Backend;
class WebApi_Backend;

namespace fb2k
{

class PlayCallbacks;

}

class SpotifyInstance
{

public:
    ~SpotifyInstance() = default;

    static SpotifyInstance& Get();
    void Finalize();

    AbortManager& GetAbortManager();
    LibSpotify_Backend& GetLibSpotify_Backend();
    WebApi_Backend& GetWebApi_Backend();

private:
    SpotifyInstance() = default;
    void InitializeAll();

private:
    std::mutex mutex_;
    bool isFinalized_ = false;

    std::unique_ptr<AbortManager> pAbortManager_;
    std::unique_ptr<LibSpotify_Backend> pLibSpotify_backend_;
    std::unique_ptr<WebApi_Backend> pWebApi_backend_;
    bool fb2k_playCallbacks_initialized_ = false;
};

} // namespace sptf