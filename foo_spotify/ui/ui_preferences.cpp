#include <stdafx.h>

#include "ui_preferences.h"

#include <backend/libspotify_backend.h>
#include <backend/webapi_auth.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_user.h>

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
        p_out << qwr::unicode::ToU8( std::wstring( url::homepage ) );
        return true;
    }

    preferences_page_instance::ptr instantiate( HWND parent, preferences_page_callback::ptr callback ) override
    {
        auto p = fb2k::service_new<ui::Preferences>( callback );
        p->Create( parent );
        return p;
    }
};

preferences_page_factory_t<preferences_page_impl> g_pref;

} // namespace

namespace sptf::ui
{

Preferences::Preferences( preferences_page_callback::ptr callback )
{
}

Preferences::~Preferences()
{
}

HWND Preferences::get_wnd()
{
    return m_hWnd;
}

t_uint32 Preferences::get_state()
{
    return 0;
}

void Preferences::apply()
{
    return;
}

void Preferences::reset()
{
    return;
}

BOOL Preferences::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    btnLibSpotify_ = GetDlgItem( IDC_BTN_LOGIN_LIBSPOTIFY );
    btnWebApi_ = GetDlgItem( IDC_BTN_LOGIN_WEBAPI );

    textLibSpotify_ = GetDlgItem( IDC_STATIC_LIBSPOTIFY_STATUS );
    textWebApi_ = GetDlgItem( IDC_STATIC_WEBAPI_STATUS );

    // TODO: add proepr abortable
    auto dummy = abort_callback_dummy();
    isLibSpotifyLoggedIn_ = LibSpotify_Backend::Instance().Relogin( dummy );
    UpdateLibSpotifyStatus();

    isWebApiLoggedIn_ = WebApi_Backend::Instance().GetAuthorizer().IsAuthenticated();
    UpdateWebApiStatus();

    return TRUE; // set focus to default control
}

void Preferences::OnDestroy()
{
}

void Preferences::OnLibSpotifyLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    (void)uNotifyCode;
    (void)nID;
    (void)wndCtl;

    // TODO: add proepr abortable
    auto dummy = abort_callback_dummy();
    if ( !isLibSpotifyLoggedIn_ )
    {
        isLibSpotifyLoggedIn_ = LibSpotify_Backend::Instance().LoginWithUI( m_hWnd, dummy );
    }
    else
    {
        LibSpotify_Backend::Instance().LogoutAndForget( dummy );
        isLibSpotifyLoggedIn_ = false;
    }

    UpdateLibSpotifyStatus();
}

void Preferences::OnWebApiLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    (void)uNotifyCode;
    (void)nID;
    (void)wndCtl;

    // TODO: add proepr abortable
    auto dummy = abort_callback_dummy();
    if ( !isWebApiLoggedIn_ )
    {
        WebApi_Backend::Instance().GetAuthorizer().AuthenticateClean();
        isWebApiLoggedIn_ = true;
    }
    else
    {
        WebApi_Backend::Instance().GetAuthorizer().ClearAuth();
        isWebApiLoggedIn_ = false;
    }

    UpdateWebApiStatus();
}

void Preferences::UpdateLibSpotifyStatus()
{
    if ( isLibSpotifyLoggedIn_ )
    {
        btnLibSpotify_.SetWindowText( L"Log out" );
        textLibSpotify_.SetWindowText( qwr::unicode::ToWide( fmt::format( "status: logged in as `{}`", LibSpotify_Backend::Instance().GetLoggedInUserName() ) ).c_str() );
    }
    else
    {
        btnLibSpotify_.SetWindowText( L"Log in" );
        textLibSpotify_.SetWindowText( L"status: logged out" );
    }
}

void Preferences::UpdateWebApiStatus()
{
    if ( isWebApiLoggedIn_ )
    {
        btnWebApi_.SetWindowText( L"Log out" );

        // TODO: add proepr abortable
        const auto username = [] {
            auto dummy = abort_callback_dummy();
            try
            {
                const auto pUser = WebApi_Backend::Instance().GetUser( dummy );
                return pUser->display_name.value_or( pUser->uri );
            }
            catch ( const std::exception& e )
            {
                return fmt::format( "<error: user name could not be fetched - {}>", e.what() );
            }
        }();

        textWebApi_.SetWindowText( qwr::unicode::ToWide( fmt::format( "status: logged in as `{}`", username ) ).c_str() );
    }
    else
    {
        btnWebApi_.SetWindowText( L"Log in" );
        textWebApi_.SetWindowText( L"status: logged out" );
    }
}

} // namespace sptf::ui