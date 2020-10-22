#include <stdafx.h>

#include "config.h"

namespace sptf::config
{

qwr::fb2k::ConfigBool_MT enable_normalization( sptf::guid::config_enable_normalization, true );
qwr::fb2k::ConfigBool_MT enable_private_mode( sptf::guid::config_enable_private_mode, false );
qwr::fb2k::ConfigUint8_MT libspotify_cache_size_in_percent( sptf::guid::config_libspotify_cache_size_in_percent, 10 );
qwr::fb2k::ConfigUint32_MT libspotify_cache_size_in_mb( sptf::guid::config_libspotify_cache_size_in_mb, 2048 );
qwr::fb2k::ConfigUint8Enum_MT<BitrateSettings> preferred_bitrate( sptf::guid::config_preferred_bitrate, BitrateSettings::Bitrate320k );
qwr::fb2k::ConfigString_MT webapi_auth_scopes( sptf::guid::config_webapi_auth_scopes, "playlist-read-collaborative playlist-read-private user-read-private" );

} // namespace sptf::config
