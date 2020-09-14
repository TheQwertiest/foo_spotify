#include <stdafx.h>

#include "webapi_artist.h"

#include <backend/webapi_objects/webapi_objects_all.h>
#include <utils/json_std_extenders.h>

namespace sptf
{

SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_Artist_Simplified, id, name );
SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_Artist, id, images, name, popularity );

} // namespace sptf
