#include <stdafx.h>

#include "spotify_object.h"

#include <qwr/string_helpers.h>

using namespace std::literals::string_view_literals;

namespace sptf
{

bool SpotifyObject::IsValid( std::string_view input )
{
    try
    {
        SpotifyObject tmp( input );
        (void)tmp;
        return true;
    }
    catch (const qwr::QwrException&)
    {
        return false;
    }
}

SpotifyObject::SpotifyObject( std::string_view input )
{
    const auto urlPrefix = "https://open.spotify.com/"sv;
    if ( input._Starts_with( urlPrefix ) )
    {
        input.remove_prefix( urlPrefix.size() );

        const auto ret = qwr::string::Split( input, '/' );
        qwr::QwrException::ExpectTrue( ret.size() == 2, "Invalid URL" );

        const auto localType = ret[0];
        qwr::QwrException::ExpectTrue( localType == "track"sv || localType == "album"sv || localType == "playlist"sv, "Unsupported Spotify object: {}", localType );
        type = std::string( localType.cbegin(), localType.cend() );
       
        const auto localId = [localId = ret[1]] {
            const auto pos = localId.find( '?' );
            if (pos == std::string_view::npos)
            {
                return localId;
            }

            return localId.substr( 0, pos );
        }();
        qwr::QwrException::ExpectTrue( !localId.empty(), "Invalid Spotify object id: {}", localId );
        id = std::string( localId.cbegin(), localId.cend() );
    }
    else
    {
        const auto ret = qwr::string::Split( input, ':' );
        qwr::QwrException::ExpectTrue( ret.size() == 3, "Invalid URI" );
        qwr::QwrException::ExpectTrue( ret[0] == "spotify"sv, "Invalid URI" );

        const auto localType = ret[1];
        qwr::QwrException::ExpectTrue( localType == "track"sv || localType == "album"sv || localType == "playlist"sv, "Unsupported Spotify object: {}", localType );
        type = std::string( localType.cbegin(), localType.cend() );

        const auto localId = ret[2];
        qwr::QwrException::ExpectTrue( !localId.empty(), "Invalid Spotify object id: {}", localId );
        id = std::string( localId.cbegin(), localId.cend() );
    }
}

std::string SpotifyObject::ToUri()
{
    return fmt::format( "spotify:{}:{}", type, id );
}

std::string SpotifyObject::ToUrl()
{
    return fmt::format( "https://open.spotify.com/{}/{}", type, id );
}

} // namespace sptf
