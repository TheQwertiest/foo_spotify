#include <stdafx.h>

#include "text_helpers.h"

#include <MLang.h>

#include <nonstd/span.hpp>

#include <cwctype>
#include <optional>

namespace
{

enum class CodePage : UINT
{
    DosLatin1 = 850,
    ShiftJis = 932,
    Gbk = 936,
    EucKr = 949,
    Big5 = 950,
    WinLatin1 = 1252,
    UsAscii = 20127,
    Utf8 = CP_UTF8,
};

UINT FilterEncodings( nonstd::span<const DetectEncodingInfo> encodings )
{
    assert( !encodings.empty() );

    if ( encodings.size() == 1 )
    {
        const auto codepage = encodings[0].nCodePage;
        if ( codepage == static_cast<UINT>( CodePage::UsAscii ) )
        { // UsAscii is a subset of Utf8
            return static_cast<UINT>( CodePage::Utf8 );
        }
        else
        {
            return codepage;
        }
    }
    else
    {
        return ranges::max_element( encodings,
                                    []( const auto& a, const auto& b ) {
                                        return ( a.nConfidence < b.nConfidence );
                                    } )
            ->nCodePage;
    }
}

} // namespace

namespace qwr
{

std::optional<UINT> DetectCharSet( std::string_view text )
{
    _COM_SMARTPTR_TYPEDEF( IMultiLanguage2, IID_IMultiLanguage2 );
    IMultiLanguage2Ptr lang;

    HRESULT hr = lang.CreateInstance( CLSID_CMultiLanguage, nullptr, CLSCTX_INPROC_SERVER );
    if ( FAILED( hr ) )
    {
        return std::nullopt;
    }

    constexpr int maxEncodings = 2;
    int encodingCount = maxEncodings;
    std::array<DetectEncodingInfo, maxEncodings> encodings;
    int iTextSize = text.size();

    hr = lang->DetectInputCodepage( MLDETECTCP_NONE, 0, const_cast<char*>( text.data() ), &iTextSize, encodings.data(), &encodingCount );
    if ( FAILED( hr ) || !encodingCount )
    {
        return std::nullopt;
    }

    return FilterEncodings( nonstd::span<const DetectEncodingInfo>( encodings.data(), encodingCount ) );
}

} // namespace qwr
