#include <stdafx.h>

#include "webapi_cache.h"

#include <qwr/winapi_error_helpers.h>

// move to props
#pragma comment( lib, "Urlmon.lib" )

namespace fs = std::filesystem;

namespace sptf
{

fs::path WebApi_ImageCache::GetImage( const std::string& id, const std::string& imgUrl )
{
    std::lock_guard lock( cacheMutex_ );

    const auto imagePath = path::WebApiCache() / "images" / cacheSubdir_ / fmt::format( "{}.jpeg", id );
    if ( !fs::exists( imagePath ) )
    {
        fs::create_directories( imagePath.parent_path() );

        const auto url_w = qwr::unicode::ToWide( imgUrl );
        auto hr = URLDownloadToFile( nullptr, url_w.c_str(), imagePath.c_str(), 0, nullptr );
        qwr::error::CheckHR( hr, "URLDownloadToFile" );
    }
    assert( fs::exists( imagePath ) );
    return imagePath;
}

} // namespace sptf