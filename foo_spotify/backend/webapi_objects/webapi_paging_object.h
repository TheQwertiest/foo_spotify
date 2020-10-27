#pragma once

#include <memory>
#include <optional>
#include <string>

namespace sptf
{

struct WebApi_PagingObject
{
    // href 	string 	A link to the Web API endpoint returning the full result of the request.
    nlohmann::json items;
    size_t limit;
    std::optional<std::wstring> next;
    size_t offset;
    std::optional<std::wstring> previous;
    size_t total;
};

void from_json( const nlohmann::json& j, WebApi_PagingObject& p );

} // namespace sptf
