#pragma once

#include <resource.h>

#include <mutex>

namespace sptf::ui
{

class NotAuth
    : public CDialogImpl<NotAuth>
{
public:
    enum
    {
        IDD = IDD_NOT_AUTH
    };

    NotAuth( std::function<void()> onDestroyCallback );
    ~NotAuth() = default;

protected:
    BEGIN_MSG_MAP( NotAuth )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_DESTROY( OnDestroy )
        COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
        COMMAND_HANDLER_EX( IDC_BTN_OPEN_PREF, BN_CLICKED, OnOpenPrefClick )
    END_MSG_MAP()

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnDestroy();

    void OnCloseCmd( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnOpenPrefClick( UINT uNotifyCode, int nID, CWindow wndCtl );

    void OnFinalMessage( _In_ HWND hWnd ) override;

    static void GetMsgProc( int code, WPARAM wParam, LPARAM lParam, HWND hParent );

private:
    uint32_t hookId_ = 0;
    std::function<void()> onDestroyCallback_;
};

class NotAuthHandler
{
public:
    ~NotAuthHandler() = default;
    static NotAuthHandler& Get();

    void ShowDialog();
    void CloseDialog();

private:
    NotAuthHandler() = default;

private:
    std::mutex authMutex_;
    sptf::ui::NotAuth* pNotAuth_ = nullptr;
};

} // namespace sptf::ui
