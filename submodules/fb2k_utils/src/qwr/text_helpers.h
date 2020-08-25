#pragma once

#include <optional>
#include <string_view>

namespace qwr
{

std::optional<UINT> DetectCharSet( std::string_view text );

} // namespace qwr
