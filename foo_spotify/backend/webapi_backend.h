#pragma once

#include <cpprest/http_client.h>
#include <nonstd/span.hpp>

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace sptf
{

struct WebApi_Track;
class WebApiAuthorizer;

class WebApiBackend
{
public:
    ~WebApiBackend();

    static WebApiBackend& Instance();

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

    std::filesystem::path GetAlbumImage( const std::string& albumId, const std::string& imgUrl );

private:
    WebApiBackend();

    nlohmann::json GetJsonResponse( const web::uri& requestUri );

    void CacheTracks( const std::vector<std::unique_ptr<WebApi_Track>>& tracks, bool force = false );

    std::optional<std::unique_ptr<WebApi_Track>>
    GetTrackFromCache( const std::string& trackId );

private:
    pplx::cancellation_token_source cts_;
    std::unique_ptr<WebApiAuthorizer> pAuth_;
    web::http::client::http_client client_;

    std::mutex trackCacheMutex_;
    std::mutex albumImageCacheMutex_;
};

} // namespace sptf