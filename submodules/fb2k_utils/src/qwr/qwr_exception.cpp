#include <stdafx.h>

#include "qwr_exception.h"

namespace qwr
{

_Post_satisfies_( checkValue ) void QwrException::ExpectTrue( bool checkValue, std::string_view errorMessage )
{
    if ( !checkValue )
    {
        throw QwrException( errorMessage );
    }
}

void QwrException::ExpectTrue( _Post_notnull_ void* checkValue, std::string_view errorMessage )
{
    return ExpectTrue( static_cast<bool>( checkValue ), errorMessage );
}

} // namespace qwr
