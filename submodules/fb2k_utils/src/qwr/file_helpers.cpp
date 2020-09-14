#include <stdafx.h>

#include "file_helpers.h"

#include <qwr/abort_callback.h>
#include <qwr/fbk2_paths.h>
#include <qwr/final_action.h>
#include <qwr/string_helpers.h>
#include <qwr/text_helpers.h>
#include <qwr/winapi_error_helpers.h>

#include <nonstd/span.hpp>

#include <filesystem>

using namespace qwr;
using namespace qwr::file;
namespace fs = std::filesystem;

namespace
{

[[maybe_unused]] constexpr unsigned char kBom32Be[] = { 0x00, 0x00, 0xfe, 0xff };
[[maybe_unused]] constexpr unsigned char kBom32Le[] = { 0xff, 0xfe, 0x00, 0x00 };
[[maybe_unused]] constexpr unsigned char kBom16Be[] = { 0xfe, 0xff }; // must be 4byte size
constexpr unsigned char kBom16Le[] = { 0xff, 0xfe };                  // must be 4byte size, but not 0xff, 0xfe, 0x00, 0x00
constexpr unsigned char kBom8[] = { 0xef, 0xbb, 0xbf };

// TODO: dirty hack! remove
std::unordered_map<std::wstring, UINT> codepageMap;

template <typename T>
T ConvertFileContent( const std::wstring& path, std::string_view content, UINT codepage )
{
    T fileContent;
    if ( content.empty() )
    {
        return fileContent;
    }

    constexpr bool isWide = std::is_same_v<T, std::wstring>;

    const char* curPos = content.data();
    size_t curSize = content.size();

    UINT detectedCodepage = codepage;
    bool isWideCodepage = false;
    if ( curSize >= 4
         && !memcmp( kBom16Le, curPos, sizeof( kBom16Le ) ) )
    {
        curPos += sizeof( kBom16Le );
        curSize -= sizeof( kBom16Le );

        isWideCodepage = true;
    }
    else if ( curSize >= sizeof( kBom8 )
              && !memcmp( kBom8, curPos, sizeof( kBom8 ) ) )
    {
        curPos += sizeof( kBom8 );
        curSize -= sizeof( kBom8 );

        detectedCodepage = CP_UTF8;
    }

    if ( !isWideCodepage && detectedCodepage == CP_ACP )
    { // TODO: dirty hack! remove
        if ( const auto it = codepageMap.find( path );
             it != codepageMap.cend() )
        {
            detectedCodepage = it->second;
        }
        else
        {
            detectedCodepage = qwr::DetectCharSet( std::string_view{ curPos, curSize } ).value_or( CP_ACP );
            codepageMap.emplace( path, detectedCodepage );
        }
    }

    if ( isWideCodepage )
    {
        auto readDataAsWide = [curPos, curSize] {
            std::wstring tmpString;
            tmpString.resize( curSize >> 1 );
            // Can't use wstring.assign(), because of potential aliasing issues
            memcpy( tmpString.data(), curPos, curSize );
            return tmpString;
        };

        if constexpr ( isWide )
        {
            fileContent = readDataAsWide();
        }
        else
        {
            fileContent = qwr::unicode::ToU8( readDataAsWide() );
        }
    }
    else
    {
        auto codepageToWide = [curPos, curSize, detectedCodepage] {
            std::wstring tmpString;
            size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( detectedCodepage, curPos, curSize );
            tmpString.resize( outputSize );

            outputSize = pfc::stringcvt::convert_codepage_to_wide( detectedCodepage, tmpString.data(), outputSize, curPos, curSize );
            tmpString.resize( outputSize );

            return tmpString;
        };

        if constexpr ( isWide )
        {
            fileContent = codepageToWide();
        }
        else
        {
            if ( CP_UTF8 == detectedCodepage )
            {
                fileContent = std::u8string( curPos, curSize );
            }
            else
            {
                fileContent = qwr::unicode::ToU8( codepageToWide() );
            }
        }
    }

    return fileContent;
}

std::filesystem::path GetAbsoluteNormalPath( const std::filesystem::path& path )
{
    namespace fs = std::filesystem;

    try
    {
        return fs::absolute( path ).lexically_normal();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw QwrException( "Failed to open file `{}`:\n"
                            "  {}",
                            path.u8string(),
                            qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
}

} // namespace

namespace
{

class FileReader
{
public:
    FileReader( const fs::path& path, bool checkFileExistense = true );
    ~FileReader();
    FileReader( const FileReader& ) = delete;
    FileReader& operator=( const FileReader& ) = delete;

    [[nodiscard]] std::string_view GetFileContent() const;
    [[nodiscard]] const fs::path& GetFullPath() const;

private:
    std::string_view fileContent_;
    fs::path path_;

    HANDLE hFile_ = nullptr;
    HANDLE hFileMapping_ = nullptr;
    LPCBYTE pFileView_ = nullptr;
    size_t fileSize_ = 0;
};

FileReader::FileReader( const fs::path& inPath, bool checkFileExistense )
{
    namespace fs = std::filesystem;

    const auto fsPath = GetAbsoluteNormalPath( inPath );
    const auto u8path = fsPath.u8string();
    path_ = fsPath;

    try
    {
        if ( checkFileExistense && ( !fs::exists( fsPath ) || !fs::is_regular_file( fsPath ) ) )
        {
            throw QwrException( "Path does not point to a valid file: {}", u8path );
        }

        if ( !fs::file_size( fsPath ) )
        { // CreateFileMapping fails on file with zero length, so we need to bail out early
            return;
        }
        hFile_ = CreateFile( fsPath.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
        qwr::error::CheckWinApi( ( INVALID_HANDLE_VALUE != hFile_ ), "CreateFile" );

        final_action autoFile( [hFile = hFile_] {
            CloseHandle( hFile );
        } );

        hFileMapping_ = CreateFileMapping( hFile_, nullptr, PAGE_READONLY, 0, 0, nullptr );
        qwr::error::CheckWinApi( hFileMapping_, "CreateFileMapping" );

        final_action autoMapping( [hFileMapping = hFileMapping_] {
            CloseHandle( hFileMapping );
        } );

        fileSize_ = GetFileSize( hFile_, nullptr );
        QwrException::ExpectTrue( fileSize_ != INVALID_FILE_SIZE, "Internal error: failed to read file size of `{}`", u8path );

        pFileView_ = static_cast<LPCBYTE>( MapViewOfFile( hFileMapping_, FILE_MAP_READ, 0, 0, 0 ) );
        qwr::error::CheckWinApi( pFileView_, "MapViewOfFile" );

        final_action autoAddress( [pFileView = pFileView_] {
            UnmapViewOfFile( pFileView );
        } );

        autoAddress.cancel();
        autoMapping.cancel();
        autoFile.cancel();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw QwrException( "Failed to open file `{}`:\n"
                            "  {}",
                            u8path,
                            qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
}

FileReader::~FileReader()
{
    if ( pFileView_ )
    {
        UnmapViewOfFile( pFileView_ );
    }
    if ( hFileMapping_ )
    {
        CloseHandle( hFileMapping_ );
    }
    if ( hFile_ )
    {
        CloseHandle( hFile_ );
    }
}

std::string_view FileReader::GetFileContent() const
{
    return std::string_view{ reinterpret_cast<const char*>( pFileView_ ), fileSize_ };
}

const fs::path& FileReader::GetFullPath() const
{
    return path_;
}

template <typename T>
T ReadFileImpl( const fs::path& path, UINT codepage, bool checkFileExistense )
{
    const FileReader fileReader( path, checkFileExistense );
    return ConvertFileContent<T>( fileReader.GetFullPath(), fileReader.GetFileContent(), codepage );
}

} // namespace

namespace qwr::file
{

std::u8string ReadFile( const fs::path& path, UINT codepage, bool checkFileExistense )
{
    return ReadFileImpl<std::u8string>( path, codepage, checkFileExistense );
}

std::wstring ReadFileW( const fs::path& path, UINT codepage, bool checkFileExistense )
{
    return ReadFileImpl<std::wstring>( path, codepage, checkFileExistense );
}

void WriteFile( const fs::path& path, const std::u8string& content, bool write_bom )
{
    const int offset = ( write_bom ? sizeof( kBom8 ) : 0 );
    HANDLE hFile = CreateFile( path.wstring().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
    qwr::error::CheckWinApi( hFile != INVALID_HANDLE_VALUE, "CreateFile" );
    final_action autoFile( [hFile] {
        CloseHandle( hFile );
    } );

    HANDLE hFileMapping = CreateFileMapping( hFile, nullptr, PAGE_READWRITE, 0, content.length() + offset, nullptr );
    qwr::error::CheckWinApi( hFileMapping, "CreateFileMapping" );
    final_action autoMapping( [hFileMapping] {
        CloseHandle( hFileMapping );
    } );

    auto pFileView = static_cast<LPBYTE>( MapViewOfFile( hFileMapping, FILE_MAP_WRITE, 0, 0, 0 ) );
    qwr::error::CheckWinApi( pFileView, "MapViewOfFile" );
    final_action autoAddress( [pFileView] {
        UnmapViewOfFile( pFileView );
    } );

    if ( write_bom )
    {
        memcpy( pFileView, kBom8, sizeof( kBom8 ) );
    }
    memcpy( pFileView + offset, content.c_str(), content.length() );
}

UINT DetectFileCharset( const std::u8string& path )
{
    return qwr::DetectCharSet( FileReader{ path }.GetFileContent() ).value_or( CP_ACP );
}

std::optional<fs::path> FileDialog( const std::wstring& title,
                                    bool saveFile,
                                    const GUID& savePathGuid,
                                    nonstd::span<const COMDLG_FILTERSPEC> filterSpec,
                                    const std::wstring& defaultExtension,
                                    const std::wstring& defaultFilename )
{
    _COM_SMARTPTR_TYPEDEF( IFileDialog, __uuidof( IFileDialog ) );
    _COM_SMARTPTR_TYPEDEF( IShellItem, __uuidof( IShellItem ) );

    try
    {
        IFileDialogPtr pfd;
        HRESULT hr = pfd.CreateInstance( ( saveFile ? CLSID_FileSaveDialog : CLSID_FileOpenDialog ), nullptr, CLSCTX_INPROC_SERVER );
        qwr::error::CheckHR( hr, "CreateInstance" );

        DWORD dwFlags;
        hr = pfd->GetOptions( &dwFlags );
        qwr::error::CheckHR( hr, "GetOptions" );

        hr = pfd->SetClientGuid( savePathGuid );
        qwr::error::CheckHR( hr, "SetClientGuid" );

        hr = pfd->SetTitle( title.c_str() );
        qwr::error::CheckHR( hr, "SetTitle" );

        if ( !filterSpec.empty() )
        {
            hr = pfd->SetFileTypes( filterSpec.size(), filterSpec.data() );
            qwr::error::CheckHR( hr, "SetFileTypes" );
        }

        //hr = pfd->SetFileTypeIndex( 1 );
        //qwr::error::CheckHR( hr, "SetFileTypeIndex" );

        if ( defaultExtension.length() )
        {
            hr = pfd->SetDefaultExtension( defaultExtension.c_str() );
            qwr::error::CheckHR( hr, "SetDefaultExtension" );
        }

        if ( defaultFilename.length() )
        {
            hr = pfd->SetFileName( defaultFilename.c_str() );
            qwr::error::CheckHR( hr, "SetFileName" );
        }

        IShellItemPtr pFolder;
        hr = SHCreateItemFromParsingName( path::Component().wstring().c_str(), nullptr, IShellItemPtr::GetIID(), reinterpret_cast<void**>( &pFolder ) );
        qwr::error::CheckHR( hr, "SHCreateItemFromParsingName" );

        hr = pfd->SetDefaultFolder( pFolder );
        qwr::error::CheckHR( hr, "SetDefaultFolder" );

        hr = pfd->Show( nullptr );
        qwr::error::CheckHR( hr, "Show" );

        IShellItemPtr psiResult;
        hr = pfd->GetResult( &psiResult );
        qwr::error::CheckHR( hr, "GetResult" );

        PWSTR pszFilePath = nullptr;
        hr = psiResult->GetDisplayName( SIGDN_FILESYSPATH, &pszFilePath );
        qwr::error::CheckHR( hr, "GetDisplayName" );

        qwr::final_action autoFilePath( [pszFilePath] {
            if ( pszFilePath )
            {
                CoTaskMemFree( pszFilePath );
            }
        } );

        return pszFilePath;
    }
    catch ( const std::filesystem::filesystem_error& )
    {
        return std::nullopt;
    }
    catch ( const QwrException& )
    {
        return std::nullopt;
    }
}

} // namespace qwr::file
