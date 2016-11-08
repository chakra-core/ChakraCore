//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

// Conditionally-compiled on x64 and arm
#if PDATA_ENABLED

#ifdef _WIN32
// ----------------------------------------------------------------------------
//  _WIN32 x64 unwind uses PDATA
// ----------------------------------------------------------------------------

void PDataManager::RegisterPdata(RUNTIME_FUNCTION* pdataStart, _In_ const ULONG_PTR functionStart, _In_ const ULONG_PTR functionEnd, _Out_ PVOID* pdataTable, ULONG entryCount, ULONG maxEntryCount)
{
    BOOLEAN success = FALSE;
    if (AutoSystemInfo::Data.IsWin8OrLater())
    {
        Assert(pdataTable != NULL);

        // Since we do not expect many thunk functions to be created, we are using 1 table/function
        // for now. This can be optimized further if needed.
        DWORD status = NtdllLibrary::Instance->AddGrowableFunctionTable(pdataTable,
            pdataStart,
            entryCount,
            maxEntryCount,
            /*RangeBase*/ functionStart,
            /*RangeEnd*/ functionEnd);
        success = NT_SUCCESS(status);
        if (success)
        {
            Assert(pdataTable);
        }
    }
    else
    {
        *pdataTable = pdataStart;
        success = RtlAddFunctionTable(pdataStart, entryCount, functionStart);
    }
    Js::Throw::CheckAndThrowOutOfMemory(success);
}

void PDataManager::UnregisterPdata(RUNTIME_FUNCTION* pdata)
{
    if (AutoSystemInfo::Data.IsWin8OrLater())
    {
        NtdllLibrary::Instance->DeleteGrowableFunctionTable(pdata);
    }
    else
    {
        BOOLEAN success = RtlDeleteFunctionTable(pdata);
        Assert(success);
    }
}

#else  // !_WIN32

// ----------------------------------------------------------------------------
//  !_WIN32 x64 unwind uses .eh_frame
// ----------------------------------------------------------------------------

void PDataManager::RegisterPdata(RUNTIME_FUNCTION* pdataStart,
    _In_ const ULONG_PTR functionStart, _In_ const ULONG_PTR functionEnd,
    _Out_ PVOID* pdataTable, ULONG entryCount, ULONG maxEntryCount)
{
    __REGISTER_FRAME(pdataStart);
    *pdataTable = pdataStart;
}

void PDataManager::UnregisterPdata(RUNTIME_FUNCTION* pdata)
{
    __DEREGISTER_FRAME(pdata);
}

#endif  // !_WIN32
#endif  // PDATA_ENABLED
