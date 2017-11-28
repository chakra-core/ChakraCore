//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCorePch.h"
#ifdef _WIN32
#include <wchar.h> // wmemcpy_s
#endif

int AssertCount = 0;
int AssertsToConsole = false;

#if _WIN32
 // IsInAssert defined in header
#elif !defined(__IOS__)
 __declspec(thread) int IsInAssert = false;
#else
int IsInAssert = false;
#endif

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
    wmemcpy_s(dst, count, src, count);
}

#if defined(_M_IX86) || defined(_M_X64)

void __stdcall js_memset_zero_nontemporal(__bcount(sizeInBytes) void *dst, size_t sizeInBytes)
{
    if ((sizeInBytes % sizeof(__m128i)) == 0)
    {
        size_t writeBytes = 0;
        __m128i simdZero = _mm_setzero_si128();
        for (__m128i * p = (__m128i *)dst; writeBytes < sizeInBytes; p += 1, writeBytes += sizeof(__m128i))
        {
            _mm_stream_si128(p, simdZero);
        }

        _mm_sfence();
    }
    // cannot do non-temporal store directly if set size is not multiple of sizeof(__m128i)
    else
    {
        memset(dst, 0, sizeInBytes);
    }
}

#endif
