#include <stdafx.h>

#include "file_info_filler.h"

#include <qwr/string_helpers.h>

namespace sptf
{

void FillFileInfoWithMeta( const std::unordered_multimap<std::string, std::string>& meta, file_info& info )
{
    auto addMeta = [&]( file_info& p_info, std::string_view metaName ) {
        const auto er = meta.equal_range( std::string( metaName.begin(), metaName.end() ) );
        if ( er.first != er.second )
        {
            const auto [key, val] = *er.first;
            p_info.meta_add( metaName.data(), val.c_str() );
        }
    };
    auto addMetaIfPositive = [&]( file_info& p_info, std::string_view metaName ) {
        const auto er = meta.equal_range( std::string( metaName.begin(), metaName.end() ) );
        if ( er.first != er.second )
        {
            const auto [key, val] = *er.first;
            auto numOpt = qwr::string::GetNumber<uint32_t>( static_cast<std::string_view>( val ) );
            if ( numOpt && *numOpt )
            {
                p_info.meta_add( metaName.data(), val.c_str() );
            }
        }
    };
    auto addMetaMultiple = [&]( file_info& p_info, std::string_view metaName ) {
        const auto er = meta.equal_range( std::string( metaName.begin(), metaName.end() ) );
        for ( auto it = er.first; it != er.second; ++it )
        {
            const auto [key, val] = *er.first;
            p_info.meta_add( metaName.data(), val.c_str() );
        }
    };

    addMeta( info, "TITLE" );
    addMeta( info, "ALBUM" );
    addMeta( info, "DATE" );
    addMetaMultiple( info, "ARTIST" );
    addMetaMultiple( info, "ALBUM ARTIST" );
    addMetaIfPositive( info, "TRACKNUMBER" );
    addMetaIfPositive( info, "DISCNUMBER" );
}

} // namespace sptf
