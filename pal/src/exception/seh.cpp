//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "pal/thread.hpp"
#include "pal/handleapi.hpp"

VOID
PALAPI
RaiseException(IN DWORD dwExceptionCode,
               IN DWORD dwExceptionFlags,
               IN DWORD nNumberOfArguments,
               IN CONST ULONG_PTR *lpArguments)
{
    if (dwExceptionCode == DBG_TERMINATE_PROCESS || dwExceptionFlags & EXCEPTION_NONCONTINUABLE)
    {
        abort();
    }
}
