#include <stdafx.h>

#include <fb2k/playlist_loader.h>
#include <ui/ui_input_box.h>

using namespace sptf;

namespace
{
class OnProcessLocationsNotify_InsertHandles
    : public process_locations_notify
{
public:
    OnProcessLocationsNotify_InsertHandles( int playlistIdx, UINT baseIdx, bool shouldSelect );

    void on_completion( metadb_handle_list_cref items ) override;
    void on_aborted() override;

private:
    int playlistIdx_;
    size_t baseIdx_;
    bool shouldSelect_;
};
}

namespace
{
OnProcessLocationsNotify_InsertHandles::OnProcessLocationsNotify_InsertHandles( int playlistIdx, UINT baseIdx, bool shouldSelect )
    : playlistIdx_( playlistIdx )
    , baseIdx_( baseIdx )
    , shouldSelect_( shouldSelect )
{
}

void OnProcessLocationsNotify_InsertHandles::on_completion( metadb_handle_list_cref items )
{
    auto api = playlist_manager::get();
    const size_t adjustedPlIdx = ( playlistIdx_ == -1 ? api->get_active_playlist() : playlistIdx_ );
    if ( adjustedPlIdx >= api->get_playlist_count()
         || ( api->playlist_lock_get_filter_mask( adjustedPlIdx ) & playlist_lock::filter_add ) )
    {
        return;
    }

    pfc::bit_array_val selection( shouldSelect_ );
    api->playlist_insert_items( adjustedPlIdx, baseIdx_, items, selection );
    if ( shouldSelect_ )
    {
        api->set_active_playlist( adjustedPlIdx );
        api->playlist_set_focus_item( adjustedPlIdx, baseIdx_ );
    }
}

void OnProcessLocationsNotify_InsertHandles::on_aborted()
{
}
}

namespace
{

class MainMenuCommandsSpotify : public mainmenu_commands
{
public:
    t_uint32 get_command_count() override
    {
        return 1;
    }
    GUID get_command( t_uint32 p_index ) override
    {
        switch ( p_index )
        {
        case 0:
            return sptf::guid::mainmenu_cmd_add;
        default:
            uBugCheck();
        }
    }
    void get_name( t_uint32 p_index, pfc::string_base& p_out ) override
    {
        switch ( p_index )
        {
        case 0:
            p_out = "Add URL/URI...";
            return;
        default:
            uBugCheck();
        }
    }
    bool get_description( t_uint32 /* p_index */, pfc::string_base& p_out ) override
    {
        p_out = "Adds tracks from specified URL or URI to the current playlist.";
        return true;
    }
    GUID get_parent() override
    {
        return sptf::guid::mainmenu_group_file;
    }
    void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback ) override
    {
        const auto hFb2k = core_api::get_main_window();
        const auto path = [hFb2k] {
            if ( modal_dialog_scope::can_create() )
            {
                modal_dialog_scope scope( hFb2k );

                // TODO: replace with proper dialog:
                // - drop-down history
                // - cleaner UI
                sptf::ui::CInputBox dlg( "Enter Spotify URL or URI", "Add URL/URI", "" );
                int status = dlg.DoModal( hFb2k );
                if ( status == IDOK )
                {
                    return dlg.GetValue();
                }
            }
            return std::string();
        }();
        if ( path.empty() )
        {
            return;
        }

        const auto plIdx = playlist_manager::get()->get_active_playlist();
        const auto plCount = playlist_manager::get()->playlist_get_item_count( plIdx );

        pfc::string_list_impl locations;
        locations.add_item( path.c_str() );

        playlist_incoming_item_filter_v2::get()->process_locations_async(
            locations,
            playlist_incoming_item_filter_v2::op_flag_no_filter | playlist_incoming_item_filter_v2::op_flag_delay_ui,
            "",
            "",
            hFb2k,
            fb2k::service_new<OnProcessLocationsNotify_InsertHandles>( plIdx, plCount, true ) );
    }
    bool get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags ) override
    {
        get_name( p_index, p_out );
        p_flags = sort_priority_dontcare;
        return true;
    }
};

} // namespace

namespace
{

mainmenu_group_popup_factory g_mainmenu_group(
    sptf::guid::mainmenu_group_file, mainmenu_groups::file, static_cast<t_uint32>( mainmenu_commands::sort_priority_dontcare ), SPTF_NAME );

mainmenu_commands_factory_t<MainMenuCommandsSpotify> g_mainmenu_commands;

} // namespace
