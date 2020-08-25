#pragma once

#include <memory>
#include <string>
#include <vector>

namespace sptf
{

struct WebApi_Album_Simplified;
struct WebApi_Artist_Simplified;

struct WebApi_Track_Simplified
{
    std::vector<std::unique_ptr<WebApi_Artist_Simplified>> artists;
    uint32_t disc_number;
    uint32_t duration_ms;
    std::string name;
    std::string preview_url;
    uint32_t track_number;
    std::string id;
};

struct WebApi_Track
{
    WebApi_Track() = default;
    WebApi_Track( std::unique_ptr<WebApi_Track_Simplified> trackSimplified, std::shared_ptr<WebApi_Album_Simplified> albumSimplified );

    std::shared_ptr<WebApi_Album_Simplified> album;
    std::vector<std::unique_ptr<WebApi_Artist_Simplified>> artists;
    uint32_t disc_number;
    uint32_t duration_ms;
    std::string name;
    //uint32_t popularity;
    std::string preview_url;
    uint32_t track_number;
    std::string id;
};

void to_json( nlohmann::json& j, const WebApi_Track_Simplified& p );
void from_json( const nlohmann::json& j, WebApi_Track_Simplified& p );

void to_json( nlohmann::json& j, const WebApi_Track& p );
void from_json( const nlohmann::json& j, WebApi_Track& p );

} // namespace sptf
