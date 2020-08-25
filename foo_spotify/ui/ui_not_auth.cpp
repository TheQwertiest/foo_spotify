#include <stdafx.h>

#include "ui_not_auth.h"

#include <qwr/hook_handler.h>

namespace sptf::ui
{

NotAuth::NotAuth( std::function<void()> onDestroyCallback )
    : onDestroyCallback_( onDestroyCallback )
{
}

BOOL NotAuth::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    hookId_ = qwr::HookHandler::GetInstance().RegisterHook(
        [hParent = m_hWnd]( int code, WPARAM wParam, LPARAM lParam ) {
            GetMsgProc( code, wParam, lParam, hParent );
        } );

    SetWindowText( TEXT( SPTF_NAME ) );

    CStatic text( GetDlgItem( IDC_STATIC_NOT_AUTH ) );
    text.SetWindowText( fmt::format(
                            L"{} ({}) requires Spotify Premium account.\n"
                            L"Open the corresponding foobar2000 Preferences page to authorize.",
                            TEXT( SPTF_NAME ),
                            TEXT( SPTF_UNDERSCORE_NAME ) )
                            .c_str() );

    return TRUE; // set focus to default control
}

void NotAuth::OnDestroy()
{
    if ( hookId_ )
    {
        qwr::HookHandler::GetInstance().UnregisterHook( hookId_ );
        hookId_ = 0;
    }
}

void NotAuth::OnCloseCmd( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    DestroyWindow();
}

void NotAuth::OnOpenPrefClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    ::fb2k::inMainThread( [] { ui_control::get()->show_preferences( sptf::guid::preferences ); } );
    DestroyWindow();
}

void NotAuth::OnFinalMessage( _In_ HWND /*hWnd*/ )
{
    onDestroyCallback_();
    delete this;
}

void NotAuth::GetMsgProc( int, WPARAM, LPARAM lParam, HWND hParent )
{
    if ( auto pMsg = reinterpret_cast<LPMSG>( lParam );
         pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST )
    { // Only react to keypress events
        HWND hWndFocus = ::GetFocus();
        if ( hWndFocus != nullptr && ( ( hParent == hWndFocus ) || ::IsChild( hParent, hWndFocus ) ) )
        {
            if ( ::IsDialogMessage( hParent, pMsg ) )
            {
                pMsg->message = WM_NULL;
            }
        }
    }
}

NotAuthHandler& NotAuthHandler::Get()
{
    static NotAuthHandler nah;
    return nah;
}

void NotAuthHandler::ShowDialog()
{
    assert( core_api::is_main_thread() );
    if ( !pNotAuth_ )
    {
        pNotAuth_ = new ui::NotAuth( [&] {
            assert( core_api::is_main_thread() );
            pNotAuth_ = nullptr;
        } );
        if ( !pNotAuth_->Create( core_api::get_main_window() ) )
        {
            delete pNotAuth_;
            pNotAuth_ = nullptr;
            return;
        }

        pNotAuth_->SetActiveWindow();
        pNotAuth_->CenterWindow( core_api::get_main_window() );
        pNotAuth_->ShowWindow( SW_SHOW );
    }
    else
    {
        pNotAuth_->SetActiveWindow();
        pNotAuth_->ShowWindow( SW_SHOW );
    }
}

void NotAuthHandler::CloseDialog()
{
    assert( core_api::is_main_thread() );
    if ( pNotAuth_ )
    {
        pNotAuth_->DestroyWindow();
    }
}

} // namespace sptf::ui