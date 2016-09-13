//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonMemoryPch.h"

void
Memory::ChakraMemSet(_In_ void *dst, int val, size_t sizeInBytes, HANDLE processHandle)
{
    const bool isLocalProc = processHandle == GetCurrentProcess();
    byte * writeBuffer;

    if (isLocalProc)
    {
        writeBuffer = (byte*)dst;
    }
    else
    {
        writeBuffer = HeapNewArray(byte, sizeInBytes);
    }
    memset(writeBuffer, val, sizeInBytes);
    if (!isLocalProc)
    {
        if (!WriteProcessMemory(processHandle, dst, writeBuffer, sizeInBytes, NULL))
        {
            Js::Throw::FatalInternalError();
        }
        HeapDeleteArray(sizeInBytes, writeBuffer);
    }
}

void
Memory::ChakraMemCopy(_In_ void *dst, size_t sizeInBytes, _In_reads_bytes_(count) const void *src, size_t count, HANDLE processHandle)
{
    Assert(count <= sizeInBytes);
    if (count > sizeInBytes)
    {
        Js::Throw::FatalInternalError();
    }

    if (processHandle == GetCurrentProcess())
    {
        memcpy(dst, src, count);
    }
    else
    {
        if (!WriteProcessMemory(processHandle, dst, src, count, NULL))
        {
            Output::Print(_u("FATAL ERROR: WriteProcessMemory failed, GLE: %d\n"), GetLastError());
            Js::Throw::FatalInternalError();
        }
    }
}
