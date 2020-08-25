#pragma once

#include <string>

namespace sptf
{

struct SpotifyObject
{
    /// @throw qwr::QwrException
    SpotifyObject( std::string_view input );
    std::string ToUri();
    std::string ToUrl();
    static bool IsValid( std::string_view input );

    std::string type;
    std::string id;
};

} // namespace sptf
