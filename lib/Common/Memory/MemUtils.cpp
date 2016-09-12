//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonMemoryPch.h"
#include <wchar.h>

void
Memory::ChakraMemSet(__bcount(sizeInBytes) void *dst, int val, size_t sizeInBytes, HANDLE processHandle)
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
Memory::ChakraWMemSet(__ecount(count) char16* dst, char16 val, size_t count, HANDLE processHandle)
{
    const bool isLocalProc = processHandle == GetCurrentProcess();
    char16 * writeBuffer;

    if (isLocalProc)
    {
        writeBuffer = (char16*)dst;
    }
    else
    {
        writeBuffer = HeapNewArray(char16, count);
    }
    wmemset(writeBuffer, val, count);
    if (!isLocalProc)
    {
        if (!WriteProcessMemory(processHandle, dst, writeBuffer, count * sizeof(char16), NULL))
        {
            Js::Throw::FatalInternalError();
        }
        HeapDeleteArray(count, writeBuffer);
    }
}

void
Memory::ChakraMemCopy(__bcount(sizeInBytes) void *dst, size_t sizeInBytes, __in_bcount(count) const void *src, size_t count, HANDLE processHandle)
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
