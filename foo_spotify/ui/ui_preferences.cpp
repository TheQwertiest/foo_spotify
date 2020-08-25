#include <stdafx.h>

#include "ui_preferences.h"

#include <backend/libspotify_backend.h>
#include <backend/spotify_instance.h>
#include <backend/webapi_auth.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_user.h>

#include <component_urls.h>

#include <qwr/abort_callback.h>
#include <qwr/error_popup.h>

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
        auto p = ::fb2k::service_new<ui::Preferences>( callback );
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
    (void)callback;
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

    pStatusThread_ = std::make_unique<std::thread>( [this] {
        qwr::TimedAbortCallback tac;

        const auto lsStatus = SpotifyInstance::Get().GetLibSpotify_Backend().Relogin( tac );
        const auto waStatus = [&] {
            auto& auth = SpotifyInstance::Get().GetWebApi_Backend().GetAuthorizer();
            if ( !auth.IsAuthenticated() )
            {
                return false;
            }
            try
            {
                auth.AuthenticateWithRefreshToken( tac );
                return true;
            }
            catch ( const std::exception& )
            {
                return false;
            }
        }();

        ::PostMessage( m_hWnd, kOnStatusUpdateFinish, WPARAM( lsStatus ), LPARAM( waStatus ) );
    } );

    UpdateLibSpotifyUi();
    UpdateWebApiUi();

    return TRUE; // set focus to default control
}

void Preferences::OnDestroy()
{
    auto& auth = SpotifyInstance::Get().GetWebApi_Backend().GetAuthorizer();
    if ( webApiStatus_ == LoginStatus::logged_in )
    {
        auth.AuthenticateClean_Cleanup();
    }
    else
    {
        auth.CancelAuth();
    }

    if ( pStatusThread_ )
    {
        if ( pStatusThread_->joinable() )
        {
            pStatusThread_->join();
        }
        pStatusThread_.reset();
    }
}

HBRUSH Preferences::OnCtlColorStatic( CDCHandle dc, CStatic wndStatic )
{
    const auto id = wndStatic.GetDlgCtrlID();
    if ( id == IDC_STATIC_LIBSPOTIFY_STATUS
         || id == IDC_STATIC_WEBAPI_STATUS )
    {
        const auto getColour = []( auto loginStatus ) {
            switch ( loginStatus )
            {
            case LoginStatus::logged_out:
            {
                return RGB( 0x8B, 0x00, 0x00 );
            }
            case LoginStatus::logged_in:
            {
                return RGB( 0x2F, 0x4F, 0x4f );
            }
            default:
                return RGB( 0xFF, 0x7F, 0x50 );
            }
        };

        ::SetTextColor( dc, getColour( id == IDC_STATIC_LIBSPOTIFY_STATUS ? libSpotifyStatus_ : webApiStatus_ ) );
        ::SetBkColor( dc, GetSysColor( COLOR_BTNFACE ) );
        return (HBRUSH)GetStockObject( HOLLOW_BRUSH );
    }

    SetMsgHandled( FALSE );

    return 0;
}

void Preferences::OnLibSpotifyLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    (void)uNotifyCode;
    (void)nID;
    (void)wndCtl;

    qwr::TimedAbortCallback tac;

    auto& lsBackend = SpotifyInstance::Get().GetLibSpotify_Backend();
    if ( libSpotifyStatus_ == LoginStatus::logged_out )
    {
        libSpotifyStatus_ = ( lsBackend.LoginWithUI( m_hWnd, tac ) ? LoginStatus::logged_in : LoginStatus::logged_out );
    }
    else
    {
        lsBackend.LogoutAndForget( tac );
        libSpotifyStatus_ = LoginStatus::logged_out;
    }

    UpdateLibSpotifyUi();
}

void Preferences::OnWebApiLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    (void)uNotifyCode;
    (void)nID;
    (void)wndCtl;

    auto& auth = SpotifyInstance::Get().GetWebApi_Backend().GetAuthorizer();
    if ( webApiStatus_ == LoginStatus::logged_out )
    {
        // this operation is async
        webApiStatus_ = LoginStatus::login_in_progress;
        UpdateWebApiUi();

        try
        {
            auth.AuthenticateClean( [&] { ::PostMessage( m_hWnd, kOnWebApiLoginResponse, 0, 0 ); } );
        }
        catch ( const std::exception& e )
        {
            qwr::ReportErrorWithPopup( fmt::format( "WebAPI login failed:\n{}", e.what() ), SPTF_NAME );

            webApiStatus_ = LoginStatus::logged_out;
            UpdateWebApiUi();
        }
    }
    else
    {
        auth.ClearAuth();

        webApiStatus_ = LoginStatus::logged_out;
        UpdateWebApiUi();
    }
}

LRESULT Preferences::OnWebApiLoginResponse( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    (void)uMsg;
    (void)wParam;
    (void)lParam;

    bHandled = TRUE;

    auto& auth = SpotifyInstance::Get().GetWebApi_Backend().GetAuthorizer();

    webApiStatus_ = ( auth.IsAuthenticated() ? LoginStatus::logged_in : LoginStatus::logged_out );
    UpdateWebApiUi();

    return 0;
}

LRESULT Preferences::OnStatusUpdateFinish( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    (void)uMsg;

    bHandled = TRUE;

    libSpotifyStatus_ = ( wParam ? LoginStatus::logged_in : LoginStatus::logged_out );
    webApiStatus_ = ( lParam ? LoginStatus::logged_in : LoginStatus::logged_out );
    UpdateLibSpotifyUi();
    UpdateWebApiUi();

    return 0;
}

void Preferences::UpdateLibSpotifyUi()
{
    const auto getUsername = [] {
        return SpotifyInstance::Get().GetLibSpotify_Backend().GetLoggedInUserName();
    };
    UpdateBackendUi( libSpotifyStatus_, btnLibSpotify_, textLibSpotify_, getUsername );
}

void Preferences::UpdateWebApiUi()
{
    const auto getUsername = [] {
        qwr::TimedAbortCallback tac;
        try
        {
            auto& waBackend = SpotifyInstance::Get().GetWebApi_Backend();
            const auto pUser = waBackend.GetUser( tac );
            return pUser->display_name.value_or( pUser->uri );
        }
        catch ( const std::exception& e )
        {
            return fmt::format( "<error: user name could not be fetched - {}>", e.what() );
        }
    };
    UpdateBackendUi( webApiStatus_, btnWebApi_, textWebApi_, getUsername );
}

void Preferences::UpdateBackendUi( LoginStatus loginStatus, CButton& btn, CStatic& text, std::function<std::string()> getUserNameFn )
{
    Invalidate(); ///< needed to clear static text

    switch ( loginStatus )
    {
    case LoginStatus::fetching_login_status:
    {
        if ( btn.IsWindowEnabled() )
        {
            btn.EnableWindow( FALSE );
        }
        btn.SetWindowText( L"Log in" );
        text.SetWindowText( L"status: verifying login status" );
        break;
    }
    case LoginStatus::logged_out:
    {
        if ( !btn.IsWindowEnabled() )
        {
            btn.EnableWindow( TRUE );
        }
        btn.SetWindowText( L"Log in" );
        text.SetWindowText( L"status: logged out" );
        break;
    }
    case LoginStatus::login_in_progress:
    {
        if ( btn.IsWindowEnabled() )
        {
            btn.EnableWindow( FALSE );
        }
        btn.SetWindowText( L"Log in" );
        text.SetWindowText( L"status: login in progress..." );
        break;
    }
    case LoginStatus::logged_in:
    {
        if ( !btn.IsWindowEnabled() )
        {
            btn.EnableWindow( TRUE );
        }
        btn.SetWindowText( L"Log out" );
        text.SetWindowText( qwr::unicode::ToWide( fmt::format( "status: logged in as `{}`", getUserNameFn() ) ).c_str() );
        break;
    }
    default:
        break;
    }
}

} // namespace sptf::ui