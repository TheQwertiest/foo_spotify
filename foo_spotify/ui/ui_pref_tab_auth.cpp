#include <stdafx.h>

#include "ui_pref_tab_auth.h"

#include <backend/libspotify_backend.h>
#include <backend/spotify_instance.h>
#include <backend/webapi_auth.h>
#include <backend/webapi_auth_scopes.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_user.h>
#include <fb2k/config.h>
#include <ui/ui_pref_tab_manager.h>

#include <component_urls.h>

#include <qwr/abort_callback.h>
#include <qwr/error_popup.h>
#include <qwr/string_helpers.h>

namespace sptf::ui
{

PreferenceTabAuth::PreferenceTabAuth( PreferenceTabManager* pParent )
    : pParent_( pParent )
    , ddxOptions_( {} )
{
}

PreferenceTabAuth::~PreferenceTabAuth()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Revert();
    }
}

HWND PreferenceTabAuth::CreateTab( HWND hParent )
{
    return Create( hParent );
}

CDialogImplBase& PreferenceTabAuth::Dialog()
{
    return *this;
}

const wchar_t* PreferenceTabAuth::Name() const
{
    return L"Authorization";
}

t_uint32 PreferenceTabAuth::get_state()
{
    const bool hasChanged =
        ddxOptions_.cend() != std::find_if( ddxOptions_.cbegin(), ddxOptions_.cend(), []( const auto& ddxOpt ) {
            return ddxOpt->Option().HasChanged();
        } );

    return ( preferences_state::resettable | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabAuth::apply()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Apply();
    }

    pParent_->OnDataChanged();
}

void PreferenceTabAuth::reset()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().ResetToDefault();
    }
    UpdateUiFromCfg();

    pParent_->OnDataChanged();
}

BOOL PreferenceTabAuth::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    btnLibSpotify_ = GetDlgItem( IDC_BTN_LOGIN_LIBSPOTIFY );
    btnWebApi_ = GetDlgItem( IDC_BTN_LOGIN_WEBAPI );

    textLibSpotify_ = GetDlgItem( IDC_STATIC_LIBSPOTIFY_STATUS );
    textWebApi_ = GetDlgItem( IDC_STATIC_WEBAPI_STATUS );

    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Ddx().SetHwnd( m_hWnd );
    }
    UpdateUiFromCfg();

    pStatusThread_ = std::make_unique<std::thread>( [this] {
        const auto lsStatus = [] {
            qwr::TimedAbortCallback tac( fmt::format( "{}: {}", SPTF_UNDERSCORE_NAME, "LibSpotify relogin" ) );
            return SpotifyInstance::Get().GetLibSpotify_Backend().Relogin( tac );
        }();
        const auto waStatus = [&] {
            auto& auth = SpotifyInstance::Get().GetWebApi_Backend().GetAuthorizer();
            if ( !auth.HasRefreshToken() )
            {
                return false;
            }
            try
            {
                qwr::TimedAbortCallback tac( fmt::format( "{}: {}", SPTF_UNDERSCORE_NAME, "WebApi relogin" ) );
                auth.UpdateRefreshToken( tac );
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

void PreferenceTabAuth::OnDestroy()
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

HBRUSH PreferenceTabAuth::OnCtlColorStatic( CDCHandle dc, CStatic wndStatic )
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
        ::SetBkColor( dc, GetSysColor( COLOR_WINDOW ) );
        return (HBRUSH)GetStockObject( HOLLOW_BRUSH );
    }

    SetMsgHandled( FALSE );

    return 0;
}

void PreferenceTabAuth::OnDdxChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    auto it = std::find_if( ddxOptions_.begin(), ddxOptions_.end(), [nID]( auto& val ) {
        return val->Ddx().IsMatchingId( nID );
    } );

    if ( ddxOptions_.end() != it )
    {
        ( *it )->Ddx().ReadFromUi();
        pParent_->OnDataChanged();
    }
}

void PreferenceTabAuth::OnLibSpotifyLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    (void)uNotifyCode;
    (void)nID;
    (void)wndCtl;

    auto& lsBackend = SpotifyInstance::Get().GetLibSpotify_Backend();
    if ( libSpotifyStatus_ == LoginStatus::logged_out )
    {
        libSpotifyStatus_ = ( lsBackend.LoginWithUI( m_hWnd ) ? LoginStatus::logged_in : LoginStatus::logged_out );
    }
    else
    {
        qwr::TimedAbortCallback tac( fmt::format( "{}: {}", SPTF_UNDERSCORE_NAME, "LibSpotify logout" ) );
        lsBackend.LogoutAndForget( tac );
        libSpotifyStatus_ = LoginStatus::logged_out;
    }

    UpdateLibSpotifyUi();
}

void PreferenceTabAuth::OnWebApiLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl )
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
            const auto authScopes = qwr::unicode::ToWide( static_cast<std::string>( config::webapi_auth_scopes ) );
            const auto authScopesSplit = qwr::string::Split<wchar_t>( authScopes, L' ' );
            // TODO: remove me
            WebApiAuthScopes tmp( authScopesSplit );
            tmp.user_read_private = true;
            auth.AuthenticateClean( tmp, [&] { ::PostMessage( m_hWnd, kOnWebApiLoginResponse, 0, 0 ); } );
        }
        catch ( const std::exception& e )
        {
            qwr::ReportErrorWithPopup( SPTF_UNDERSCORE_NAME, fmt::format( "WebAPI login failed:\n{}", e.what() ) );

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

LRESULT PreferenceTabAuth::OnWebApiLoginResponse( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    (void)uMsg;
    (void)wParam;
    (void)lParam;

    bHandled = TRUE;

    auto& auth = SpotifyInstance::Get().GetWebApi_Backend().GetAuthorizer();

    webApiStatus_ = ( auth.HasRefreshToken() ? LoginStatus::logged_in : LoginStatus::logged_out );
    UpdateWebApiUi();

    return 0;
}

LRESULT PreferenceTabAuth::OnStatusUpdateFinish( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    (void)uMsg;

    bHandled = TRUE;

    libSpotifyStatus_ = ( wParam ? LoginStatus::logged_in : LoginStatus::logged_out );
    webApiStatus_ = ( lParam ? LoginStatus::logged_in : LoginStatus::logged_out );
    UpdateLibSpotifyUi();
    UpdateWebApiUi();

    return 0;
}

void PreferenceTabAuth::UpdateUiFromCfg()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Ddx().WriteToUi();
    }
}

void PreferenceTabAuth::UpdateLibSpotifyUi()
{
    const auto getUsername = [] {
        return SpotifyInstance::Get().GetLibSpotify_Backend().GetLoggedInUserName();
    };
    UpdateBackendUi( libSpotifyStatus_, btnLibSpotify_, textLibSpotify_, getUsername );
}

void PreferenceTabAuth::UpdateWebApiUi()
{
    const auto getUsername = [] {
        qwr::TimedAbortCallback tac( fmt::format( "{}: {}", SPTF_UNDERSCORE_NAME, "WebApi get username" ) );
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

void PreferenceTabAuth::UpdateBackendUi( LoginStatus loginStatus, CButton& btn, CStatic& text, std::function<std::string()> getUserNameFn )
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
