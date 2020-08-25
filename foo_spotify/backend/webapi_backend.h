#pragma once

#include <cpprest/http_client.h>
#include <nonstd/span.hpp>

#include <backend/webapi_cache.h>

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace sptf
{

struct WebApi_Track;
struct WebApi_Artist;
class WebApiAuthorizer;

class WebApi_Backend
{
public:
    ~WebApi_Backend();

    static WebApi_Backend& Instance();

    void Initialize();
    void Finalize();

    std::unique_ptr<WebApi_Track>
    GetTrack( const std::string& trackId );

    std::vector<std::unique_ptr<WebApi_Track>>
    GetTracksFromPlaylist( const std::string& playlistId );

    std::vector<std::unique_ptr<WebApi_Track>>
    GetTracksFromAlbum( const std::string& albumId );

    std::vector<std::unordered_multimap<std::string, std::string>>
    GetMetaForTracks( nonstd::span<const std::unique_ptr<WebApi_Track>> tracks );

    std::unique_ptr<WebApi_Artist>
    GetArtist( const std::string& artistId );

    std::filesystem::path GetAlbumImage( const std::string& albumId, const std::string& imgUrl );
    std::filesystem::path GetArtistImage( const std::string& artistId, const std::string& imgUrl );

private:
    WebApi_Backend();

    nlohmann::json GetJsonResponse( const web::uri& requestUri );

private:
    pplx::cancellation_token_source cts_;
    std::unique_ptr<WebApiAuthorizer> pAuth_;
    web::http::client::http_client client_;

    WebApi_ObjectCache<WebApi_Track> trackCache_;
    WebApi_ObjectCache<WebApi_Artist> artistCache_;

    WebApi_ImageCache albumImageCache_;
    WebApi_ImageCache artistImageCache_;
    
};

} // namespace sptf