#pragma once

#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_msg.h>

#include <chrono>

namespace web::http::experimental::listener
{
class http_listener;
}

namespace sptf
{

class WebApiAuthorizer
{
public:
    WebApiAuthorizer();
    ~WebApiAuthorizer();

    bool IsAuthenticated() const;

    const std::wstring GetAccessToken();

    void ClearAuth();
    void CancelAuth();

    void AuthenticateClean();
    void AuthenticateWithRefreshToken();

    pplx::task<void> CompleteAuthentication( const std::wstring& respondUrl );

private:
    void StartResponseListener();
    void StopResponseListener();
    void HandleAuthenticationResponse( const web::http::http_response& response );

private:
    pplx::cancellation_token_source cts_;

    std::unique_ptr<web::http::experimental::listener::http_listener> pListener_;

    std::wstring codeVerifier_;
    std::wstring state_;

    mutable std::mutex accessTokenMutex_;
    std::wstring accessToken_;
    std::wstring refreshToken_;
    std::chrono::time_point<std::chrono::system_clock> expiresIn_;
};

} // namespace sptf