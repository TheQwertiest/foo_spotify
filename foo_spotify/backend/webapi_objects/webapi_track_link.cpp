#include <stdafx.h>

#include "webapi_track_link.h"

#include <backend/webapi_objects/webapi_media_objects.h>
#include <utils/json_std_extenders.h>

namespace sptf
{

SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_TrackLink, id );

} // namespace sptf
