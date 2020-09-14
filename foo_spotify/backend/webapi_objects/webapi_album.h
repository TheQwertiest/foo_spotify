#pragma once

#include <memory>
#include <string>
#include <vector>

namespace sptf
{

struct WebApi_Artist_Simplified;
struct WebApi_Image;

struct WebApi_Album_Simplified
{
    // album_group 	string, optional 	The field is present when getting an artist’s albums. Possible values are “album”, “single”, “compilation”, “appears_on”. Compare to album_type this field represents relationship between the artist and the album.
    // album_type 	string 	The type of the album: one of “album”, “single”, or “compilation”.
    // available_markets 	array of strings 	The markets in which the album is available: ISO 3166-1 alpha-2 country codes. Note that an album is considered available in a market when at least 1 of its tracks is available in that market.
    // external_urls 	an external URL object 	Known external URLs for this album.
    // release_date_precision 	string 	The precision with which release_date value is known: year , month , or day.
    // restrictions 	a restrictions object 	Part of the response when Track Relinking is applied, the original track is not available in the given market, and Spotify did not have any tracks to relink it with. The track response will still contain metadata for the original track, and a restrictions object containing the reason why the track is not available: "restrictions" : {"reason" : "market"}

    std::vector<std::unique_ptr<WebApi_Artist_Simplified>> artists;
    std::vector<std::unique_ptr<WebApi_Image>> images;
    std::string id;
    std::string release_date;
    std::string name;
};

void to_json( nlohmann::json& j, const WebApi_Album_Simplified& p );
void from_json( const nlohmann::json& j, WebApi_Album_Simplified& p );

} // namespace sptf