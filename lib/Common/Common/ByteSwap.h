//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175)) || (defined(_M_ARM32_OR_ARM64) && defined(_MSC_FULL_VER))
#ifdef __cplusplus
extern "C" {
#endif
    _Check_return_ unsigned short __cdecl _byteswap_ushort(_In_ unsigned short _Short);
    _Check_return_ unsigned long  __cdecl _byteswap_ulong(_In_ unsigned long _Long);
    _Check_return_ unsigned __int64 __cdecl _byteswap_uint64(_In_ unsigned __int64 _Int64);
#ifdef __cplusplus
}
#endif
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)

#define RtlUshortByteSwap(_x)    _byteswap_ushort((USHORT)(_x))
#define RtlUlongByteSwap(_x)     _byteswap_ulong((_x))
#define RtlUlonglongByteSwap(_x) _byteswap_uint64((_x))
#elif defined(_WIN32)
#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
USHORT
FASTCALL
RtlUshortByteSwap(
    _In_ USHORT Source
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
ULONG
FASTCALL
RtlUlongByteSwap(
    _In_ ULONG Source
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
    _In_ ULONGLONG Source
    );
#endif

#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>

#define RtlUshortByteSwap(_x)    OSSwapInt16((_x))
#define RtlUlongByteSwap(_x)     OSSwapInt32((_x))
#define RtlUlonglongByteSwap(_x) OSSwapInt64((_x))

#elif defined(__ANDROID__)
#define RtlUshortByteSwap(_x)    __builtin_bswap16((_x))
#define RtlUlongByteSwap(_x)     __builtin_bswap32((_x))
#define RtlUlonglongByteSwap(_x) __builtin_bswap64((_x))

#elif defined(__linux__)
#include <byteswap.h>

#define RtlUshortByteSwap(_x)    __bswap_16((_x))
#define RtlUlongByteSwap(_x)     __bswap_32((_x))
#define RtlUlonglongByteSwap(_x) __bswap_64((_x))

#elif defined(__FreeBSD__)
#include <byteswap.h>
/* FreeBSD 13+, also above definitions would work
 * TODO replace with "has byteswap.h" check? */
#define RtlUshortByteSwap(_x)    bswap_16((_x))
#define RtlUlongByteSwap(_x)     bswap_32((_x))
#define RtlUlonglongByteSwap(_x) bswap_64((_x))

#else
#error "ByteSwap.h: Not implemented for this platform"
#endif
