//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Memory
{
    void ChakraMemSet(_In_ void *dst, int val, size_t sizeInBytes, HANDLE processHandle = GetCurrentProcess());
    void ChakraMemCopy(_In_ void *dst, size_t sizeInBytes, _In_reads_bytes_(count) const void *src, size_t count, HANDLE processHandle = GetCurrentProcess());
} // namespace Memory
