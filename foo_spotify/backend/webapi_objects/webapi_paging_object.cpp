#include <stdafx.h>

#include "webapi_paging_object.h"

#include <utils/json_std_extenders.h>

namespace sptf
{

void from_json( const nlohmann::json& nlohmann_json_j, WebApi_PagingObject& nlohmann_json_t )
{
    NLOHMANN_JSON_EXPAND( NLOHMANN_JSON_PASTE( NLOHMANN_JSON_FROM, items, limit, next, offset, previous, total ) )
}

} // namespace sptf
