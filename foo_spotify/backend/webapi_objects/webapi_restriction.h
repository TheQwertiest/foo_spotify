#pragma once

#include <memory>
#include <string>

namespace sptf
{

struct WebApi_Restriction
{
    std::string reason;
};

void to_json( nlohmann::json& j, const WebApi_Restriction& p );
void from_json( const nlohmann::json& j, WebApi_Restriction& p );

} // namespace sptf