#include <stdafx.h>

#include "ui_pref_tab_manager.h"

#include <fb2k/config.h>
#include <ui/ui_pref_tab_auth.h>
#include <ui/ui_pref_tab_playback.h>

#include <component_urls.h>

namespace
{

using namespace sptf;

class preferences_page_impl
    : public preferences_page_v3
{
public:
    const char* get_name() override
    {
        return SPTF_NAME;
    }

    GUID get_guid() override
    {
        return guid::preferences;
    }

    GUID get_parent_guid() override
    {
        return preferences_page::guid_tools;
    }

    bool get_help_url( pfc::string_base& p_out ) override
    {
        p_out << qwr::unicode::ToU8( std::wstring_view( url::homepage ) );
        return true;
    }

    preferences_page_instance::ptr instantiate( HWND parent, preferences_page_callback::ptr callback ) override
    {
        auto p = ::fb2k::service_new<ui::PreferenceTabManager>( callback );
        p->Create( parent );
        return p;
    }
};

preferences_page_factory_t<preferences_page_impl> g_pref;

} // namespace

namespace sptf::ui
{

using namespace config;

PreferenceTabManager::PreferenceTabManager( preferences_page_callback::ptr callback )
    : callback_( callback )
    , tabs_( { std::make_unique<PreferenceTabAuth>( this ),
               std::make_unique<PreferenceTabPlayback>( this ) } )
{
}

void PreferenceTabManager::OnDataChanged()
{
    callback_->on_state_changed();
}

HWND PreferenceTabManager::get_wnd()
{
    return m_hWnd;
}

t_uint32 PreferenceTabManager::get_state()
{
    uint32_t state = preferences_state::resettable;
    for ( auto& tab: tabs_ )
    {
        state |= tab->get_state();
    }

    return state;
}

void PreferenceTabManager::apply()
{
    for ( auto& tab: tabs_ )
    {
        tab->apply();
    }

    OnDataChanged();
}

void PreferenceTabManager::reset()
{
    for ( auto& tab: tabs_ )
    {
        tab->reset();
    }

    OnDataChanged();
}

BOOL PreferenceTabManager::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    cTabs_ = GetDlgItem( IDC_TAB_HOST );

    for ( const auto& [i, pTab]: ranges::views::enumerate( tabs_ ) )
    {
        cTabs_.InsertItem( i, pTab->Name() );
    }

    cTabs_.SetCurSel( activeTabIdx_ );
    CreateTab();

    return TRUE; // set focus to default control
}

void PreferenceTabManager::OnParentNotify( UINT message, UINT nChildID, LPARAM lParam )
{
    if ( WM_DESTROY == message && pcCurTab_ && reinterpret_cast<HWND>( lParam ) == static_cast<HWND>( *pcCurTab_ ) )
    {
        pcCurTab_ = nullptr;
    }
}

LRESULT PreferenceTabManager::OnSelectionChanged( LPNMHDR pNmhdr )
{
    activeTabIdx_ = TabCtrl_GetCurSel( GetDlgItem( IDC_TAB_HOST ) );
    CreateTab();

    return 0;
}

LRESULT PreferenceTabManager::OnWindowPosChanged( UINT, WPARAM, LPARAM lp, BOOL& bHandled )
{
    auto lpwp = reinterpret_cast<LPWINDOWPOS>( lp );
    // Temporary workaround for various bugs that occur due to foobar2000 1.0+
    // having a dislike for destroying preference pages
    if ( lpwp->flags & SWP_HIDEWINDOW )
    {
        DestroyTab();
    }
    else if ( lpwp->flags & SWP_SHOWWINDOW && !pcCurTab_ )
    {
        CreateTab();
    }

    bHandled = FALSE;

    return 0;
}

void PreferenceTabManager::CreateTab()
{
    DestroyTab();

    RECT tabRc;

    cTabs_.GetWindowRect( &tabRc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&tabRc, 2 );

    cTabs_.AdjustRect( FALSE, &tabRc );

    if ( activeTabIdx_ >= tabs_.size() )
    {
        activeTabIdx_ = 0;
    }

    auto& pCurTab = tabs_[activeTabIdx_];
    pcCurTab_ = &pCurTab->Dialog();
    pCurTab->CreateTab( m_hWnd );

    EnableThemeDialogTexture( *pcCurTab_, ETDT_ENABLETAB );

    pcCurTab_->SetWindowPos( nullptr, tabRc.left, tabRc.top, tabRc.right - tabRc.left, tabRc.bottom - tabRc.top, SWP_NOZORDER );
    cTabs_.SetWindowPos( HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

    pcCurTab_->ShowWindow( SW_SHOWNORMAL );
}

void PreferenceTabManager::DestroyTab()
{
    if ( pcCurTab_ && *pcCurTab_ )
    {
        pcCurTab_->ShowWindow( SW_HIDE );
        pcCurTab_->DestroyWindow();
    }
}

} // namespace sptf::ui
