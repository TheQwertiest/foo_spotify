#include <stdafx.h>

#include "webapi_artist.h"

using json = nlohmann::json;

namespace sptf
{

SPTF_NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( WebApi_Artist_Simplified, name, id );

} // namespace sptf
