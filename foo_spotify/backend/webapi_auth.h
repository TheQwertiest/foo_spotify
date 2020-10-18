#pragma once

#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>

#include <chrono>

namespace web::http::experimental::listener
{
class http_listener;
}

namespace sptf
{

class AbortManager;
struct WebApiAuthScopes;
struct AuthData;

class WebApiAuthorizer
{
public:
    WebApiAuthorizer( const web::http::client::http_client_config& config, AbortManager& abortManager );
    ~WebApiAuthorizer();

    bool HasRefreshToken() const;

    const std::wstring GetAccessToken( abort_callback& abort );

    void ClearAuth();
    void CancelAuth();

    void AuthenticateClean( const WebApiAuthScopes& scopes, std::function<void()> onResponseEnd );
    void AuthenticateClean_Cleanup();
    void AuthenticateWithRefreshToken( abort_callback& abort );

private:
    void AuthenticateWithRefreshToken_NonBlocking( abort_callback& abort );
    pplx::task<void> CompleteAuthentication( const std::wstring& respondUrl );

    void StartResponseListener( std::function<void()> onResponseEnd );
    void StopResponseListener();
    void HandleAuthenticationResponse( const web::http::http_response& response );

private:
    AbortManager& abortManager_;

    pplx::cancellation_token_source cts_;
    web::http::client::http_client client_;
    std::unique_ptr<web::http::experimental::listener::http_listener> pListener_;

    std::wstring codeVerifier_;
    std::wstring state_;

    mutable std::mutex accessTokenMutex_;
    std::unique_ptr<AuthData> pAuthData_;
};

} // namespace sptf