//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
typedef wchar_t wchar;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

typedef signed char sbyte;

#ifdef _WIN32
typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;
#else
typedef __int8_t int8;
typedef __int16_t int16;
typedef __int32_t int32;
typedef __int64_t int64;
#endif

typedef unsigned char byte;
#ifdef _WIN32
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
#else
typedef __uint8_t uint8;
typedef __uint16_t uint16;
typedef __uint32_t uint32;
typedef __uint64_t uint64;

#endif

#if defined (_WIN64)
typedef __int64 intptr;
typedef unsigned __int64 uintptr;
#else
#ifdef _WIN32
typedef __int32 intptr;
typedef unsigned __int32 uintptr;
#else
typedef int64 intptr;
typedef uint64 uintptr;
typedef uint64 UINT_PTR
#endif
#endif

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
#define STRINGIZEW(s) TEXT(QUOTE(s))

namespace Js
{
    typedef uint32 LocalFunctionId;
};

