#pragma once

namespace sptf
{

class LibSpotifyBackendUser
{
public:
    ~LibSpotifyBackendUser() = default;
    virtual void Finalize() = 0;
};

} // namespace sptf
