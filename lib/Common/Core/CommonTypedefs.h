//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <limits.h>

#ifdef _WIN32
typedef WCHAR char16;
#define _u(s) L##s
#else
#define _u(s) u##s
#endif

typedef char16 wchar;
typedef unsigned int uint;
typedef unsigned short ushort;

// Use of ulong is not allowed in ChakraCore- uint32 should be used instead since
// it's our cross-platform 32 bit integer value. However, legacy code in ChakraFull
// still depends on ulong so it's allowed for full builds.
#ifdef NTBUILD
typedef unsigned long ulong;
#endif

typedef signed char sbyte;

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned char byte;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

// charcount_t represents a count of characters in a JavascriptString
// It is unsigned and the maximum value is (INT_MAX-1)
typedef uint32 charcount_t;

//A Unicode code point
typedef uint32 codepoint_t;
const codepoint_t INVALID_CODEPOINT = (codepoint_t)-1;

// Synonym for above, 2^31-1 is used as the limit to protect against addition overflow
typedef uint32 CharCount;
const CharCount MaxCharCount = INT_MAX-1;
// As above, but 2^32-1 is used to signal a 'flag' condition (e.g. undefined)
typedef uint32 CharCountOrFlag;
const CharCountOrFlag CharCountFlag = (CharCountOrFlag)-1;

#define QUOTE(s) #s
#define STRINGIZE(s) QUOTE(s)
#define STRINGIZEW(s) _u(#s)

namespace Js
{
    typedef uint32 LocalFunctionId;
};
