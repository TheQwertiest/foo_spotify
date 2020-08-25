#include <stdafx.h>

#include "advanced_config.h"

namespace
{

advconfig_branch_factory branch_sptf(
    "Spotify Integration", sptf::guid::adv_branch, advconfig_branch::guid_branch_tools, 0 );
advconfig_branch_factory branch_network(
    "Network: restart is required", sptf::guid::adv_branch_network, sptf::guid::adv_branch, 0 );

} // namespace

namespace sptf::config::advanced
{

advconfig_string_factory network_proxy(
    "Proxy",
    sptf::guid::adv_var_network_proxy, sptf::guid::adv_branch_network, 0,
    "" );

} // namespace sptf::config::advanced
