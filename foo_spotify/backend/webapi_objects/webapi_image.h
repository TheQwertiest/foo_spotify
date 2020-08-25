#pragma once

#include <memory>
#include <string>

namespace sptf
{

struct WebApi_Image
{
    uint32_t height;
    std::string url;
    uint32_t width;
};

void to_json( nlohmann::json& j, const WebApi_Image& p );
void from_json( const nlohmann::json& j, WebApi_Image& p );

} // namespace sptf