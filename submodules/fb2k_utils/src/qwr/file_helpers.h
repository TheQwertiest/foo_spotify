#pragma once

#include <shtypes.h>

#include <nonstd/span.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace qwr::file
{

/// @throw smp::SmpException
std::u8string ReadFile( const std::filesystem::path& path, UINT codepage, bool checkFileExistense = true );

/// @throw smp::SmpException
std::wstring ReadFileW( const std::filesystem::path& path, UINT codepage, bool checkFileExistense = true );

/// @throw smp::SmpException
void WriteFile( const std::filesystem::path& path, const std::u8string& content, bool write_bom = true );

UINT DetectFileCharset( const std::u8string& path );

std::optional<std::filesystem::path>
FileDialog( const std::wstring& title,
            bool saveFile,
            const GUID& savePathGuid,
            nonstd::span<const COMDLG_FILTERSPEC> filterSpec = std::array<COMDLG_FILTERSPEC, 1>{ COMDLG_FILTERSPEC{ L"All files", L"*.*" } },
            const std::wstring& defaultExtension = L"",
            const std::wstring& defaultFilename = L"" );

} // namespace qwr::file
