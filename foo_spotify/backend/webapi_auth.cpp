#include <stdafx.h>

#include "webapi_auth.h"

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

#include <random>
#include <string_view>

#include <experimental/resumable>

// move to props
#pragma comment( lib, "bcrypt.lib" )
#pragma comment( lib, "Crypt32.lib" )
#pragma comment( lib, "winhttp.lib" )
#pragma comment( lib, "httpapi.lib" )

namespace fs = std::filesystem;

namespace
{

constexpr wchar_t k_clientId[] = L"56b24aee069c4de2937c5e359de82b93";

constexpr wchar_t k_proxy[] = L"http://127.0.0.1:3128";

web::http::client::http_client_config config;

} // namespace

namespace
{

constexpr wchar_t codeVerifierChars[] =
    L"abcdefghijklmnopqrstuvwxyz"
    L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    L"1234567890"
    L"_.-~";

std::wstring GenerateCodeVerifier()
{
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

WebApiAuthorizer::WebApiAuthorizer()
    : expiresIn_( std::chrono::system_clock::now() )
{
    const auto authpath = path::WebApiSettings() / "auth.json";
    if ( fs::exists( authpath ) )
    {
        const auto data = qwr::file::ReadFileW( authpath, CP_UTF8, false );
        const auto dataJson = web::json::value::parse( data );
        accessToken_ = dataJson.at( L"access_token" ).as_string();
        refreshToken_ = dataJson.at( L"refresh_token" ).as_string();
        expiresIn_ = std::chrono::time_point<std::chrono::system_clock>( std::chrono::minutes( dataJson.at( L"expires_in" ).as_integer() ) );
    }

    // TODO: move to advanced
    //config.set_proxy( web::web_proxy( L"http://127.0.0.1:3128" ) );
}

WebApiAuthorizer::~WebApiAuthorizer()
{
    cts_.cancel();
    cts_ = pplx::cancellation_token_source();
    StopResponseListener();
}

bool WebApiAuthorizer::IsAuthenticated() const
{
    std::lock_guard lock( accessTokenMutex_ );
    return !accessToken_.empty() && !refreshToken_.empty();
}

const std::wstring WebApiAuthorizer::GetAccessToken()
{
    std::lock_guard lock( accessTokenMutex_ );

    qwr::QwrException::ExpectTrue( !refreshToken_.empty(), "``foo_spotify` is not authenticated!`" );

    if ( std::chrono::system_clock::now() - expiresIn_ > std::chrono::minutes( 1 ) )
    {
        AuthenticateWithRefreshToken();
    }

    assert( !accessToken_.empty() );
    return accessToken_;
}

void WebApiAuthorizer::ClearAuth()
{
    codeVerifier_.clear();
    accessToken_.clear();
    refreshToken_.clear();
    state_.clear();

    const auto authpath = path::WebApiSettings() / "auth.json";
    if ( fs::exists( authpath ) )
    {
        fs::remove( authpath );
    }
}

void WebApiAuthorizer::CancelAuth()
{
    cts_.cancel();
    cts_ = pplx::cancellation_token_source();
    StopResponseListener();
    ClearAuth();
}

void WebApiAuthorizer::AuthenticateClean()
{
    StartResponseListener();

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
    builder.append_query( L"scope", web::uri::encode_data_string( L"playlist-read-collaborative playlist-read-private" ), false );

    OpenAuthConfirmationInBrowser( builder.to_string() );
}

void WebApiAuthorizer::AuthenticateWithRefreshToken()
{
    web::uri_builder builder;
    builder.append_query( L"grant_type", L"refresh_token" );
    assert( !refreshToken_.empty() );
    builder.append_query( L"refresh_token", refreshToken_ );
    builder.append_query( L"client_id", web::uri::encode_data_string( k_clientId ), false );

    web::http::http_request req( web::http::methods::POST );
    req.headers().set_content_type( L"application/x-www-form-urlencoded" );
    req.set_request_uri( builder.to_uri() );

    web::http::client::http_client client( url::accountsApi, config );
    auto response = client.request( req, cts_.get_token() );
    HandleAuthenticationResponse( response.get() );
}

// TODO: move to private?
pplx::task<void> WebApiAuthorizer::CompleteAuthentication( const std::wstring& responseUrl )
{
    // TODO: final_action
    console::info( "foo_scrobble: Requesting session key" );

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

    web::http::client::http_client client( url::accountsApi, config );
    const auto response = co_await client.request( req, cts_.get_token() );
    HandleAuthenticationResponse( response );
}

void WebApiAuthorizer::StartResponseListener()
{
    assert( !pListener_ );
    pListener_ = std::make_unique<web::http::experimental::listener::http_listener>( url::redirectUri );
    pListener_->support( [this]( const auto& request ) {
        try
        {
            if ( request.request_uri().path() == L"/" && request.request_uri().query() != L"" )
            {
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
        throw qwr::QwrException( qwr::unicode::ToU8(
            fmt::format( L"{}: {}\n"
                         L"Additional data: {}\n",
                         response.status_code(),
                         response.reason_phrase(),
                         [&] {
                             try
                             {
                                 return response.extract_json().get().serialize();
                             }
                             catch ( ... )
                             {
                                 return response.to_string();
                             }
                         }() ) ) );
    }

    const auto responseJson = response.extract_json().get();
    qwr::QwrException::ExpectTrue( responseJson.is_object(),
                                   L"Malformed authentication response: json is not an object" );

    qwr::QwrException::ExpectTrue( responseJson.at( L"token_type" ).as_string() == L"Bearer",
                                   L"Malformed authentication response: invalid `token_type`: {}",
                                   responseJson.at( L"token_type" ).as_string() );

    accessToken_ = responseJson.at( L"access_token" ).as_string();
    refreshToken_ = responseJson.at( L"refresh_token" ).as_string();
    expiresIn_ = std::chrono::system_clock::now() + std::chrono::seconds( responseJson.at( L"expires_in" ).as_integer() );

    web::json::value configJson;
    configJson[L"access_token"] = web::json::value( accessToken_ );
    configJson[L"refresh_token"] = web::json::value( refreshToken_ );
    configJson[L"expires_in"] = web::json::value( std::chrono::time_point_cast<std::chrono::minutes>( expiresIn_ ).time_since_epoch().count() );
    
    const auto settingsPath = path::WebApiSettings();
    if ( !fs::exists( settingsPath ) )
    {
        fs::create_directories( settingsPath );
    }
    // not using component config because it's not saved immediately
    qwr::file::WriteFile( settingsPath / "auth.json", qwr::unicode::ToU8( configJson.serialize() ) );
}

} // namespace sptf
