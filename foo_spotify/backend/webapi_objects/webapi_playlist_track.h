#pragma once

#include <memory>
#include <string>
#include <variant>

namespace sptf
{

struct WebApi_Track;

struct WebApi_LocalTrack
{
    std::string id;
    std::optional<std::string> name;
};

struct WebApi_PlaylistTrack
{
    // added_at 	a timestamp 	The date and time the track was added. Note that some very old playlists may return null in this field.
    // added_by 	a user object 	The Spotify user who added the track. Note that some very old playlists may return null in this field.
    std::unique_ptr<std::variant<WebApi_Track, WebApi_LocalTrack>> track;
};

void to_json( nlohmann::json& j, const WebApi_PlaylistTrack& p );
void from_json( const nlohmann::json& j, WebApi_PlaylistTrack& p );

void to_json( nlohmann::json& j, const WebApi_LocalTrack& p );
void from_json( const nlohmann::json& j, WebApi_LocalTrack& p );

} // namespace sptf
