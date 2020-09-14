#pragma once

#include <memory>
#include <string>

namespace sptf
{

struct WebApi_Track;

struct WebApi_PlaylistTrack
{
    // added_at 	a timestamp 	The date and time the track was added. Note that some very old playlists may return null in this field.
    // added_by 	a user object 	The Spotify user who added the track. Note that some very old playlists may return null in this field.
    // is_local 	a Boolean 	Whether this track is a local file or not.
    std::unique_ptr<WebApi_Track> track;
};

void to_json( nlohmann::json& j, const WebApi_PlaylistTrack& p );
void from_json( const nlohmann::json& j, WebApi_PlaylistTrack& p );

} // namespace sptf
