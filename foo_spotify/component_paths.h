#pragma once

#include <filesystem>

namespace sptf::path
{

std::filesystem::path LibSpotifyCache();
std::filesystem::path LibSpotifySettings();

std::filesystem::path WebApiCache();
std::filesystem::path WebApiSettings();

} // namespace sptf::guid
