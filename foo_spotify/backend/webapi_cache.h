#pragma once

#include <nonstd/span.hpp>
#include <qwr/file_helpers.h>

#include <filesystem>
#include <memory>
#include <mutex>

namespace sptf
{

template <typename T>
class WebApi_ObjectCache
{
public:
    WebApi_ObjectCache( const std::string& cacheSubdir )
        : cacheSubdir_( cacheSubdir )
    {
    }

    void CacheObject( const std::unique_ptr<T>& object, bool force = false )
    {
        std::lock_guard lock( cacheMutex_ );
        CacheObjectNonBlocking( object, force );
    }

    void CacheObjects( const nonstd::span<std::unique_ptr<T>> objects, bool force = false )
    {
        std::lock_guard lock( cacheMutex_ );
        for ( const auto& pObject: objects )
        {
            CacheObjectNonBlocking( pObject, force );
        }
    }

    std::optional<std::unique_ptr<T>>
    GetObjectFromCache( const std::string& id )
    {
        namespace fs = std::filesystem;

        std::lock_guard lock( cacheMutex_ );

        const auto filePath = GetCachedPath( id );
        if ( !fs::exists( filePath ) )
        {
            return std::nullopt;
        }

        const auto data = qwr::file::ReadFile( filePath, CP_UTF8, false );
        try
        {
            return nlohmann::json::parse( data ).get<std::unique_ptr<T>>();
        }
        catch ( const nlohmann::detail::exception& )
        {
            return std::nullopt;
        }
    }

private:
    void CacheObjectNonBlocking( const std::unique_ptr<T>& object, bool force )
    {
        namespace fs = std::filesystem;

        const auto filePath = GetCachedPath( object->id );
        if ( fs::exists( filePath ) )
        {
            if ( !force )
            {
                return;
            }
            fs::remove( filePath );
        }

        fs::create_directories( filePath.parent_path() );
        qwr::file::WriteFile( filePath, nlohmann::json( *object ).dump( 2 ) );
    }

    std::filesystem::path GetCachedPath( const std::string& id ) const
    {
        return path::WebApiCache() / "data" / cacheSubdir_ / fmt::format( "{}.json", id );
    }

private:
    std::string cacheSubdir_;
    std::mutex cacheMutex_;
};

class WebApi_ImageCache
{
public:
    WebApi_ImageCache( const std::string& cacheSubdir )
        : cacheSubdir_( cacheSubdir )
    {
    }

    std::filesystem::path GetImage( const std::string& id,
                                    const std::string& imgUrl );

private:
    std::string cacheSubdir_;
    std::mutex cacheMutex_;
};

} // namespace sptf