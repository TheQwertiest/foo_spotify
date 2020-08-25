#include <stdafx.h>

#include "cred_prompt.h"

#include <Ntsecapi.h>
#include <WinCred.h>

namespace sptf
{

std::unique_ptr<CredentialsResult> ShowCredentialsDialog( HWND hWnd, const char* msg )
{
    const auto wMsg = [msg]
    {
        if (msg)
        {
            auto ret = qwr::unicode::ToWide( std::string_view{ msg } );
            if ( ret.size() > CREDUI_MAX_MESSAGE_LENGTH )
            {
                ret.resize( CREDUI_MAX_MESSAGE_LENGTH );
                ret[CREDUI_MAX_MESSAGE_LENGTH - 3] = L'.';
                ret[CREDUI_MAX_MESSAGE_LENGTH - 2] = L'.';
                ret[CREDUI_MAX_MESSAGE_LENGTH - 1] = L'.';
            }
            return ret;
        }
        else
        {
            return std::wstring( L"Please enter your Spotify username and password." );
        }
    }();

    CREDUI_INFOW cui{};
    cui.cbSize = sizeof( CREDUI_INFO );
    cui.hwndParent = hWnd;
    //  Ensure that MessageText and CaptionText identify what credentials
    //  to use and which application requires them.
    cui.pszMessageText = wMsg.c_str();
    cui.pszCaptionText = L"Sign in to Spotify";
    BOOL fSave = FALSE;

    SecureVector<wchar_t> pszName( CREDUI_MAX_USERNAME_LENGTH + 1 );
    SecureVector<wchar_t> pszPwd( CREDUI_MAX_PASSWORD_LENGTH + 1 );

    const DWORD dwErr = CredUIPromptForCredentialsW(
        &cui,                              // CREDUI_INFO structure
        TEXT( SPTF_UNDERSCORE_NAME ),      // Target for credentials
        nullptr,                           // Reserved
        0,                                 // Reason
        pszName.data(),                    // User name
        pszName.size(),                    // Max number of char for user name
        pszPwd.data(),                     // Password
        pszPwd.size(),                     // Max number of char for password
        &fSave,                            // State of save check box
        CREDUI_FLAGS_GENERIC_CREDENTIALS | // flags
            CREDUI_FLAGS_ALWAYS_SHOW_UI | CREDUI_FLAGS_DO_NOT_PERSIST );

    auto cpr = std::make_unique<CredentialsResult>();
    if ( dwErr == NO_ERROR )
    {
        const auto wtou = []( const auto& in, auto& out ) {
            size_t stringLen = WideCharToMultiByte( CP_UTF8, 0, in.data(), in.size(), nullptr, 0, nullptr, nullptr );
            out.resize( stringLen );

            stringLen = WideCharToMultiByte( CP_UTF8, 0, in.data(), in.size(), out.data(), out.size(), nullptr, nullptr );
            out.resize( stringLen );
        };

        wtou( pszName, cpr->un );
        wtou( pszPwd, cpr->pw );
    }
    else if ( dwErr == ERROR_CANCELLED )
    {
        cpr->cancelled = true;
    }
    return cpr;
}

} // namespace sptf
