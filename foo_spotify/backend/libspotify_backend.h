#pragma once

#include <backend/audio_buffer.h>
#include <backend/libspotify_backend_user.h>

#include <libspotify/api.h>

#include <condition_variable>
#include <unordered_set>

namespace sptf
{

enum class ComponentStatus
{
    not_initialized,
    initialized,
};

class LibSpotify_Backend
{
public:
    LibSpotify_Backend( const LibSpotify_Backend& ) = delete;
    LibSpotify_Backend( LibSpotify_Backend&& ) = delete;
    ~LibSpotify_Backend() = default;

    static LibSpotify_Backend& Instance();

    void Initialize();
    void Finalize();

    ComponentStatus GetComponentStatus() const;

    void RegisterBackendUser( LibSpotify_BackendUser& backendUser );
    void UnregisterBackendUser( LibSpotify_BackendUser& backendUser );

    void AcquireDecoder( void* owner );
    void ReleaseDecoder( void* owner );

    AudioBuffer& GetAudioBuffer();

    sp_session* GetInitializedSpSession( abort_callback& p_abort );
    sp_session* GetWhateverSpSession();

    template <typename Fn, typename... Args>
    auto ExecSpMutex( Fn func, Args&&... args ) -> decltype( auto )
    {
        std::lock_guard lock( apiMutex_ );
        return func( std::forward<Args>( args )... );
    }

private:
    LibSpotify_Backend() = default;

    void TryToLogin( abort_callback& p_abort );
    void ShowLoginUI( sp_error last_login_result = SP_ERROR_OK );
    void WaitForLogin( abort_callback& p_abort );

    void EventLoopThread();
    void StartEventLoopThread();
    void StopEventLoopThread();

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
    sp_session_callbacks callbacks_{};
    sp_session_config config_{};

    std::mutex decoderOwnerMutex_;
    void* pDecoderOwner_ = nullptr;

    std::mutex apiMutex_;
    sp_session* pSpSession_;
    std::atomic<ComponentStatus> componentStatus_{ ComponentStatus::not_initialized };

    std::unique_ptr<std::thread> pWorker_;
    std::mutex workerMutex_;
    std::condition_variable eventLoopCv_;
    bool hasEvents_ = false;
    bool shouldStopEventLoop_ = false;

    std::mutex backendUsersMutex_;
    std::unordered_set<LibSpotify_BackendUser*> backendUsers_;

    enum class LoginStatus
    {
        logged_out,
        logged_in,
        login_in_process,
        login_in_process_with_dialog,
    };

    std::mutex loginMutex_;
    std::condition_variable loginCv_;
    LoginStatus loginStatus_ = LoginStatus::logged_out;

    AudioBuffer audioBuffer_;
};

void assertSucceeds( pfc::string8 msg, sp_error err );

} // namespace sptf
