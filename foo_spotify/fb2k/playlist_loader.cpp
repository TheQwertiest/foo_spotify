#include <stdafx.h>

#include "playlist_loader.h"

#include <backend/spotify_object.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_objects_all.h>
#include <fb2k/file_info_filler.h>

#include <qwr/string_helpers.h>

using namespace std::literals::string_view_literals;

using namespace sptf;

namespace sptf
{

PlaylistLoaderSpotify::PlaylistLoaderSpotify()
    : waBackend_( WebApi_Backend::Instance() )
{
}

void PlaylistLoaderSpotify::open( const char* p_path, const service_ptr_t<file>& p_file, playlist_loader_callback::ptr p_callback, abort_callback& p_abort )
{
    const auto spotifyObject = [&] {
        try
        {
            return SpotifyObject( p_path );
        }
        catch ( const qwr::QwrException& )
        {
            throw exception_io_unsupported_format();
        }
    }();

    p_callback->on_progress( p_path );

    const auto tracks = [&] {
        if ( spotifyObject.type == "album" )
        {
            return waBackend_.GetTracksFromAlbum( spotifyObject.id, p_abort );
        }
        else if ( spotifyObject.type == "playlist" )
        {
            return waBackend_.GetTracksFromPlaylist( spotifyObject.id, p_abort );
        }
        else
        {
            std::vector<std::unique_ptr<const WebApi_Track>> tmp;
            tmp.emplace_back( waBackend_.GetTrack( spotifyObject.id, p_abort ) );
            return tmp;
        }
    }();

    const auto tracksMeta = waBackend_.GetMetaForTracks( tracks );
    for ( const auto& [track, trackMeta]: ranges::views::zip( tracks, tracksMeta ) )
    {
        file_info_impl f_info;

        const auto er = trackMeta.equal_range( "SPTF_LENGTH" );
        if ( er.first != er.second )
        {
            const auto [key, val] = *er.first;
            auto numOpt = qwr::string::GetNumber<uint32_t>( static_cast<std::string_view>( val ) );
            if ( numOpt && *numOpt )
            {
                f_info.set_length( *numOpt / 1000.0 );
            }
        }

        FillFileInfoWithMeta( trackMeta, f_info );

        metadb_handle_ptr f_handle;
        p_callback->handle_create( f_handle, make_playable_location( fmt::format( "spotify:track:{}", track->id ).c_str(), 0 ) );
        p_callback->on_entry_info( f_handle, playlist_loader_callback::entry_user_requested, filestats_invalid, f_info, false );
    }
}

void PlaylistLoaderSpotify::write( const char* p_path, const service_ptr_t<file>& p_file, metadb_handle_list_cref p_data, abort_callback& p_abort )
{
    throw pfc::exception_not_implemented();
}

const char* PlaylistLoaderSpotify::get_extension()
{
    return "spotify";
}

bool PlaylistLoaderSpotify::can_write()
{
    return false;
}

bool PlaylistLoaderSpotify::is_our_content_type( const char* p_content_type )
{
    constexpr auto mime = "text/html; charset=UTF-8"sv;
    return ( mime == p_content_type );
}

bool PlaylistLoaderSpotify::is_associatable()
{
    return false;
}

} // namespace sptf

namespace
{

playlist_loader_factory_t<PlaylistLoaderSpotify> g_playlist_loader;

} // namespace
