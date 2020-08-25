#pragma once

#include <memory>
#include <string>
#include <vector>

namespace sptf
{

struct WebApi_Artist_Simplified;
struct WebApi_Image;

struct WebApi_Album_Simplified
{
    std::vector<std::unique_ptr<WebApi_Artist_Simplified>> artists;
    std::vector<std::unique_ptr<WebApi_Image>> images;
    std::string release_date;
    std::string name;
    std::string id;
};

void to_json( nlohmann::json& j, const WebApi_Album_Simplified& p );
void from_json( const nlohmann::json& j, WebApi_Album_Simplified& p );

} // namespace sptf