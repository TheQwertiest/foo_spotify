#include <stdafx.h>

#include "delayed_executor.h"

namespace qwr
{

DelayedExecutor& DelayedExecutor::GetInstance()
{
    static DelayedExecutor de;
    return de;
}

void DelayedExecutor::EnableExecution()
{
    canExecute_ = true;
    while ( !tasks_.empty() )
    {
        std::invoke( *tasks_.front() );
        tasks_.pop();
    }
}

} // namespace qwr
