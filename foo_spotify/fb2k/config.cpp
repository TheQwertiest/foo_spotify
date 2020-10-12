#include <stdafx.h>

#include "config.h"

namespace sptf::config
{

qwr::fb2k::ConfigUint8Enum_MT<BitrateSettings> preferred_bitrate( sptf::guid::config_preferred_bitrate, BitrateSettings::Bitrate320k );

}
