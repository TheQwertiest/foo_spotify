#pragma once

#include <fb2k/config.h>
#include <ui/ui_ipref_tab.h>

#include <resource.h>

#include <qwr/fb2k_config_ui_option.h>
#include <qwr/macros.h>
#include <qwr/ui_ddx_option.h>

#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace sptf::ui
{

class PreferenceTabManager;

class PreferenceTabPlayback
    : public CDialogImpl<PreferenceTabPlayback>
    , public IPrefTab
{
public:
    enum
    {
        IDD = IDD_PREF_TAB_PLAYBACK
    };

    PreferenceTabPlayback( PreferenceTabManager* pParent );
    ~PreferenceTabPlayback() override;

    // IPrefTab
    HWND CreateTab( HWND hParent ) override;
    CDialogImplBase& Dialog() override;
    const wchar_t* Name() const override;

    t_uint32 get_state() override;
    void apply() override;
    void reset() override;

protected:
    BEGIN_MSG_MAP( PreferenceTabPlayback )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_HSCROLL( OnTrackBarHScroll )
        COMMAND_HANDLER_EX( IDC_COMBO_BITRATE, CBN_SELCHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_CHECK_NORMALIZE, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_CHECK_PRIVATE, BN_CLICKED, OnDdxUiChange )
        NOTIFY_HANDLER_EX( IDC_SLIDER_CACHE_MB, TRBN_THUMBPOSCHANGING, OnTrackBarPosChangedNotify )
        NOTIFY_HANDLER_EX( IDC_SLIDER_CACHE_PERCENT, TRBN_THUMBPOSCHANGING, OnTrackBarPosChangedNotify )
        COMMAND_HANDLER_EX( IDC_EDIT_CACHE_MB, EN_CHANGE, OnTrackBarEdit )
        COMMAND_HANDLER_EX( IDC_EDIT_CACHE_PERCENT, EN_CHANGE, OnTrackBarEdit )
    END_MSG_MAP()

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );

    void OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnDdxValueChange( int nID );

    LRESULT OnTrackBarPosChangedNotify( LPNMHDR pnmh );
    void OnTrackBarHScroll( UINT nSBCode, UINT nPos, CTrackBarCtrl trackBar );
    void OnTrackBarEdit( UINT uNotifyCode, int nID, CWindow wndCtl );

    void DoFullDdxToUi();

    void RefreshLibSpotifySettings();

private:
    PreferenceTabManager* pParent_ = nullptr;

    std::unordered_map<int, int> trackToEdit_;
    std::unordered_map<int, int> editToTrack_;
    std::unordered_set<int> tracksRedrawing_;

    bool suppressUiDdx_ = true;

#define SPTF_DEFINE_UI_OPTION( name ) \
    qwr::ui::UiOption<decltype( config::name )> name##_;

#define SPTF_DEFINE_UI_OPTIONS( ... ) \
    QWR_EXPAND( QWR_PASTE( SPTF_DEFINE_UI_OPTION, __VA_ARGS__ ) )

    SPTF_DEFINE_UI_OPTIONS( enable_normalization,
                            enable_private_mode,
                            libspotify_cache_size_in_percent,
                            libspotify_cache_size_in_mb,
                            preferred_bitrate )

#undef SPTF_DEFINE_OPTIONS
#undef SPTF_DEFINE_OPTION

    std::array<std::unique_ptr<qwr::ui::IUiDdxOption>, 5> ddxOptions_;
};

} // namespace sptf::ui
