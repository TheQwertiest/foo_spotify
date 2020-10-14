#pragma once

#include <string>

namespace sptf
{

struct SpotifyObject
{
    /// @throw qwr::QwrException
    SpotifyObject( std::string_view input );
    /// @throw qwr::QwrException
    SpotifyObject( std::string_view type, std::string_view id );

    std::string ToUri() const;
    std::string ToUrl() const;
    std::string ToSchema() const;

    static bool IsValid( std::string_view input );

    std::string type;
    std::string id;
};

class SpotifyFilteredTrack
{
public:
    /// @throw qwr::QwrException
    SpotifyFilteredTrack( std::string_view id );

    std::string ToUri() const;
    std::string ToUrl() const;
    std::string ToSchema() const;

    static bool IsValid( std::string_view input, bool usePurePathOnly );

private:
    SpotifyObject object_;
};

} // namespace sptf
