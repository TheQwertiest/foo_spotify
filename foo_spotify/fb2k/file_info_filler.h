#pragma once

#include <string>
#include <unordered_map>

namespace sptf::fb2k
{

void FillFileInfoWithMeta( const std::unordered_multimap<std::string, std::string>& meta, file_info& info );

}