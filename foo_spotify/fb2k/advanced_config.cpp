#include <stdafx.h>

#include "advanced_config.h"

namespace
{

advconfig_branch_factory branch_sptf(
    "Spotify Integration", sptf::guid::adv_branch, advconfig_branch::guid_branch_tools, 0 );
advconfig_branch_factory branch_network(
    "Network: restart is required", sptf::guid::adv_branch_network, sptf::guid::adv_branch, 0 );
advconfig_branch_factory branch_logging(
    "Logging: restart is required", sptf::guid::adv_branch_logging, sptf::guid::adv_branch, 1 );

} // namespace

namespace sptf::config::advanced
{

qwr::fb2k::AdvConfigString_MT network_proxy(
    "Proxy: url",
    sptf::guid::adv_var_network_proxy, sptf::guid::adv_branch_network, 0,
    "" );

qwr::fb2k::AdvConfigString_MT network_proxy_username(
    "Proxy: username",
    sptf::guid::adv_var_network_proxy_username, sptf::guid::adv_branch_network, 1,
    "" );

qwr::fb2k::AdvConfigString_MT network_proxy_password(
    "Proxy: password",
    sptf::guid::adv_var_network_proxy_password, sptf::guid::adv_branch_network, 2,
    "" );

qwr::fb2k::AdvConfigBool_MT logging_webapi_request(
    "Log Spotify Web API: request",
    sptf::guid::adv_var_logging_webapi_request, sptf::guid::adv_branch_logging, 0,
    false );

qwr::fb2k::AdvConfigBool_MT logging_webapi_response(
    "Log Spotify Web API: response",
    sptf::guid::adv_var_logging_webapi_response, sptf::guid::adv_branch_logging, 0,
    false );

} // namespace sptf::config::advanced
