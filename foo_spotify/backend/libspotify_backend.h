#pragma once

#include <backend/audio_buffer.h>
#include <backend/libspotify_backend_user.h>

#include <libspotify/api.h>

#include <condition_variable>
#include <unordered_set>

namespace sptf
{

class AbortManager;

class LibSpotify_Backend
{
public:
    LibSpotify_Backend( AbortManager& abortManager );
    LibSpotify_Backend( const LibSpotify_Backend& ) = delete;
    LibSpotify_Backend( LibSpotify_Backend&& ) = delete;
    ~LibSpotify_Backend() = default;

    void Finalize();

    void RegisterBackendUser( LibSpotify_BackendUser& backendUser );
    void UnregisterBackendUser( LibSpotify_BackendUser& backendUser );

    void AcquireDecoder( void* owner );
    void ReleaseDecoder( void* owner );

    AudioBuffer& GetAudioBuffer();

    sp_session* GetInitializedSpSession( abort_callback& abort );
    sp_session* GetWhateverSpSession();

    template <typename Fn, typename... Args>
    auto ExecSpMutex( Fn func, Args&&... args ) -> decltype( auto )
    {
        std::lock_guard lock( apiMutex_ );
        return func( std::forward<Args>( args )... );
    }

    bool Relogin( abort_callback& abort );
    bool LoginWithUI( HWND hWnd );
    void LogoutAndForget( abort_callback& abort );

    std::string GetLoggedInUserName();

private:
    void EventLoopThread();
    void StartEventLoopThread();
    void StopEventLoopThread();

    std::optional<bool> WaitForLoginStatusUpdate( abort_callback& abort );

    // callbacks

    void log_message( const char* error );
    void logged_in( sp_error error );
    void message_to_user( const char* error );
    void notify_main_thread();
    int music_delivery( const sp_audioformat* format, const void* frames, int num_frames );
    void end_of_track();
    void play_token_lost();
    void connectionstate_updated();

private:
    AbortManager& abortManager_;

    sp_session_callbacks callbacks_{};
    sp_session_config config_{};

    std::mutex decoderOwnerMutex_;
    void* pDecoderOwner_ = nullptr;

    std::mutex apiMutex_;
    sp_session* pSpSession_ = nullptr;

    std::unique_ptr<std::thread> pWorker_;
    std::mutex workerMutex_;
    std::condition_variable eventLoopCv_;
    bool hasEvents_ = false;
    bool shouldStopEventLoop_ = false;

    std::mutex backendUsersMutex_;
    std::unordered_set<LibSpotify_BackendUser*> backendUsers_;

    enum class LoginStatus
    {
        uninitialized,
        logged_in,
        logged_out,
        login_in_process,
        logout_in_process,
    };

    std::mutex loginMutex_;
    std::condition_variable loginCv_;
    LoginStatus loginStatus_ = LoginStatus::uninitialized;
    bool isLoginBad_;

    AudioBuffer audioBuffer_;
};

} // namespace sptf
