//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Memory
{
    void ChakraMemSet(__bcount(sizeInBytes) void *dst, int val, size_t sizeInBytes, HANDLE processHandle = GetCurrentProcess());
    void ChakraMemCopy(__bcount(sizeInBytes) void *dst, size_t sizeInBytes, __in_bcount(count) const void *src, size_t count, HANDLE processHandle = GetCurrentProcess());
} // namespace Memory
