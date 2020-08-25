#include <stdafx.h>

#include "component_paths.h"

#include <qwr/fbk2_paths.h>

namespace fs = std::filesystem;

namespace sptf::path
{

fs::path LibSpotifyCache()
{
    return qwr::path::Profile() / SPTF_UNDERSCORE_NAME / "ls" / "cache";
}
fs::path LibSpotifySettings()
{
    return qwr::path::Profile() / SPTF_UNDERSCORE_NAME / "ls" / "settings";
}

fs::path WebApiCache()
{
    return qwr::path::Profile() / SPTF_UNDERSCORE_NAME / "wa" / "cache";
}
fs::path WebApiSettings()
{
    return qwr::path::Profile() / SPTF_UNDERSCORE_NAME / "wa" / "settings";
}

} // namespace sptf::path