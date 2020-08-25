#include <stdafx.h>

#include "webapi_user.h"

#include <utils/json_std_extenders.h>

namespace sptf
{

SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_User, display_name, uri );

} // namespace sptf
