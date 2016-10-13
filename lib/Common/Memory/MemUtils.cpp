//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonMemoryPch.h"

void
Memory::ChakraMemSet(_In_ void *dst, int val, size_t sizeInBytes, HANDLE processHandle)
{
    byte* dest = (byte*)dst;
    if (processHandle == GetCurrentProcess())
    {
        memset(dest, val, sizeInBytes);
    }
    else
    {
        const size_t bufferSize = 0x1000;
        byte writeBuffer[bufferSize];
        memset(writeBuffer, val, bufferSize < sizeInBytes ? bufferSize : sizeInBytes);

        for (size_t i = 0; i < sizeInBytes / bufferSize; i++)
        {
            if (!WriteProcessMemory(processHandle, dest, writeBuffer, bufferSize, NULL))
            {
                MemoryOperationLastError::CheckProcessAndThrowFatalError(processHandle);
            }
            dest += bufferSize;
        }

        if (sizeInBytes % bufferSize > 0)
        {
            if (!WriteProcessMemory(processHandle, dest, writeBuffer, sizeInBytes%bufferSize, NULL))
            {
                MemoryOperationLastError::CheckProcessAndThrowFatalError(processHandle);
            }
        }
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
    else if (!WriteProcessMemory(processHandle, dst, src, count, NULL))
    {
        MemoryOperationLastError::CheckProcessAndThrowFatalError(processHandle);
    }

}
