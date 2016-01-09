//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Banned.h"
#include "CommonDefines.h"
#define _CRT_RAND_S         // Enable rand_s in the CRT

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4995) /* 'function': name was marked as #pragma deprecated */

// === Windows Header Files ===
#define INC_OLE2                 /* for windows.h */
#define CONST_VTABLE             /* for objbase.h */
#include <windows.h>

/* Don't want GetObject and GetClassName to be defined to GetObjectW and GetClassNameW */
#undef GetObject
#undef GetClassName
#undef Yield /* winbase.h defines this but we want to use it for Js::OpCode::Yield; it is Win16 legacy, no harm undef'ing it */
#pragma warning(pop)

typedef wchar_t wchar16;

// xplat-todo: get a better name for this macro
#define CH_WSTR(s) L##s

#else
#include "inc/pal.h"
#include "inc/rt/palrt.h"
#include "inc/rt/no_sal2.h"

typedef char16_t wchar16;
#define CH_WSTR(s) u##s

// SAL.h doesn't define these if PAL_STDCPP_COMPAT is defined
// Apparently, some C++ headers will conflict with this-
// not sure which ones but stubbing them out for now in linux-
// we can revisit if we do hit a conflict
#define __in
#define __out

#endif

// === core Header Files ===
#include "core/api.h"
#include "core/CommonTypedefs.h"
#include "core/CriticalSection.h"
#include "core/Assertions.h"

// === Exceptions Header Files ===
#include "Exceptions/Throw.h"
#include "Exceptions/ExceptionCheck.h"
#include "Exceptions/reporterror.h"
