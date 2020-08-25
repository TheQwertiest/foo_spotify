#pragma once

// clang-format off
// !!! Include order is important here (esp. for Win headers) !!!

// Support only Windows 7 and newer
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

// Fix std min max conflicts
#define NOMINMAX
#include <WinSock2.h>
#include <Windows.h>

// ATL/WTL
/// atlstr.h (includes atlbase.h) must be included first for CString to LPTSTR conversion to work.
#include <atlstr.h> 
#include <atlapp.h>
#include <atlcom.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlddx.h>
#include <atldlgs.h>
#include <atlfind.h>
#include <atlframe.h>
#include <atltheme.h>
#include <atltypes.h>
#include <atlwin.h>

// foobar2000 SDK
#pragma warning( push, 0 )
#   include <foobar2000/SDK/foobar2000.h>
#pragma warning( pop ) 

// fmt
#define FMT_HEADER_ONLY
#include <fmt/format.h>

// nlohmann json
#include <nlohmann/json.hpp>

// range v3
#include <range/v3/all.hpp>

#if not __cpp_char8_t
// Dummy type
#include <string>

using char8_t = char;
namespace std // NOLINT(cert-dcl58-cpp)
{
using u8string = basic_string<char8_t, char_traits<char8_t>, allocator<char8_t>>;
using u8string_view = basic_string_view<char8_t>;
}
#endif

// Additional PFC wrappers
#include <qwr/pfc_helpers_cnt.h>
#include <qwr/pfc_helpers_stream.h>

#include <qwr/unicode.h>
#include <qwr/qwr_exception.h>

#include <utils/json_macro_fix.h>

#include <component_defines.h>
#include <component_guids.h>
#include <component_paths.h>

// clang-format on
