#pragma once

namespace sptf
{

class LibSpotify_BackendUser
{
public:
    ~LibSpotify_BackendUser() = default;
    virtual void Finalize() = 0;
};

} // namespace sptf
