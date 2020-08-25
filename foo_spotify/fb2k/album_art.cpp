#include <stdafx.h>

#include <backend/spotify_instance.h>
#include <backend/spotify_object.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_objects_all.h>

using namespace sptf;

namespace
{

class AlbumArtExtractorInstanceSpotify : public album_art_extractor_instance
{
public:
    AlbumArtExtractorInstanceSpotify( std::unique_ptr<const WebApi_Track> track, std::unique_ptr<const WebApi_Artist> artist );

    album_art_data_ptr query( const GUID& p_what, abort_callback& p_abort ) override;

private:
    WebApi_Backend& waBackend_;
    std::unique_ptr<const WebApi_Track> track_;
    std::unique_ptr<const WebApi_Artist> artist_;
};

class AlbumArtExtractorSpotify : public album_art_extractor
{
public:
    AlbumArtExtractorSpotify();

    bool is_our_path( const char* p_path, const char* p_extension ) override;
    album_art_extractor_instance_ptr open( file_ptr p_filehint, const char* p_path, abort_callback& p_abort ) override;
};

} // namespace

namespace
{

AlbumArtExtractorInstanceSpotify::AlbumArtExtractorInstanceSpotify( std::unique_ptr<const WebApi_Track> track, std::unique_ptr<const WebApi_Artist> artist )
    : waBackend_( SpotifyInstance::Get().GetWebApi_Backend() )
    , track_( std::move( track ) )
    , artist_( std::move( artist ) )
{
    assert( track_ );
    assert( artist_ );
}

album_art_data_ptr AlbumArtExtractorInstanceSpotify::query( const GUID& p_what, abort_callback& p_abort )
{
    const auto imagePath = [&] {
        if ( p_what == album_art_ids::cover_front )
        {
            if ( track_->album->images.empty() )
            {
                throw exception_album_art_not_found();
            }

            return waBackend_.GetAlbumImage( track_->album->id, track_->album->images[0]->url, p_abort );
        }
        else if ( p_what == album_art_ids::artist )
        {
            if ( artist_->images.empty() )
            {
                throw exception_album_art_not_found();
            }

            return waBackend_.GetArtistImage( artist_->id, artist_->images[0]->url, p_abort );
        }
        else
        {
            throw exception_album_art_not_found();
        }
    }();

    pfc::string8_fast canPath;
    filesystem::g_get_canonical_path( imagePath.u8string().c_str(), canPath );

    file::ptr file;
    filesystem::g_open( file, canPath, filesystem::open_mode_read, p_abort );

    return album_art_data_impl::g_create( file.get_ptr(), (size_t)file->get_size_ex( p_abort ), p_abort );
}

AlbumArtExtractorSpotify::AlbumArtExtractorSpotify()
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

    auto& waBackend = SpotifyInstance::Get().GetWebApi_Backend();
    auto track = waBackend.GetTrack( object.id, p_abort );
    auto artist = waBackend.GetArtist( track->artists[0]->id, p_abort );
    if ( track->album->images.empty() && artist->images.empty() )
    {
        throw exception_album_art_not_found();
    }

    return ::fb2k::service_new<AlbumArtExtractorInstanceSpotify>( std::move( track ), std::move( artist ) );
}

} // namespace

namespace
{
static service_factory_single_t<AlbumArtExtractorSpotify> g_album_art_extractor;
}
