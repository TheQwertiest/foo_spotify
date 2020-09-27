#pragma once

#include <resource.h>

namespace sptf::ui
{

class Preferences
    : public CDialogImpl<Preferences>
    , public preferences_page_instance
{
public:
    enum
    {
        IDD = IDD_PREFERENCES
    };

    BEGIN_MSG_MAP( Preferences )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_DESTROY( OnDestroy )
        COMMAND_HANDLER_EX( IDC_BTN_LOGIN_LIBSPOTIFY, BN_CLICKED, OnLibSpotifyLoginClick )
        COMMAND_HANDLER_EX( IDC_BTN_LOGIN_WEBAPI, BN_CLICKED, OnWebApiLoginClick )
    END_MSG_MAP()

public:
    Preferences( preferences_page_callback::ptr callback );
    ~Preferences() override;

    // preferences_page_instance
    HWND get_wnd() override;
    t_uint32 get_state() override;
    void apply() override;
    void reset() override;

private:

    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnDestroy();

    void OnLibSpotifyLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnWebApiLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl );

private:
    void UpdateLibSpotifyStatus();
    void UpdateWebApiStatus();

private:
    preferences_page_callback::ptr callback_;

    CButton btnLibSpotify_;
    CButton btnWebApi_;

    CStatic textLibSpotify_;
    CStatic textWebApi_;

    bool isLibSpotifyLoggedIn_ = false;
    bool isWebApiLoggedIn_ = false;
};

} // namespace sptf::ui
