#include <stdafx.h>

#include "webapi_user.h"

#include <utils/json_std_extenders.h>

namespace sptf
{

void to_json( nlohmann::json& nlohmann_json_j, const WebApi_User& nlohmann_json_t )
{
    NLOHMANN_JSON_EXPAND( NLOHMANN_JSON_PASTE( NLOHMANN_JSON_TO, country, display_name, uri ) )
}

void from_json( const nlohmann::json& nlohmann_json_j, WebApi_User& nlohmann_json_t )
{
    NLOHMANN_JSON_EXPAND( NLOHMANN_JSON_PASTE( NLOHMANN_JSON_FROM, display_name, uri ) )
    if ( nlohmann_json_j.contains( "country" ) )
    {
        NLOHMANN_JSON_FROM( country )
    }
}

} // namespace sptf
