#include <stdafx.h>

#include "config.h"

#include <libspotify/api.h>

namespace sptf::config
{

qwr::ConfigUint8_MT preferred_bitrate( sptf::guid::config_preferred_bitrate, static_cast<uint8_t>( SP_BITRATE_320k ) );

}
