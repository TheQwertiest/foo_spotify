#include <stdafx.h>

#include "config.h"

namespace sptf::config
{

qwr::fb2k::ConfigBool_MT enable_normalization( sptf::guid::config_enable_normalization, true );
qwr::fb2k::ConfigBool_MT enable_private_mode( sptf::guid::config_enable_private_mode, false );
qwr::fb2k::ConfigUint8Enum_MT<BitrateSettings> preferred_bitrate( sptf::guid::config_preferred_bitrate, BitrateSettings::Bitrate320k );
qwr::fb2k::ConfigString_MT webapi_auth_scopes( sptf::guid::config_webapi_auth_scopes, "playlist-read-collaborative playlist-read-private" );

} // namespace sptf::config
