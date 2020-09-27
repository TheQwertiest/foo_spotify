#pragma once

#include <utils/secure_vector.h>

namespace sptf
{

struct CredentialsResult
{
    SecureVector<char> un;
    SecureVector<char> pw;
    bool cancelled = false;
};

std::unique_ptr<CredentialsResult> ShowCredentialsDialog( HWND hWnd, const char* msg );

} // namespace sptf
