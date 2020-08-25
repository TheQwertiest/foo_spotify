#include <stdafx.h>

#include <component_urls.h>

#include <qwr/acfu_integration.h>

using namespace sptf;

namespace
{

class SptfSource
    : public qwr::acfu::QwrSource
{
public:
    // ::acfu::source
    GUID get_guid() override;
    ::acfu::request::ptr create_request() override;

    // qwr::acfu::github_conf
    static pfc::string8 get_owner();
    static pfc::string8 get_repo();

    // qwr::acfu::QwrSource
    std::string GetComponentName() const override;
    std::string GetComponentFilename() const override;
};

} // namespace

namespace
{

GUID SptfSource::get_guid()
{
    return guid::acfu_source;
}

::acfu::request::ptr SptfSource::create_request()
{
    return fb2k::service_new<qwr::acfu::github_latest_release<SptfSource>>();
}

pfc::string8 SptfSource::get_owner()
{
    return "TheQwertiest";
}

pfc::string8 SptfSource::get_repo()
{
    return qwr::unicode::ToU8( std::wstring_view{ url::homepage } ).c_str();
}

std::string SptfSource::GetComponentName() const
{
    return SPTF_NAME;
}

std::string SptfSource::GetComponentFilename() const
{
    return SPTF_UNDERSCORE_NAME;
}

service_factory_single_t<SptfSource> g_acfuSource;

} // namespace
