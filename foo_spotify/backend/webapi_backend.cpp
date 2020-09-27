#include <stdafx.h>

#include "webapi_backend.h"

#include <backend/webapi_auth.h>
#include <backend/webapi_objects/webapi_objects_all.h>
#include <fb2k/advanced_config.h>
#include <utils/json_std_extenders.h>
#include <utils/abort_manager.h>

#include <component_urls.h>

#include <qwr/file_helpers.h>
#include <qwr/string_helpers.h>
#include <qwr/winapi_error_helpers.h>
#include <qwr/final_action.h>

#include <filesystem>

// TODO: (paged) search
// TODO: preferences page with auth
// TODO: separate command for fixed playlist (auto-sync? manual?)
// TODO: possible to sync playlist back to spotify?
// TODO: handle `abort` callbacks
// TODO: replace unique_ptr with shared_ptr wherever needed to avoid copying
// TODO: maybe cache parsed Tracks?

namespace fs = std::filesystem;

namespace sptf
{

WebApi_Backend::WebApi_Backend()
    : client_( url::spotifyApi )
    , trackCache_( "tracks" )
    , artistCache_( "artists" )
    , albumImageCache_( "albums" )
    , artistImageCache_( "artists" )
{
}

WebApi_Backend::~WebApi_Backend()
{
}

WebApi_Backend& WebApi_Backend::Instance()
{
    static WebApi_Backend wab;
    return wab;
}

void WebApi_Backend::Initialize()
{
    pAuth_ = std::make_unique<WebApiAuthorizer>();
}

void WebApi_Backend::Finalize()
{
    cts_.cancel();
    cts_ = pplx::cancellation_token_source();
    pAuth_.reset();
}

std::unique_ptr<const sptf::WebApi_Track>
WebApi_Backend::GetTrack( const std::string& trackId, abort_callback& abort )
{
    assert( pAuth_ );

    if ( auto trackOpt = trackCache_.GetObjectFromCache( trackId );
         trackOpt )
    {
        return std::unique_ptr<sptf::WebApi_Track>( std::move( *trackOpt ) );
    }
    else
    {
        web::uri_builder builder;
        builder.append_path( L"tracks" );
        builder.append_path( qwr::unicode::ToWide( trackId ) );

        const auto responseJson = GetJsonResponse( builder.to_uri(), abort );
        auto ret = responseJson.get<std::unique_ptr<WebApi_Track>>();

        trackCache_.CacheObject( *ret );
        return std::unique_ptr<sptf::WebApi_Track>( std::move( ret ) );
    }
}

std::vector<std::unique_ptr<const sptf::WebApi_Track>>
WebApi_Backend::GetTracksFromPlaylist( const std::string& playlistId, abort_callback& abort )
{
    size_t offset = 0;

    std::vector<std::unique_ptr<const WebApi_Track>> ret;
    while ( true )
    {
        web::uri_builder builder;
        builder.append_path( fmt::format( L"playlists/{}/tracks", qwr::unicode::ToWide( playlistId ) ) );
        builder.append_query( L"limit", 100, false );
        builder.append_query( L"offset", offset, false );
        offset += 100;

        const auto responseJson = GetJsonResponse( builder.to_uri(), abort );

        const auto itemsIt = responseJson.find( "items" );
        qwr::QwrException::ExpectTrue( responseJson.cend() != itemsIt,
                                       L"Malformed track data response response: missing `items`" );

        auto playlistTracks = itemsIt->get<std::vector<std::unique_ptr<WebApi_PlaylistTrack>>>();
        auto newData = playlistTracks | ranges::views::transform( []( auto& elem ) { return std::move( elem->track ); } ) | ranges::to_vector;
        ret.insert( ret.end(), make_move_iterator( newData.begin() ), make_move_iterator( newData.end() ) );

        if ( responseJson.at( "next" ).is_null() )
        {
            break;
        }
    }

    trackCache_.CacheObjects( ret );
    return ret;
}

std::vector<std::unique_ptr<const sptf::WebApi_Track>>
WebApi_Backend::GetTracksFromAlbum( const std::string& albumId, abort_callback& abort )
{
    assert( pAuth_ );

    auto album = [&] {
        web::uri_builder builder;
        builder.append_path( fmt::format( L"albums/{}", qwr::unicode::ToWide( albumId ) ) );

        const auto responseJson = GetJsonResponse( builder.to_uri(), abort );
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

        const auto responseJson = GetJsonResponse( builder.to_uri(), abort );

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
                      return std::make_unique<const WebApi_Track>( std::move( elem ), album );
                  } )
                  | ranges::to_vector;
    trackCache_.CacheObjects( newRet );
    return newRet;
}

std::vector<std::unordered_multimap<std::string, std::string>>
WebApi_Backend::GetMetaForTracks( nonstd::span<const std::unique_ptr<const WebApi_Track>> tracks )
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
    // TODO: check webapi_objects for other fields

    return ret;
}

std::unique_ptr<const WebApi_Artist>
WebApi_Backend::GetArtist( const std::string& artistId, abort_callback& abort )
{
    assert( pAuth_ );

    if ( auto objectOpt = artistCache_.GetObjectFromCache( artistId );
         objectOpt )
    {
        return std::unique_ptr<const WebApi_Artist>( std::move( *objectOpt ) );
    }
    else
    {
        web::uri_builder builder;
        builder.append_path( L"artists" );
        builder.append_path( qwr::unicode::ToWide( artistId ) );

        const auto responseJson = GetJsonResponse( builder.to_uri(), abort );
        auto ret = responseJson.get<std::unique_ptr<const WebApi_Artist>>();
        artistCache_.CacheObject( *ret );
        return std::unique_ptr<const WebApi_Artist>( std::move( ret ) );
    }
}

fs::path WebApi_Backend::GetAlbumImage( const std::string& albumId, const std::string& imgUrl, abort_callback& abort )
{
    return albumImageCache_.GetImage( albumId, imgUrl, abort );
}

fs::path WebApi_Backend::GetArtistImage( const std::string& artistId, const std::string& imgUrl, abort_callback& abort )
{
    return artistImageCache_.GetImage( artistId, imgUrl, abort );
}

nlohmann::json WebApi_Backend::GetJsonResponse( const web::uri& requestUri, abort_callback& abort )
{
    web::http::http_request req( web::http::methods::GET );
    req.headers().add( L"Authorization", L"Bearer " + pAuth_->GetAccessToken() );
    req.headers().add( L"Accept", L"application/json" );
    req.headers().set_content_type( L"application/json" );
    req.set_request_uri( requestUri );

    auto ctsToken = cts_.get_token();
    auto localCts = Concurrency::cancellation_token_source::create_linked_source( ctsToken );
    const auto abortableScope = AbortManager::Instance().GetAbortableScope( [&localCts] { localCts.cancel(); }, abort );

    const auto response = client_.request( req, localCts.get_token() ).get();
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
                                   L"Malformed track data response response: json is not an object" );

    return responseJson;
}

} // namespace sptf
