#include <stdafx.h>

#include "webapi_album.h"

#include <backend/webapi_objects/webapi_objects_all.h>
#include <utils/json_std_extenders.h>

namespace sptf
{

SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_Album_Simplified, artists, images, release_date, name, id );

} // namespace sptf
