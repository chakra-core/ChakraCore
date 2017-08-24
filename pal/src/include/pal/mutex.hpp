//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//

/*++



Module Name:

    mutex.hpp

Abstract:

    Mutex object structure definition.



--*/

#ifndef _PAL_MUTEX_H_
#define _PAL_MUTEX_H_

#include "corunix.hpp"

namespace CorUnix
{
    extern CObjectType otMutex;

    PAL_ERROR
    InternalCreateMutex(
        CPalThread *pThread,
        LPSECURITY_ATTRIBUTES lpMutexAttributes,
        BOOL bInitialOwner,
        LPCWSTR lpName,
        HANDLE *phMutex
        );

    PAL_ERROR
    InternalReleaseMutex(
        CPalThread *pThread,
        HANDLE hMutex
        );

    PAL_ERROR
    InternalOpenMutex(
        CPalThread *pThread,
        DWORD dwDesiredAccess,
        BOOL bInheritHandle,
        LPCWSTR lpName,
        HANDLE *phMutex
        );

}
#endif //_PAL_MUTEX_H_
