#pragma once

#include <chrono>

namespace sptf
{

bool SleepFor( const std::chrono::milliseconds& duration, abort_callback& abort );

} // namespace sptf
