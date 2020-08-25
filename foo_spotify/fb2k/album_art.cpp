#include <stdafx.h>

#include <backend/spotify_object.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_objects_all.h>

using namespace sptf;

namespace
{

class AlbumArtExtractorInstanceSpotify : public album_art_extractor_instance
{
public:
    AlbumArtExtractorInstanceSpotify( std::unique_ptr<WebApi_Track> track );

    //! Throws exception_album_art_not_found when the requested album art entry could not be found in the referenced media file.
    album_art_data_ptr query( const GUID& p_what, abort_callback& p_abort ) override;

private:
    WebApiBackend& waBackend_;
    std::unique_ptr<WebApi_Track> track_;
};

class AlbumArtExtractorSpotify : public album_art_extractor
{
public:
    AlbumArtExtractorSpotify();

    //! Returns whether the specified file is one of formats supported by our album_art_extractor implementation.
    //! @param p_path Path to file being queried.
    //! @param p_extension Extension of file being queried (also present in p_path parameter) - provided as a separate parameter for performance reasons.
    bool is_our_path( const char* p_path, const char* p_extension ) override;

    //! Instantiates album_art_extractor_instance providing access to album art stored in a specified media file. \n
    //! Throws one of I/O exceptions on failure; exception_album_art_not_found when the file has no album art record at all.
    //! @param p_filehint Optional; specifies a file interface to use for accessing the specified file; can be null - in that case, the implementation will open and close the file internally.
    album_art_extractor_instance_ptr open( file_ptr p_filehint, const char* p_path, abort_callback& p_abort ) override;

private:
    WebApiBackend& waBackend_;
};

} // namespace

namespace
{

AlbumArtExtractorInstanceSpotify::AlbumArtExtractorInstanceSpotify( std::unique_ptr<WebApi_Track> track )
    : waBackend_( WebApiBackend::Instance() )
    , track_( std::move( track ) )
{
    assert( track_ );
}

album_art_data_ptr AlbumArtExtractorInstanceSpotify::query( const GUID& p_what, abort_callback& p_abort )
{
    if ( p_what == album_art_ids::cover_front )
    {
        if ( track_->album->images.empty() )
        {
            throw exception_album_art_not_found();
        }

        const auto imagePath = waBackend_.GetAlbumImage( track_->album->id, track_->album->images[0]->url );

        pfc::string8_fast canPath;
        filesystem::g_get_canonical_path( imagePath.u8string().c_str(), canPath );

        file::ptr file;
        filesystem::g_open( file, canPath, filesystem::open_mode_read, p_abort );

        return album_art_data_impl::g_create( file.get_ptr(), (size_t)file->get_size_ex( p_abort ), p_abort );
    }
    else
    {
        // TODO: handle artist
        throw exception_album_art_not_found();
    }
}

AlbumArtExtractorSpotify::AlbumArtExtractorSpotify()
    : waBackend_( WebApiBackend::Instance() )
{
}

bool AlbumArtExtractorSpotify::is_our_path( const char* p_path, const char* p_extension )
{
    const auto spotifyObject = [&]() -> std::optional<SpotifyObject> {
        try
        {
            return SpotifyObject( p_path );
        }
        catch ( const qwr::QwrException& )
        {
            return std::nullopt;
        }
    }();

    return ( spotifyObject && spotifyObject->type == "track" );
}

album_art_extractor_instance_ptr AlbumArtExtractorSpotify::open( file_ptr p_filehint, const char* p_path, abort_callback& p_abort )
{
    SpotifyObject object( p_path );
    assert( object.type == "track" );

    auto track = waBackend_.GetTrack( object.id );
    if ( track->album->images.empty() )
    {
        // TODO: handle artist image
        throw exception_album_art_not_found();
    }

    return fb2k::service_new<AlbumArtExtractorInstanceSpotify>( std::move( track ) );
}

} // namespace

namespace
{
static service_factory_single_t<AlbumArtExtractorSpotify> g_album_art_extractor;
}
