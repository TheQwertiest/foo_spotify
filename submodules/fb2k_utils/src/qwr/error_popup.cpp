#include <stdafx.h>

#include "error_popup.h"

#include <qwr/delayed_executor.h>

namespace qwr
{

void ReportErrorWithPopup( const std::string& errorText, const std::string& title )
{
    FB2K_console_formatter() << errorText;
    qwr::DelayedExecutor::GetInstance().AddTask( [errorText, title] {
        popup_message::g_show( errorText.c_str(), title.c_str() );
    } );
    MessageBeep( MB_ICONASTERISK );
}

} // namespace qwr
