#pragma once

#include <backend/webapi_cache.h>

#include <cpprest/http_client.h>
#include <nonstd/span.hpp>

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace sptf
{

struct WebApi_User;
struct WebApi_Track;
struct WebApi_Artist;
class WebApiAuthorizer;
class AbortManager;

class WebApi_Backend
{
public:
    WebApi_Backend( AbortManager& abortManager );
    ~WebApi_Backend();

    void Finalize();

    WebApiAuthorizer& GetAuthorizer();

    std::unique_ptr<const WebApi_User>
    GetUser( abort_callback& abort );

    std::unique_ptr<const WebApi_Track>
    GetTrack( const std::string& trackId, abort_callback& abort );

    std::vector<std::unique_ptr<const WebApi_Track>>
    GetTracksFromPlaylist( const std::string& playlistId, abort_callback& abort );

    std::vector<std::unique_ptr<const WebApi_Track>>
    GetTracksFromAlbum( const std::string& albumId, abort_callback& abort );

    std::vector<std::unordered_multimap<std::string, std::string>>
    GetMetaForTracks( nonstd::span<const std::unique_ptr<const WebApi_Track>> tracks );

    std::unique_ptr<const WebApi_Artist>
    GetArtist( const std::string& artistId, abort_callback& abort );

    std::filesystem::path GetAlbumImage( const std::string& albumId, const std::string& imgUrl, abort_callback& abort );
    std::filesystem::path GetArtistImage( const std::string& artistId, const std::string& imgUrl, abort_callback& abort );

private:
    nlohmann::json GetJsonResponse( const web::uri& requestUri, abort_callback& abort );

private:
    AbortManager& abortManager_;

    pplx::cancellation_token_source cts_;
    std::unique_ptr<WebApiAuthorizer> pAuth_;
    web::http::client::http_client client_;

    WebApi_ObjectCache<WebApi_Track> trackCache_;
    WebApi_ObjectCache<WebApi_Artist> artistCache_;

    WebApi_ImageCache albumImageCache_;
    WebApi_ImageCache artistImageCache_;
};

} // namespace sptf