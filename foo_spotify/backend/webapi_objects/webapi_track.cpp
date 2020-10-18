#include <stdafx.h>

#include "webapi_track.h"

#include <backend/webapi_objects/webapi_media_objects.h>
#include <utils/json_std_extenders.h>

namespace sptf
{

WebApi_Track::WebApi_Track( std::unique_ptr<WebApi_Track_Simplified> trackSimplified,
                            std::shared_ptr<const WebApi_Album_Simplified> albumSimplified )
{
    album = albumSimplified;
    artists = std::move( trackSimplified->artists );
    disc_number = trackSimplified->disc_number;
    duration_ms = trackSimplified->duration_ms;
    name = trackSimplified->name;
    linked_from = std::move( trackSimplified->linked_from );
    restrictions = std::move( trackSimplified->restrictions );
    preview_url = trackSimplified->preview_url;
    track_number = trackSimplified->track_number;
    id = trackSimplified->id;
}

void to_json( nlohmann::json& nlohmann_json_j, const WebApi_Track_Simplified& nlohmann_json_t )
{
    // we don't need to save `restrictions`, since it's only used on initial parsing
    NLOHMANN_JSON_EXPAND( NLOHMANN_JSON_PASTE( NLOHMANN_JSON_TO, artists, disc_number, duration_ms, linked_from, name, preview_url, track_number, id ) )
}

void from_json( const nlohmann::json& nlohmann_json_j, WebApi_Track_Simplified& nlohmann_json_t )
{
    NLOHMANN_JSON_EXPAND( NLOHMANN_JSON_PASTE( NLOHMANN_JSON_FROM, artists, disc_number, duration_ms, name, preview_url, track_number, id ) )
    if ( nlohmann_json_j.contains( "linked_from" ) )
    {
        NLOHMANN_JSON_FROM( linked_from )
    }
    if ( nlohmann_json_j.contains( "restrictions" ) )
    {
        NLOHMANN_JSON_FROM( restrictions )
    }
}

void to_json( nlohmann::json& nlohmann_json_j, const WebApi_Track& nlohmann_json_t )
{
    // we don't need to save `restrictions`, since it's only used on initial parsing
    NLOHMANN_JSON_EXPAND( NLOHMANN_JSON_PASTE( NLOHMANN_JSON_TO, album, artists, disc_number, duration_ms, linked_from, name, preview_url, track_number, id ) )
}

void from_json( const nlohmann::json& nlohmann_json_j, WebApi_Track& nlohmann_json_t )
{
    NLOHMANN_JSON_EXPAND( NLOHMANN_JSON_PASTE( NLOHMANN_JSON_FROM, album, artists, disc_number, duration_ms, name, preview_url, track_number, id ) )
    if ( nlohmann_json_j.contains( "linked_from" ) )
    {
        NLOHMANN_JSON_FROM( linked_from )
    }
    if ( nlohmann_json_j.contains( "restrictions" ) )
    {
        NLOHMANN_JSON_FROM( restrictions )
    }
}

} // namespace sptf
