#pragma once

#include <fb2k/config.h>

#include <resource.h>

#include <qwr/fb2k_config_ui_option.h>
#include <qwr/macros.h>
#include <qwr/ui_ddx_option.h>

#include <thread>

namespace sptf::ui
{

class Preferences
    : public CDialogImpl<Preferences>
    , public preferences_page_instance
{
private:
    enum class LoginStatus
    {
        fetching_login_status,
        logged_out,
        login_in_progress,
        logged_in,
    };

public:
    enum
    {
        IDD = IDD_PREFERENCES
    };

    Preferences( preferences_page_callback::ptr callback );
    ~Preferences() override;

    // preferences_page_instance
    HWND get_wnd() override;
    t_uint32 get_state() override;
    void apply() override;
    void reset() override;

protected:
    static constexpr int kOnWebApiLoginResponse = WM_USER + 1;
    static constexpr int kOnStatusUpdateFinish = WM_USER + 2;

    BEGIN_MSG_MAP( Preferences )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_DESTROY( OnDestroy )
        MSG_WM_CTLCOLORSTATIC( OnCtlColorStatic )
        COMMAND_HANDLER_EX( IDC_COMBO_BITRATE, CBN_SELCHANGE, OnDdxChange )
        COMMAND_HANDLER_EX( IDC_CHECK_NORMALIZE, BN_CLICKED, OnDdxChange )
        COMMAND_HANDLER_EX( IDC_CHECK_PRIVATE, BN_CLICKED, OnDdxChange )
        COMMAND_HANDLER_EX( IDC_BTN_LOGIN_LIBSPOTIFY, BN_CLICKED, OnLibSpotifyLoginClick )
        COMMAND_HANDLER_EX( IDC_BTN_LOGIN_WEBAPI, BN_CLICKED, OnWebApiLoginClick )
        MESSAGE_HANDLER( kOnWebApiLoginResponse, OnWebApiLoginResponse )
        MESSAGE_HANDLER( kOnStatusUpdateFinish, OnStatusUpdateFinish )
    END_MSG_MAP()

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnDestroy();

    HBRUSH OnCtlColorStatic( CDCHandle dc, CStatic wndStatic );

    void OnDdxChange( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnLibSpotifyLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnWebApiLoginClick( UINT uNotifyCode, int nID, CWindow wndCtl );
    LRESULT OnWebApiLoginResponse( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnStatusUpdateFinish( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

private:
    void UpdateUiFromCfg();
    void UpdateLibSpotifyUi();
    void UpdateWebApiUi();
    void UpdateBackendUi( LoginStatus loginStatus, CButton& btn, CStatic& text, std::function<std::string()> getUserNameFn );

    void RefreshLibSpotifySettings();

private:
    preferences_page_callback::ptr callback_;

#define SPTF_DEFINE_UI_OPTION( name ) \
    qwr::ui::UiOption<decltype( config::name )> name##_;

#define SPTF_DEFINE_UI_OPTIONS( ... ) \
    QWR_EXPAND( QWR_PASTE( SPTF_DEFINE_UI_OPTION, __VA_ARGS__ ) )

    SPTF_DEFINE_UI_OPTIONS( preferred_bitrate, enable_normalization, enable_private_mode )

#undef SPTF_DEFINE_OPTIONS
#undef SPTF_DEFINE_OPTION

    std::array<std::unique_ptr<qwr::ui::IUiDdxOption>, 3> ddxOptions_;

    CComboBox comboBitrate_;

    CButton btnLibSpotify_;
    CButton btnWebApi_;

    CStatic textLibSpotify_;
    CStatic textWebApi_;

    LoginStatus libSpotifyStatus_ = LoginStatus::fetching_login_status;
    LoginStatus webApiStatus_ = LoginStatus::fetching_login_status;

    std::unique_ptr<std::thread> pStatusThread_;
};

} // namespace sptf::ui
