#include <stdafx.h>

#include "webapi_playlist_track.h"

#include <backend/webapi_objects/webapi_objects_all.h>
#include <utils/json_std_extenders.h>

namespace sptf
{

SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_PlaylistTrack, track );

} // namespace sptf
