#pragma once

#include <optional>
#include <string>

namespace sptf
{

struct WebApi_User
{
    //country 	string 	The country of the user, as set in the user’s account profile. An ISO 3166-1 alpha-2 country code. This field is only available when the current user has granted access to the user-read-private scope.
    std::optional<std::string> display_name;
    //std::optional<std::string> email;
    //external_urls 	an external URL object 	Known external URLs for this user.
    //followers 	A followers object 	Information about the followers of the user.
    //href 	string 	A link to the Web API endpoint for this user.
    //id 	string 	The Spotify user ID for the user.
    //images 	an array of image objects 	The user’s profile image.
    //product 	string 	The user’s Spotify subscription level: “premium”, “free”, etc. (The subscription level “open” can be considered the same as “free”.) This field is only available when the current user has granted access to the user-read-private scope.
    std::string uri;
};

void to_json( nlohmann::json& j, const WebApi_User& p );
void from_json( const nlohmann::json& j, WebApi_User& p );

} // namespace sptf
