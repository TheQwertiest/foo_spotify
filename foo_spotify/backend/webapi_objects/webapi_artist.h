#pragma once

#include <string>

namespace sptf
{

struct WebApi_Image;

struct WebApi_Artist_Simplified
{
    // external_urls 	an external URL object 	Known external URLs for this artist.

    std::string id;
    std::string name;
};

struct WebApi_Artist
{
    // external_urls 	an external URL object 	Known external URLs for this artist.
    // followers 	A followers object 	Information about the followers of the artist.
    // genres 	array of strings 	A list of the genres the artist is associated with. For example: "Prog Rock" , "Post-Grunge". (If not yet classified, the array is empty.)

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