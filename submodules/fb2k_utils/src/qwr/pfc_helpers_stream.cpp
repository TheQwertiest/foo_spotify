#include <stdafx.h>

#include "pfc_helpers_stream.h"

pfc::string_base& operator<<( pfc::string_base& in, std::u8string_view arg )
{
    in.add_string( arg.data(), arg.size() );
    return in;
}

pfc::string_base& operator<<( pfc::string_base& in, std::wstring_view arg )
{
    const auto u8Arg = qwr::unicode::ToU8( arg );
    in.add_string( u8Arg.data(), u8Arg.size() );
    return in;
}

namespace qwr::pfc_x
{

std::u8string ReadString( stream_reader& stream, abort_callback& abort )
{ // ripped from `stream_reader::read_string`
    t_uint32 length;
    stream.read_lendian_t( length, abort );

    std::u8string value;
    value.resize( length );
    stream.read_object( value.data(), length, abort );
    value.resize( strlen( value.c_str() ) );

    return value;
}

std::u8string ReadRawString( stream_reader& stream, abort_callback& abort )
{ // ripped from `stream_reader::read_string_raw`
    constexpr size_t maxBufferSize = 256;
    std::array<char, maxBufferSize> buffer;
    std::u8string value;

    bool hasMoreData = true;
    do
    {
        const size_t dataRead = stream.read( buffer.data(), buffer.size(), abort );
        hasMoreData = ( dataRead == maxBufferSize );
        if ( dataRead )
        {
            value.append( buffer.data(), dataRead );
        }
    } while ( hasMoreData );

    return value;
}

void WriteString( stream_writer& stream, const std::u8string& val, abort_callback& abort )
{
    stream.write_string( val.c_str(), val.length(), abort );
}

void WriteStringRaw( stream_writer& stream, const std::u8string& val, abort_callback& abort )
{
    stream.write_object( val.c_str(), val.length(), abort );
}

} // namespace qwr::pfc_x
