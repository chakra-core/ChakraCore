//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCommonPch.h"

void __stdcall js_memcpy_s(__bcount(sizeInBytes) void *dst, size_t sizeInBytes, __in_bcount(count) const void *src, size_t count)
{
    Assert((count) <= (sizeInBytes));
    if ((count) <= (sizeInBytes))
        memcpy((dst), (src), (count));
    else
        Js::Throw::FatalInternalError();
}

void __stdcall js_wmemcpy_s(__ecount(sizeInWords) char16 *dst, size_t sizeInWords, __in_ecount(count) const char16 *src, size_t count)
{
    //Multiplication Overflow check
    Assert(count <= sizeInWords && count <= SIZE_MAX/sizeof(char16));
    if(!(count <= sizeInWords && count <= SIZE_MAX/sizeof(char16)))
    {
        Js::Throw::FatalInternalError();
    }

    memcpy(dst, src, count * sizeof(char16));
}
