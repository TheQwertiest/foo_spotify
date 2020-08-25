#pragma once

// clang-format off
// !!! Include order is important here (esp. for Win headers) !!!

// Support only Windows 7 and newer
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

// Fix std min max conflicts
#define NOMINMAX
#include <WinSock2.h>
#include <windows.h>

#include <ShlDisp.h>
#include <exdisp.h>
#include <shobjidl_core.h>
// Generates wrappers for COM listed above
#include <ComDef.h>

// foobar2000 SDK
#pragma warning( push, 0 )
#   include <foobar2000/SDK/foobar2000.h>
#pragma warning( pop ) 

// fmt
#define FMT_HEADER_ONLY
#include <fmt/format.h>

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

// Unicode converters
#include <qwr/unicode.h>

#include <qwr/qwr_exception.h>

// clang-format on
