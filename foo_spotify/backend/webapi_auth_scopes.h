#pragma once

#include <nonstd/span.hpp>

#include <string_view>

namespace sptf
{

struct WebApiAuthScopes
{
    WebApiAuthScopes() = default;
    WebApiAuthScopes( nonstd::span<const std::wstring_view> scopes );
    std::wstring ToWebString() const;

    bool playlist_modify_public = false;
    bool playlist_read_private = false;
    bool playlist_modify_private = false;
    bool playlist_read_collaborative = false;
    bool user_read_playback_position = false;
    bool user_read_recently_played = false;
    bool user_top_read = false;
    bool user_follow_modify = false;
    bool user_follow_read = false;
    bool user_library_read = false;
    bool user_library_modify = false;
    bool user_read_private = false;
    bool user_read_email = false;
};

void to_json( nlohmann::json& j, const WebApiAuthScopes& p );
void from_json( const nlohmann::json& j, WebApiAuthScopes& p );

} // namespace sptf
