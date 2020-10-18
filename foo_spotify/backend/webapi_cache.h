#pragma once

#include <nonstd/span.hpp>
#include <qwr/file_helpers.h>

#include <filesystem>
#include <memory>
#include <mutex>

namespace sptf
{

template <typename T>
class WebApi_JsonCache
{
public:
    WebApi_JsonCache( const std::string& cacheSubdir )
        : cacheSubdir_( cacheSubdir )
    {
    }

    std::optional<std::unique_ptr<T>>
    GetObjectFromCache_NonBlocking( const std::string& filename )
    {
        namespace fs = std::filesystem;

        const auto filePath = GetCachedPath( filename );
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

    void CacheObject_NonBlocking( const T& object, const std::string& filename, bool force )
    {
        namespace fs = std::filesystem;

        const auto filePath = GetCachedPath( filename );
        if ( fs::exists( filePath ) )
        {
            if ( !force )
            {
                return;
            }
            fs::remove( filePath );
        }

        fs::create_directories( filePath.parent_path() );
        qwr::file::WriteFile( filePath, nlohmann::json( object ).dump( 2 ) );
    }

private:
    std::filesystem::path GetCachedPath( const std::string& filename ) const
    {
        return path::WebApiCache() / "data" / cacheSubdir_ / fmt::format( "{}.json", filename );
    }

private:
    std::string cacheSubdir_;
};

template <typename T>
class WebApi_ObjectCache
{
public:
    WebApi_ObjectCache( const std::string& cacheSubdir )
        : jsonCache_( cacheSubdir )
    {
    }

    void CacheObject( const T& object, bool force = false )
    {
        std::lock_guard lock( cacheMutex_ );
        jsonCache_.CacheObject_NonBlocking( object, object.id, force );
    }

    void CacheObjects( const nonstd::span<std::unique_ptr<T>> objects, bool force = false )
    {
        std::lock_guard lock( cacheMutex_ );
        for ( const auto& pObject: objects )
        {
            jsonCache_.CacheObject_NonBlocking( *pObject, pObject->id, force );
        }
    }

    void CacheObjects( const nonstd::span<std::unique_ptr<const T>> objects, bool force = false )
    {
        std::lock_guard lock( cacheMutex_ );
        for ( const auto& pObject: objects )
        {
            jsonCache_.CacheObject_NonBlocking( *pObject, pObject->id, force );
        }
    }

    std::optional<std::unique_ptr<T>>
    GetObjectFromCache( const std::string& id )
    {
        std::lock_guard lock( cacheMutex_ );
        return jsonCache_.GetObjectFromCache_NonBlocking( id );
    }

private:
    std::mutex cacheMutex_;
    WebApi_JsonCache<T> jsonCache_;
};

class WebApi_ImageCache
{
public:
    WebApi_ImageCache( const std::string& cacheSubdir );

    std::filesystem::path GetImage( const std::string& id,
                                    const std::string& imgUrl,
                                    abort_callback& abort );

private:
    std::string cacheSubdir_;
    std::mutex cacheMutex_;
};

} // namespace sptf
