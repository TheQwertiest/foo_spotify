#include <stdafx.h>

#include "webapi_auth_scopes.h"

#include <unordered_set>

using namespace std::literals::string_view_literals;

namespace sptf
{

WebApiAuthScopes::WebApiAuthScopes( nonstd::span<const std::wstring_view> scopes )
{
    std::unordered_set<std::wstring_view> scopeSet( scopes.cbegin(), scopes.cend() );

#define SPTF_FILL_SCOPE( scope_var, scope_id ) \
    if ( scopeSet.count( scope_id ) )          \
    {                                          \
        scope_var = true;                      \
    }

    SPTF_FILL_SCOPE( playlist_modify_public, L"playlist-modify-public"sv )
    SPTF_FILL_SCOPE( playlist_read_private, L"playlist-read-private"sv )
    SPTF_FILL_SCOPE( playlist_modify_private, L"playlist-modify-private"sv )
    SPTF_FILL_SCOPE( playlist_read_collaborative, L"playlist-read-collaborative"sv )
    SPTF_FILL_SCOPE( user_read_playback_position, L"user-read-playback-position"sv )
    SPTF_FILL_SCOPE( user_read_recently_played, L"user-read-recently-played"sv )
    SPTF_FILL_SCOPE( user_top_read, L"user-top-read"sv )
    SPTF_FILL_SCOPE( user_follow_modify, L"user-follow-modify"sv )
    SPTF_FILL_SCOPE( user_follow_read, L"user-follow-read"sv )
    SPTF_FILL_SCOPE( user_library_read, L"user-library-read"sv )
    SPTF_FILL_SCOPE( user_library_modify, L"user-library-modify"sv )
    SPTF_FILL_SCOPE( user_read_private, L"user-read-private"sv )
    SPTF_FILL_SCOPE( user_read_email, L"user-read-email"sv )

#undef SPTF_FILL_SCOPE
}

std::wstring WebApiAuthScopes::ToWebString() const
{
    std::wstring ret;

#define SPTF_FILL_SCOPE( scope_var, scope_id ) \
    if ( scope_var )                           \
    {                                          \
        ret += scope_id;                       \
        ret += L" ";                           \
    }

    SPTF_FILL_SCOPE( playlist_modify_public, L"playlist-modify-public"sv )
    SPTF_FILL_SCOPE( playlist_read_private, L"playlist-read-private"sv )
    SPTF_FILL_SCOPE( playlist_modify_private, L"playlist-modify-private"sv )
    SPTF_FILL_SCOPE( playlist_read_collaborative, L"playlist-read-collaborative"sv )
    SPTF_FILL_SCOPE( user_read_playback_position, L"user-read-playback-position"sv )
    SPTF_FILL_SCOPE( user_read_recently_played, L"user-read-recently-played"sv )
    SPTF_FILL_SCOPE( user_top_read, L"user-top-read"sv )
    SPTF_FILL_SCOPE( user_follow_modify, L"user-follow-modify"sv )
    SPTF_FILL_SCOPE( user_follow_read, L"user-follow-read"sv )
    SPTF_FILL_SCOPE( user_library_read, L"user-library-read"sv )
    SPTF_FILL_SCOPE( user_library_modify, L"user-library-modify"sv )
    SPTF_FILL_SCOPE( user_read_private, L"user-read-private"sv )
    SPTF_FILL_SCOPE( user_read_email, L"user-read-email"sv )

#undef SPTF_FILL_SCOPE

    return ret;
}

void to_json( nlohmann::json& j, const WebApiAuthScopes& p )
{
#define SPTF_FILL_SCOPE( scope_var ) \
    if ( p.scope_var )               \
    {                                \
        j[#scope_var] = p.scope_var; \
    }

    SPTF_FILL_SCOPE( playlist_modify_public )
    SPTF_FILL_SCOPE( playlist_read_private )
    SPTF_FILL_SCOPE( playlist_modify_private )
    SPTF_FILL_SCOPE( playlist_read_collaborative )
    SPTF_FILL_SCOPE( user_read_playback_position )
    SPTF_FILL_SCOPE( user_read_recently_played )
    SPTF_FILL_SCOPE( user_top_read )
    SPTF_FILL_SCOPE( user_follow_modify )
    SPTF_FILL_SCOPE( user_follow_read )
    SPTF_FILL_SCOPE( user_library_read )
    SPTF_FILL_SCOPE( user_library_modify )
    SPTF_FILL_SCOPE( user_read_private )
    SPTF_FILL_SCOPE( user_read_email )

#undef SPTF_FILL_SCOPE
}

void from_json( const nlohmann::json& j, WebApiAuthScopes& p )
{
#define SPTF_FILL_SCOPE( scope_var )         \
    if ( j.contains( #scope_var ) )          \
    {                                        \
        j[#scope_var].get_to( p.scope_var ); \
    }

    SPTF_FILL_SCOPE( playlist_modify_public )
    SPTF_FILL_SCOPE( playlist_read_private )
    SPTF_FILL_SCOPE( playlist_modify_private )
    SPTF_FILL_SCOPE( playlist_read_collaborative )
    SPTF_FILL_SCOPE( user_read_playback_position )
    SPTF_FILL_SCOPE( user_read_recently_played )
    SPTF_FILL_SCOPE( user_top_read )
    SPTF_FILL_SCOPE( user_follow_modify )
    SPTF_FILL_SCOPE( user_follow_read )
    SPTF_FILL_SCOPE( user_library_read )
    SPTF_FILL_SCOPE( user_library_modify )
    SPTF_FILL_SCOPE( user_read_private )
    SPTF_FILL_SCOPE( user_read_email )

#undef SPTF_FILL_SCOPE
}

} // namespace sptf
