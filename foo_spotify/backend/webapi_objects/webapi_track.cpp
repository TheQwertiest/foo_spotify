#include <stdafx.h>

#include "webapi_track.h"

#include <backend/webapi_objects/webapi_objects_all.h>
#include <utils/json_std_extenders.h>

namespace sptf
{

WebApi_Track::WebApi_Track( std::unique_ptr<WebApi_Track_Simplified> trackSimplified,
                            std::shared_ptr<WebApi_Album_Simplified> albumSimplified )
{
    album = albumSimplified;
    artists = std::move( trackSimplified->artists );
    disc_number = trackSimplified->disc_number;
    duration_ms = trackSimplified->duration_ms;
    name = trackSimplified->name;
    preview_url = trackSimplified->preview_url;
    track_number = trackSimplified->track_number;
    id = trackSimplified->id;
}

SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_Track_Simplified, artists, disc_number, duration_ms, name, preview_url, track_number, id );
SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_Track, album, artists, disc_number, duration_ms, name, preview_url, track_number, id );

} // namespace sptf
