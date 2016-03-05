//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonMemoryPch.h"

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
        if (!WriteProcessMemory(processHandle, dst, src, sizeInBytes, NULL))
        {
            Js::Throw::FatalInternalError();
        }
    }
}
