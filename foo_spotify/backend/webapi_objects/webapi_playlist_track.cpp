#include <stdafx.h>

#include "webapi_playlist_track.h"

#include <backend/webapi_objects/webapi_media_objects.h>
#include <utils/json_std_extenders.h>

namespace sptf
{

void to_json( nlohmann::json& j, const WebApi_PlaylistTrack& p )
{
    std::visit( [&j]( auto&& arg ) {
        j["track"] = arg;
    },
                *p.track );
}

void from_json( const nlohmann::json& j, WebApi_PlaylistTrack& p )
{
    if ( j.at( "is_local" ).get<bool>() )
    {
        p.track = std::make_unique<std::variant<WebApi_Track, WebApi_LocalTrack>>( j.at( "track" ).get<WebApi_LocalTrack>() );
    }
    else
    {
        p.track = std::make_unique<std::variant<WebApi_Track, WebApi_LocalTrack>>( j.at( "track" ).get<WebApi_Track>() );
    }
}

SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_LocalTrack, uri, name );

} // namespace sptf
