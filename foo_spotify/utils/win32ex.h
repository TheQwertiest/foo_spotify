#pragma once

#include <sstream>

struct win32exception : std::exception
{
    std::string makeMsg( const std::string& cause, DWORD err )
    {
        std::stringstream ss;
        ss << cause << ", win32: " << err << " (" << std::hex << err << "): " << format_win32_error( err );
        return ss.str();
    }

    win32exception( std::string cause )
        : std::exception( makeMsg( cause, GetLastError() ).c_str() )
    {
    }

    win32exception( std::string cause, DWORD err )
        : std::exception( makeMsg( cause, err ).c_str() )
    {
    }
};