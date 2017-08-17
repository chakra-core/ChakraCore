//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef _WIN32
#include "CommonMin.h"
#else
#include "pal.h"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#if defined(PROFILE_RECYCLER_ALLOC) || defined(HEAP_TRACK_ALLOC) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
#include <typeinfo.h>
#endif
#pragma warning(pop)
#endif

// Minimal definitions to use AssertMsg in the PAL
#ifndef _WIN32
#define DbgRaiseAssertionFailure() __builtin_trap()
#define __analysis_assume(x)
#define __in
#define __inout

#ifndef USE_ICU
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
#else
#include <unicode/umachine.h>
#endif

namespace Js
{
class Throw
{
public:
    static bool ReportAssert(const char* fileName, unsigned int lineNumber, const char* error, const char* message);
    static void LogAssert();
};
}

#include <Core/Assertions.h>

namespace PlatformAgnostic
{
    namespace UnicodeText
    {
         namespace Internal
         {
             template <typename CharType>
             int LogicalStringCompareImpl(const CharType* str1, const CharType* str2);
         }
    }
}
#endif
