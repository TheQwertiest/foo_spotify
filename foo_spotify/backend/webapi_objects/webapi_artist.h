#pragma once

#include <string>

namespace sptf
{

struct WebApi_Image;

struct WebApi_Artist_Simplified
{
    std::string id;
    std::string name;
};

struct WebApi_Artist
{
    std::string id;
    std::vector<std::unique_ptr<WebApi_Image>> images;
    std::string name;
    uint32_t popularity;
};

void to_json( nlohmann::json& j, const WebApi_Artist_Simplified& p );
void from_json( const nlohmann::json& j, WebApi_Artist_Simplified& p );

void to_json( nlohmann::json& j, const WebApi_Artist& p );
void from_json( const nlohmann::json& j, WebApi_Artist& p );

} // namespace sptf