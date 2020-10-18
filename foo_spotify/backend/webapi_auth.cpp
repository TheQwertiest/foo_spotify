#include <stdafx.h>

#include "webapi_auth.h"

#include <backend/spotify_instance.h>
#include <backend/webapi_auth_scopes.h>
#include <ui/ui_not_auth.h>
#include <utils/abort_manager.h>
#include <utils/json_std_extenders.h>

#include <bcrypt.h>
#include <component_urls.h>
#include <pplawait.h>
#include <winternl.h>

#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/uri_builder.h>
#include <nonstd/span.hpp>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/string_helpers.h>

#include <random>
#include <string_view>
#include <unordered_set>

#include <experimental/resumable>

// TODO: move to props
#pragma comment( lib, "bcrypt.lib" )
#pragma comment( lib, "Crypt32.lib" )
#pragma comment( lib, "winhttp.lib" )
#pragma comment( lib, "httpapi.lib" )

// TODO: add abortable to cts
// TODO: clean up mutex handling - it's a mess here

namespace fs = std::filesystem;

using namespace sptf;

namespace
{

constexpr wchar_t k_clientId[] = L"56b24aee069c4de2937c5e359de82b93";

} // namespace

namespace
{

std::wstring GenerateCodeVerifier()
{
    static constexpr wchar_t codeVerifierChars[] =
        L"abcdefghijklmnopqrstuvwxyz"
        L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        L"1234567890"
        L"_.-~";

    std::random_device rd;
    const auto size = std::uniform_int_distribution<size_t>( 43, 128 )( rd );

    std::wstring ret;
    ret.resize( size );

    std::uniform_int_distribution<size_t> charDist( 0, std::size( codeVerifierChars ) - 2 );
    for ( auto& ch: ret )
    {
        ch = codeVerifierChars[charDist( rd )];
    }

    return ret;
}

std::vector<uint8_t> Sha256Hash( nonstd::span<const uint8_t> data )
{
    // TODO: cleanup add error handling

    NTSTATUS status;
    BCRYPT_ALG_HANDLE alg_handle = nullptr;
    BCRYPT_HASH_HANDLE hash_handle = nullptr;

    std::vector<uint8_t> hash;
    DWORD hash_len = 0;
    ULONG result_len = 0;

    status = BCryptOpenAlgorithmProvider( &alg_handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0 );
    if ( !NT_SUCCESS( status ) )
    {
        goto cleanup;
    }
    status = BCryptGetProperty( alg_handle, BCRYPT_HASH_LENGTH, (PBYTE)&hash_len, sizeof( hash_len ), &result_len, 0 );
    if ( !NT_SUCCESS( status ) )
    {
        goto cleanup;
    }
    hash.resize( hash_len );

    status = BCryptCreateHash( alg_handle, &hash_handle, nullptr, 0, nullptr, 0, 0 );
    if ( !NT_SUCCESS( status ) )
    {
        goto cleanup;
    }
    status = BCryptHashData( hash_handle, (PBYTE)data.data(), (ULONG)data.size(), 0 );
    if ( !NT_SUCCESS( status ) )
    {
        goto cleanup;
    }
    status = BCryptFinishHash( hash_handle, hash.data(), hash_len, 0 );
    if ( !NT_SUCCESS( status ) )
    {
        goto cleanup;
    }

cleanup:
    if ( hash_handle )
    {
        BCryptDestroyHash( hash_handle );
    }
    if ( alg_handle )
    {
        BCryptCloseAlgorithmProvider( alg_handle, 0 );
    }

    return hash;
}

std::wstring GenerateChallengeCode( const std::wstring& codeVerifier )
{
    const auto code_u8 = qwr::unicode::ToU8( codeVerifier );
    const auto hash = Sha256Hash( nonstd::span<const uint8_t>( reinterpret_cast<const uint8_t*>( code_u8.data() ), code_u8.size() ) );
    auto base64 = utility::conversions::to_base64( hash );
    {
        while ( base64[base64.size() - 1] == L'=' )
        {
            base64.resize( base64.size() - 1 );
        }
        for ( auto& ch: base64 )
        {
            switch ( ch )
            {
            case L'+':
            {
                ch = L'-';
                break;
            }
            case L'/':
            {
                ch = L'_';
                break;
            }
            default:
            {
                break;
            }
            }
        }
    }

    return base64;
}

void OpenAuthConfirmationInBrowser( const std::wstring& url )
{
    ShellExecuteW( nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL );
}

} // namespace

namespace sptf
{

struct AuthData
{
    std::wstring accessToken;
    std::wstring refreshToken;
    std::chrono::time_point<std::chrono::system_clock> expiresIn = std::chrono::system_clock::now();
    WebApiAuthScopes scopes;
};

void to_json( nlohmann::json& j, const AuthData& p )
{
    j["access_token"] = p.accessToken;
    j["refresh_token"] = p.refreshToken;
    j["expires_in"] = std::chrono::time_point_cast<std::chrono::minutes>( p.expiresIn ).time_since_epoch().count();
    j["scopes"] = p.scopes;
}

void from_json( const nlohmann::json& j, AuthData& p )
{
    j.at( "access_token" ).get_to( p.accessToken );
    j.at( "refresh_token" ).get_to( p.refreshToken );
    p.expiresIn = std::chrono::time_point<std::chrono::system_clock>( std::chrono::minutes( j.at( "expires_in" ).get<int>() ) );
    if ( j.contains( "scopes" ) )
    {
        j["scopes"].get_to( p.scopes );
    }
    else
    { // for backwards compatibility
        WebApiAuthScopes scopes;
        scopes.playlist_read_collaborative = true;
        scopes.playlist_read_private = true;
        p.scopes = scopes;
    }
}

WebApiAuthorizer::WebApiAuthorizer( const web::http::client::http_client_config& config, AbortManager& abortManager )
    : abortManager_( abortManager )
    , client_( url::accountsApi, config )
{
    const auto authpath = path::WebApiSettings() / "auth.json";
    if ( fs::exists( authpath ) )
    {
        try
        {
            const auto data = qwr::file::ReadFileW( authpath, CP_UTF8, false );
            const auto dataJson = nlohmann::json::parse( data );
            dataJson.get_to( pAuthData_ );
        }
        catch ( const std::exception& )
        {
            pAuthData_.reset();
            try
            {
                fs::remove( authpath );
            }
            catch ( const fs::filesystem_error& )
            {
            }
        }
    }
}

WebApiAuthorizer::~WebApiAuthorizer()
{
    cts_.cancel();
    StopResponseListener();
}

bool WebApiAuthorizer::HasRefreshToken() const
{
    std::lock_guard lock( accessTokenMutex_ );
    return !!pAuthData_;
}

const std::wstring WebApiAuthorizer::GetAccessToken( abort_callback& abort )
{
    std::lock_guard lock( accessTokenMutex_ );

    if ( !pAuthData_ )
    {
        ::fb2k::inMainThread( [&] {
            playback_control::get()->stop();
            ui::NotAuthHandler::Get().ShowDialog();
        } );
        throw qwr::QwrException( "Failed to get authenticated Spotify session" );
    }

    if ( std::chrono::system_clock::now() - pAuthData_->expiresIn > std::chrono::minutes( 1 ) )
    {
        AuthenticateWithRefreshToken_NonBlocking( abort );
    }

    assert( !pAuthData_->accessToken.empty() );
    return pAuthData_->accessToken;
}

void WebApiAuthorizer::ClearAuth()
{
    pAuthData_.reset();
    codeVerifier_.clear();
    state_.clear();

    const auto authpath = path::WebApiSettings() / "auth.json";
    if ( fs::exists( authpath ) )
    {
        fs::remove( authpath );
    }
}

void WebApiAuthorizer::CancelAuth()
{
    // TODO: rethink cts handling: current scheme is not thread-safe.
    // Example: CancelAuth from preferences (and reassign cts_), while some other thread is using cts_.
    cts_.cancel();
    cts_ = pplx::cancellation_token_source();
    StopResponseListener();
    ClearAuth();
}

void WebApiAuthorizer::AuthenticateClean( const WebApiAuthScopes& scopes, std::function<void()> onResponseEnd )
{
    assert( core_api::is_main_thread() );

    StopResponseListener();
    StartResponseListener( onResponseEnd );

    // <https://developer.spotify.com/documentation/general/guides/authorization-guide/#authorization-code-flow-with-proof-key-for-code-exchange-pkce>

    web::uri_builder builder( url::accountsAuthenticate );
    builder.append_query( L"client_id", web::uri::encode_data_string( k_clientId ), false );
    builder.append_query( L"response_type", L"code" );
    builder.append_query( L"redirect_uri", web::uri::encode_data_string( url::redirectUri ), false );
    builder.append_query( L"code_challenge_method", L"S256" );
    codeVerifier_ = GenerateCodeVerifier();
    const auto challengeCode = GenerateChallengeCode( codeVerifier_ );
    builder.append_query( L"code_challenge", challengeCode );
    // TODO: cookie?
    // builder.append_query( L"state", L"azaza" );
    builder.append_query( L"scope", web::uri::encode_data_string( scopes.ToWebString() ), false );

    OpenAuthConfirmationInBrowser( builder.to_string() );
}

void WebApiAuthorizer::AuthenticateClean_Cleanup()
{
    StopResponseListener();
}

void WebApiAuthorizer::AuthenticateWithRefreshToken( abort_callback& abort )
{
    std::lock_guard lock( accessTokenMutex_ );
    AuthenticateWithRefreshToken_NonBlocking( abort );
}

void WebApiAuthorizer::AuthenticateWithRefreshToken_NonBlocking( abort_callback& abort )
{
    assert( pAuthData_ );

    web::uri_builder builder;
    builder.append_query( L"grant_type", L"refresh_token" );
    builder.append_query( L"refresh_token", pAuthData_->refreshToken );
    builder.append_query( L"client_id", web::uri::encode_data_string( k_clientId ), false );

    web::http::http_request req( web::http::methods::POST );
    req.headers().set_content_type( L"application/x-www-form-urlencoded" );
    req.set_request_uri( builder.to_uri() );

    auto ctsToken = cts_.get_token();
    auto localCts = Concurrency::cancellation_token_source::create_linked_source( ctsToken );
    const auto abortableScope = abortManager_.GetAbortableScope( [&localCts] { localCts.cancel(); }, abort );

    auto response = client_.request( req, cts_.get_token() );
    HandleAuthenticationResponse( response.get() );
}

pplx::task<void> WebApiAuthorizer::CompleteAuthentication( const std::wstring& responseUrl )
{
    // TODO: final_action
    web::uri uri( responseUrl );
    // TODO: check redirectUri?

    const auto data = web::uri::split_query( uri.query() );

    if ( data.count( L"error" ) )
    {
        throw qwr::QwrException( qwr::unicode::ToU8( fmt::format( L"Authentication failed: {}", data.at( L"error" ) ) ) );
    }

    if ( !state_.empty() && ( !data.count( L"state" ) || data.at( L"state" ) != state_ ) )
    {
        throw qwr::QwrException( "Malformed authentication response: `state` mismatch" );
    }

    qwr::QwrException::ExpectTrue( data.count( L"code" ), L"Malformed authentication response: missing `code`" );

    web::uri_builder builder;
    builder.append_query( L"client_id", k_clientId );
    builder.append_query( L"grant_type", L"authorization_code" );
    builder.append_query( L"code", data.at( L"code" ) );
    builder.append_query( L"redirect_uri", web::uri::encode_data_string( url::redirectUri ), false );
    builder.append_query( L"code_verifier", codeVerifier_, false );

    web::http::http_request req( web::http::methods::POST );
    req.headers().set_content_type( L"application/x-www-form-urlencoded" );
    req.set_body( builder.query() );

    const auto response = co_await client_.request( req, cts_.get_token() );
    HandleAuthenticationResponse( response );
}

void WebApiAuthorizer::StartResponseListener( std::function<void()> onResponseEnd )
{
    assert( !pListener_ );
    pListener_ = std::make_unique<web::http::experimental::listener::http_listener>( url::redirectUri );
    pListener_->support( [this, onResponseEnd]( const auto& request ) {
        try
        {
            if ( request.request_uri().path() == L"/" && request.request_uri().query() != L"" )
            {
                const auto callbackCaller = qwr::final_action( [&] { onResponseEnd(); } );
                try
                {
                    CompleteAuthentication( request.request_uri().to_string() ).wait();

                    web::http::http_response res;
                    res.set_status_code( web::http::status_codes::OK );
                    res.headers().set_content_type( L"text/html; charset=utf-8" );
                    res.set_body(
                        L"<html>\n"
                        L"<head>\n"
                        L"</head>\n"
                        L"<body onload=\" waitFiveSec() \">\n"
                        L"<p><b>foo_spotify</b> was successfully authenticated!</p>\n "
                        L"<p>You can close this tab now :)</p>\n "
                        L"</body>\n"
                        L"</html>" );

                    request.reply( res ).wait();
                }
                catch ( const std::exception& e )
                {
                    pAuthData_.reset();
                    auto errorMsg = qwr::unicode::ToWide( std::string( e.what() ) );
                    {
                        size_t pos = 0;
                        while ( ( pos = errorMsg.find( L"\n", pos ) ) != std::string::npos )
                        {
                            errorMsg.replace( pos, 1, L"<br>" );
                            pos += 4;
                        }
                    }

                    web::http::http_response res;
                    res.set_status_code( web::http::status_codes::OK );
                    res.headers().set_content_type( L"text/html; charset=utf-8" );
                    res.set_body(
                        L"<html>\n"
                        L"<head>\n"
                        L"</head>\n"
                        L"<p><b>foo_spotify</b> failed to authenticate!</p>\n"
                        L"<p>Error:</p>\n"
                        L"<p>"
                        + errorMsg + "</p>\n"
                                     L"</body>\n"
                                     L"</html>" );

                    request.reply( res ).wait();
                }
            }
            else
            {
                web::http::http_response res;
                res.set_status_code( web::http::status_codes::OK );
                res.headers().set_content_type( L"text/html; charset=utf-8" );
                res.set_body( L"<html><body><p>Congratulations! You found a useless page! o/</p></body></html>" );

                request.reply( res ).wait();
            }
        }
        catch ( ... )
        {
        }
    } );
    pListener_->open().wait();
}

void WebApiAuthorizer::StopResponseListener()
{
    if ( pListener_ )
    {
        pListener_->close().wait();
        pListener_.reset();
    }
}

void WebApiAuthorizer::HandleAuthenticationResponse( const web::http::http_response& response )
{
    if ( response.status_code() != 200 )
    {
        ClearAuth();

        throw qwr::QwrException( L"{}: {}\n"
                                 L"Additional data: {}\n",
                                 (int)response.status_code(),
                                 response.reason_phrase(),
                                 [&]() -> std::wstring {
                                     try
                                     {
                                         const auto responseJson = nlohmann::json::parse( response.extract_string().get() );
                                         return qwr::unicode::ToWide( responseJson.dump( 2 ) );
                                     }
                                     catch ( ... )
                                     {
                                         return response.to_string();
                                     }
                                 }() );
    }

    const auto responseJson = response.extract_json().get();
    qwr::QwrException::ExpectTrue( responseJson.is_object(),
                                   L"Malformed authentication response: json is not an object" );

    qwr::QwrException::ExpectTrue( responseJson.at( L"token_type" ).as_string() == L"Bearer",
                                   L"Malformed authentication response: invalid `token_type`: {}",
                                   responseJson.at( L"token_type" ).as_string() );

    auto pAuthData = std::make_unique<AuthData>();
    pAuthData->accessToken = responseJson.at( L"access_token" ).as_string();
    pAuthData->refreshToken = responseJson.at( L"refresh_token" ).as_string();
    pAuthData->expiresIn = std::chrono::system_clock::now() + std::chrono::seconds( responseJson.at( L"expires_in" ).as_integer() );

    auto scopesSplit = qwr::string::Split<wchar_t>( responseJson.at( L"scope" ).as_string(), L' ' );
    pAuthData->scopes = WebApiAuthScopes( scopesSplit );

    const auto settingsPath = path::WebApiSettings();
    if ( !fs::exists( settingsPath ) )
    {
        fs::create_directories( settingsPath );
    }
    // not using component config because it's not saved immediately
    qwr::file::WriteFile( settingsPath / "auth.json", nlohmann::json( pAuthData ).dump( 2 ) );

    pAuthData_ = std::move( pAuthData );
}

} // namespace sptf
