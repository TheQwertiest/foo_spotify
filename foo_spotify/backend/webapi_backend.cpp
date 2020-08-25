#include <stdafx.h>

#include "webapi_backend.h"

#include <backend/webapi_auth.h>
#include <backend/webapi_objects/webapi_objects_all.h>
#include <fb2k/advanced_config.h>
#include <utils/json_std_extenders.h>

#include <component_urls.h>

#include <qwr/file_helpers.h>
#include <qwr/string_helpers.h>
#include <qwr/winapi_error_helpers.h>

#include <filesystem>

// move to props
#pragma comment( lib, "Urlmon.lib" )

// TODO: (paged) search
// TODO: preferences page with auth
// TODO: batch add by playlist
// TODO: separate command for fixed playlist (auto-sync? manual?)
// TODO: possible to sync playlist back to spotify?
// TODO: handle drag-n-drop
// TODO: handle `abort` callbacks
// TODO: replace unique_ptr with shared_ptr wherever needed to avoid copying
// TODO: smart_ptr<T> > smart_ptr<const T>
// TODO: maybe cache parsed Tracks?

namespace fs = std::filesystem;

namespace sptf
{

WebApiBackend::WebApiBackend()
    : client_( url::spotifyApi )
{
}

WebApiBackend::~WebApiBackend()
{
}

WebApiBackend& WebApiBackend::Instance()
{
    static WebApiBackend wab;
    return wab;
}

void WebApiBackend::Initialize()
{
    pAuth_ = std::make_unique<WebApiAuthorizer>();
    pfc::string8 proxy;
    sptf::config::advanced::network_proxy.get( proxy );
}

void WebApiBackend::Finalize()
{
    cts_.cancel();
    cts_ = pplx::cancellation_token_source();
    pAuth_.reset();
}

std::unique_ptr<sptf::WebApi_Track>
WebApiBackend::GetTrack( const std::string& trackId )
{
    assert( pAuth_ );

    if ( auto trackOpt = GetTrackFromCache( trackId );
         trackOpt )
    {
        return std::unique_ptr<sptf::WebApi_Track>( std::move( *trackOpt ) );
    }
    else
    {
        web::uri_builder builder;
        builder.append_path( L"tracks" );
        builder.append_query( L"ids", qwr::unicode::ToWide( trackId ), false );

        const auto responseJson = GetJsonResponse( builder.to_uri() );

        const auto tracksIt = responseJson.find( "tracks" );
        qwr::QwrException::ExpectTrue( responseJson.cend() != tracksIt,
                                       L"Malformed track data response response: missing `tracks`" );

        auto ret = tracksIt->get<std::vector<std::unique_ptr<WebApi_Track>>>();
        assert( ret.size() == 1 );
        CacheTracks( ret );
        return std::unique_ptr<sptf::WebApi_Track>( std::move( ret[0] ) );
    }
}

std::vector<std::unique_ptr<sptf::WebApi_Track>>
WebApiBackend::GetTracksFromPlaylist( const std::string& playlistId )
{
    size_t offset = 0;

    std::vector<std::unique_ptr<WebApi_Track>> ret;
    while ( true )
    {
        web::uri_builder builder;
        builder.append_path( fmt::format( L"playlists/{}/tracks", qwr::unicode::ToWide( playlistId ) ) );
        builder.append_query( L"limit", 100, false );
        builder.append_query( L"offset", offset, false );
        offset += 100;

        const auto responseJson = GetJsonResponse( builder.to_uri() );

        const auto itemsIt = responseJson.find( "items" );
        qwr::QwrException::ExpectTrue( responseJson.cend() != itemsIt,
                                       L"Malformed track data response response: missing `items`" );

        auto newData = itemsIt->get<std::vector<std::unique_ptr<WebApi_Track>>>();
        ret.insert( ret.end(), make_move_iterator( newData.begin() ), make_move_iterator( newData.end() ) );

        if ( responseJson.at( "next" ).is_null() )
        {
            break;
        }
    }

    CacheTracks( ret );
    return ret;
}

std::vector<std::unique_ptr<sptf::WebApi_Track>>
WebApiBackend::GetTracksFromAlbum( const std::string& albumId )
{
    assert( pAuth_ );

    auto album = [&] {
        web::uri_builder builder;
        builder.append_path( fmt::format( L"albums/{}", qwr::unicode::ToWide( albumId ) ) );

        const auto responseJson = GetJsonResponse( builder.to_uri() );
        return responseJson.get<std::shared_ptr<WebApi_Album_Simplified>>();
    }();

    size_t offset = 0;

    std::vector<std::unique_ptr<WebApi_Track_Simplified>> ret;
    while ( true )
    {
        web::uri_builder builder;
        builder.append_path( fmt::format( L"albums/{}/tracks", qwr::unicode::ToWide( albumId ) ) );
        builder.append_query( L"limit", 50, false );
        builder.append_query( L"offset", offset, false );
        offset += 50;

        const auto responseJson = GetJsonResponse( builder.to_uri() );

        const auto itemsIt = responseJson.find( "items" );
        qwr::QwrException::ExpectTrue( responseJson.cend() != itemsIt,
                                       L"Malformed track data response response: missing `items`" );

        auto newData = itemsIt->get<std::vector<std::unique_ptr<WebApi_Track_Simplified>>>();
        ret.insert( ret.end(), make_move_iterator( newData.begin() ), make_move_iterator( newData.end() ) );

        if ( responseJson.at( "next" ).is_null() )
        {
            break;
        }
    }

    auto newRet = ranges::views::transform( ret, [&]( auto&& elem ) {
                      return std::make_unique<WebApi_Track>( std::move( elem ), album );
                  } )
                  | ranges::to_vector;
    CacheTracks( newRet );
    return newRet;
}

std::vector<std::unordered_multimap<std::string, std::string>>
WebApiBackend::GetMetaForTracks( nonstd::span<const std::unique_ptr<WebApi_Track>> tracks )
{
    std::vector<std::unordered_multimap<std::string, std::string>> ret;
    for ( const auto& track: tracks )
    {
        auto& curMap = ret.emplace_back();

        // This length will be overriden during playback
        curMap.emplace( "SPTF_LENGTH", fmt::format( "{}", track->duration_ms ) );

        curMap.emplace( "TITLE", track->name );
        curMap.emplace( "TRACKNUMBER", fmt::format( "{}", track->track_number ) );
        curMap.emplace( "DISCNUMBER", fmt::format( "{}", track->disc_number ) );

        for ( const auto& artist: track->artists )
        {
            curMap.emplace( "ARTIST", artist->name );
        }

        const auto& album = track->album;
        curMap.emplace( "ALBUM", album->name );
        curMap.emplace( "DATE", album->release_date );

        for ( const auto& artist: album->artists )
        {
            curMap.emplace( "ALBUM ARTIST", artist->name );
        }
    }

    // TODO: cache data (<https://fy.3dyd.com/help/titleformatting/#Metadata_length_limitations> might be useful)
    // TODO: save spotify links
    // TODO: save spotify uris
    // TODO: album genre
    // TODO: album popularity (maybe?)
    // TODO: preview_url

    return ret;
}

fs::path WebApiBackend::GetAlbumImage( const std::string& albumId, const std::string& imgUrl )
{
    std::lock_guard lock( albumImageCacheMutex_ );

    const auto imagePath = path::WebApiCache() / "images" / "albums" / fmt::format( "{}.jpeg", albumId );
    if ( !fs::exists( imagePath ) )
    {
        fs::create_directories( imagePath.parent_path() );

        const auto url_w = qwr::unicode::ToWide( imgUrl );
        auto hr = URLDownloadToFile( nullptr, url_w.c_str(), imagePath.c_str(), 0, nullptr );
        qwr::error::CheckHR( hr, "URLDownloadToFile" );
    }
    assert( fs::exists( imagePath ) );
    return imagePath;
}

nlohmann::json WebApiBackend::GetJsonResponse( const web::uri& requestUri )
{
    web::http::http_request req( web::http::methods::GET );
    req.headers().add( L"Authorization", L"Bearer " + pAuth_->GetAccessToken() );
    req.headers().add( L"Accept", L"application/json" );
    req.headers().set_content_type( L"application/json" );
    req.set_request_uri( requestUri );

    const auto response = client_.request( req, cts_.get_token() ).get();
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

    const auto responseJson = nlohmann::json::parse( response.extract_string().get() );
    qwr::QwrException::ExpectTrue( responseJson.is_object(),
                                   L"Malformed track data response response: json is not an array" );

    return responseJson;
}

void WebApiBackend::CacheTracks( const std::vector<std::unique_ptr<WebApi_Track>>& tracks, bool force )
{
    std::lock_guard lock( trackCacheMutex_ );

    for ( const auto& pTrack: tracks )
    {
        const auto trackPath = path::WebApiCache() / "tracks" / fmt::format( "{}.json", pTrack->id );
        if ( fs::exists( trackPath ) )
        {
            if ( !force )
            {
                continue;
            }
            fs::remove( trackPath );
        }

        fs::create_directories( trackPath.parent_path() );
        qwr::file::WriteFile( trackPath, nlohmann::json( *pTrack ).dump( 2 ) );
    }
}

std::optional<std::unique_ptr<sptf::WebApi_Track>>
WebApiBackend::GetTrackFromCache( const std::string& trackId )
{
    std::lock_guard lock( trackCacheMutex_ );

    const auto trackPath = path::WebApiCache() / "tracks" / fmt::format( "{}.json", trackId );
    if ( !fs::exists( trackPath ) )
    {
        return std::nullopt;
    }

    const auto data = qwr::file::ReadFile( trackPath, CP_UTF8, false );
    try
    {
        return nlohmann::json::parse( data ).get<std::unique_ptr<sptf::WebApi_Track>>();
    }
    catch ( const nlohmann::detail::exception& )
    {
        return std::nullopt;
    }
}

} // namespace sptf
