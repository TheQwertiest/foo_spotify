#pragma once

#include <string>

namespace sptf
{

struct WebApi_Artist_Simplified
{
    std::string name;
    std::string id;
};

void to_json( nlohmann::json& j, const WebApi_Artist_Simplified& p );
void from_json( const nlohmann::json& j, WebApi_Artist_Simplified& p );

} // namespace sptf