#pragma once

#include <memory>
#include <string>
namespace sptf
{

struct WebApi_TrackLink
{
    // external_urls 	an external URL object 	Known external URLs for this track.
    // href 	string 	A link to the Web API endpoint providing full details of the track.
    std::string id;
    // type 	string 	The object type: “track”.
    // uri 	string 	The Spotify URI for the track.
};

void to_json( nlohmann::json& j, const WebApi_TrackLink& p );
void from_json( const nlohmann::json& j, WebApi_TrackLink& p );

} // namespace sptf
