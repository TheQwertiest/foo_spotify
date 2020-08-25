#pragma once

#include <memory>
#include <string>
#include <vector>

namespace sptf
{

struct WebApi_Album_Simplified;
struct WebApi_Artist_Simplified;

struct WebApi_Track_Simplified
{
    // available_markets 	array of strings 	A list of the countries in which the track can be played, identified by their ISO 3166-1 alpha-2 code.
    // explicit 	Boolean 	Whether or not the track has explicit lyrics ( true = yes it does; false = no it does not OR unknown).
    // external_urls 	an external URL object 	External URLs for this track.
    // is_playable 	boolean 	Part of the response when Track Relinking is applied. If true , the track is playable in the given market. Otherwise false.
    // linked_from 	a linked track object 	Part of the response when Track Relinking is applied and is only part of the response if the track linking, in fact, exists. The requested track has been replaced with a different track. The track in the linked_from object contains information about the originally requested track.
    // restrictions 	a restrictions object 	Part of the response when Track Relinking is applied, the original track is not available in the given market, and Spotify did not have any tracks to relink it with. The track response will still contain metadata for the original track, and a restrictions object containing the reason why the track is not available: "restrictions" : {"reason" : "market"}
    // is_local 	boolean 	Whether or not the track is from a local file.

    std::vector<std::unique_ptr<WebApi_Artist_Simplified>> artists;
    uint32_t disc_number;
    uint32_t duration_ms;
    std::string id;
    std::string name;
    std::string preview_url;
    uint32_t track_number;
};

struct WebApi_Track
{
    WebApi_Track() = default;
    WebApi_Track( std::unique_ptr<WebApi_Track_Simplified> trackSimplified, std::shared_ptr<const WebApi_Album_Simplified> albumSimplified );

    //available_markets 	array of strings 	A list of the countries in which the track can be played, identified by their ISO 3166-1 alpha-2 code.
    //explicit 	Boolean 	Whether or not the track has explicit lyrics ( true = yes it does; false = no it does not OR unknown).
    //external_ids 	an external ID object 	Known external IDs for the track.
    //external_urls 	an external URL object 	Known external URLs for this track.
    //is_playable 	boolean 	Part of the response when Track Relinking is applied. If true , the track is playable in the given market. Otherwise false.
    //linked_from 	a linked track object 	Part of the response when Track Relinking is applied, and the requested track has been replaced with different track. The track in the linked_from object contains information about the originally requested track.
    //restrictions 	a restrictions object 	Part of the response when Track Relinking is applied, the original track is not available in the given market, and Spotify did not have any tracks to relink it with. The track response will still contain metadata for the original track, and a restrictions object containing the reason why the track is not available: "restrictions" : {"reason" : "market"}
    //popularity 	integer 	The popularity of the track. The value will be between 0 and 100, with 100 being the most popular.
    //The popularity of a track is a value between 0 and 100, with 100 being the most popular. The popularity is calculated by algorithm and is based, in the most part, on the total number of plays the track has had and how recent those plays are.
    //Generally speaking, songs that are being played a lot now will have a higher popularity than songs that were played a lot in the past. Duplicate tracks (e.g. the same track from a single and an album) are rated independently. Artist and album popularity is derived mathematically from track popularity. Note that the popularity value may lag actual popularity by a few days: the value is not updated in real time.
    //is_local 	boolean 	Whether or not the track is from a local file.

    std::shared_ptr<const WebApi_Album_Simplified> album;
    std::vector<std::unique_ptr<WebApi_Artist_Simplified>> artists;
    uint32_t disc_number;
    uint32_t duration_ms;
    std::string id;
    std::string name;
    std::string preview_url;
    uint32_t track_number;
};

void to_json( nlohmann::json& j, const WebApi_Track_Simplified& p );
void from_json( const nlohmann::json& j, WebApi_Track_Simplified& p );

void to_json( nlohmann::json& j, const WebApi_Track& p );
void from_json( const nlohmann::json& j, WebApi_Track& p );

} // namespace sptf
