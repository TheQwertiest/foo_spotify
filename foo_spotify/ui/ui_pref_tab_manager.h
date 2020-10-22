#pragma once

#include <ui/ui_ipref_tab.h>

#include <resource.h>

#include <array>

namespace sptf::ui
{

class PreferenceTabMain;

class PreferenceTabManager
    : public CDialogImpl<PreferenceTabManager>
    , public CWinDataExchange<PreferenceTabManager>
    , public preferences_page_instance
{
public:
    enum
    {
        IDD = IDD_PREF_TAB_HOST
    };

    BEGIN_MSG_MAP( PreferenceTabManager )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_PARENTNOTIFY( OnParentNotify )
        MESSAGE_HANDLER( WM_WINDOWPOSCHANGED, OnWindowPosChanged )
        NOTIFY_HANDLER_EX( IDC_TAB_HOST, TCN_SELCHANGE, OnSelectionChanged )
    END_MSG_MAP()

public:
    PreferenceTabManager( preferences_page_callback::ptr callback );

    void OnDataChanged();

    // preferences_page_instance
    HWND get_wnd() override;
    t_uint32 get_state() override;
    void apply() override;
    void reset() override;

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnParentNotify( UINT message, UINT nChildID, LPARAM lParam );
    LRESULT OnWindowPosChanged( UINT, WPARAM, LPARAM lp, BOOL& bHandled );
    LRESULT OnSelectionChanged( LPNMHDR pNmhdr );

    void CreateTab();
    void DestroyTab();

private:
    preferences_page_callback::ptr callback_;

    CTabCtrl cTabs_;
    CDialogImplBase* pcCurTab_ = nullptr;

    size_t activeTabIdx_ = 0;
    std::array<std::unique_ptr<IPrefTab>, 2> tabs_;
};

} // namespace sptf::ui
