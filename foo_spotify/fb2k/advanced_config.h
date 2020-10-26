#pragma once

#include <qwr/fb2k_adv_config.h>

namespace sptf::config::advanced
{

extern qwr::fb2k::AdvConfigString_MT network_proxy;
extern qwr::fb2k::AdvConfigString_MT network_proxy_username;
extern qwr::fb2k::AdvConfigString_MT network_proxy_password;

extern qwr::fb2k::AdvConfigBool_MT logging_webapi_request;
extern qwr::fb2k::AdvConfigBool_MT logging_webapi_response;

} // namespace sptf::config::advanced
