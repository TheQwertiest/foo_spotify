#include <stdafx.h>

#include <backend/spotify_instance.h>
#include <backend/spotify_object.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_media_objects.h>

#include <qwr/abort_callback.h>
#include <qwr/thread_pool.h>

using namespace sptf;

namespace
{

class PlaylistCallbackSpotify : public playlist_callback_static
{
public:
    unsigned get_flags() override;
    void on_default_format_changed() override
    {
    }
    void on_item_ensure_visible( t_size p_playlist, t_size p_idx ) override
    {
    }
    void on_item_focus_change( t_size p_playlist, t_size p_from, t_size p_to ) override
    {
    }
    void on_items_added( t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection ) override
    {
    }
    void on_items_removing( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) override
    {
    }
    void on_items_removed( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) override
    {
    }
    void on_items_reordered( t_size p_playlist, const t_size* p_order, t_size p_count ) override
    {
    }
    void on_items_replaced( t_size p_playlist, const pfc::bit_array& p_mask, const pfc::list_base_const_t<t_on_items_replaced_entry>& p_data ) override
    {
    }
    void on_items_selection_change( t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state ) override
    {
    }
    void on_items_modified( t_size p_playlist, const pfc::bit_array& p_mask ) override
    {
    }
    void on_items_modified_fromplayback( t_size p_playlist, const pfc::bit_array& p_mask, play_control::t_display_level p_level ) override
    {
    }
    void on_playback_order_changed( t_size p_new_index ) override
    {
    }
    void on_playlist_activate( t_size p_old, t_size p_new ) override;
    void on_playlist_created( t_size p_index, const char* p_name, t_size p_name_len ) override
    {
    }
    void on_playlist_locked( t_size p_playlist, bool p_locked ) override
    {
    }
    void on_playlist_renamed( t_size p_index, const char* p_new_name, t_size p_new_name_len ) override
    {
    }
    void on_playlists_removing( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) override
    {
    }
    void on_playlists_removed( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) override
    {
    }
    void on_playlists_reorder( const t_size* p_order, t_size p_count ) override
    {
    }
};

} // namespace

namespace
{

unsigned PlaylistCallbackSpotify::get_flags()
{
    return flag_on_playlist_activate;
}

void PlaylistCallbackSpotify::on_playlist_activate( t_size p_old, t_size p_new )
{
    if ( p_old == p_new )
    {
        return;
    }

    metadb_handle_list items;
    playlist_manager::get()->playlist_get_all_items( p_new, items );

    std::vector<std::string> trackIds;
    for ( const auto& pMeta: qwr::pfc_x::Make_Stl_CRef( items ) )
    {
        const char* path = pMeta->get_location().get_path();
        if ( !SpotifyFilteredTrack::IsValid( path, false ) )
        {
            continue;
        }
        trackIds.emplace_back( SpotifyFilteredTrack::Parse( path ).Id() );
    }

    if ( trackIds.empty() )
    {
        return;
    }

    auto& threadPool = SpotifyInstance::Get().GetThreadPool();
    threadPool.AddTask( [trackIds = std::move( trackIds )] {
        try
        {
            auto& waBackend = SpotifyInstance::Get().GetWebApi_Backend();

            // pre-cache tracks
            qwr::TimedAbortCallback tac1;
            const auto tracks = waBackend.GetTracks( trackIds, tac1 );

            const auto artistIds =
                tracks
                | ranges::views::transform( []( const auto& pTrack ) -> std::string { return pTrack->artists[0]->id; } )
                | ranges::to_vector;

            // pre-cache artists
            qwr::TimedAbortCallback tac2;
            waBackend.RefreshCacheForArtists( artistIds, tac2 );
        }
        catch ( const std::exception& e )
        {
            FB2K_console_formatter() << SPTF_UNDERSCORE_NAME " (error):\n"
                                     << "Failed to refresh tracks:\n"
                                     << e.what();
        }
    } );
}

} // namespace

namespace
{

service_factory_single_t<PlaylistCallbackSpotify> g_playlist_callback_static;

}
