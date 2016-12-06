//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#pragma warning(disable: 4127)  // constant expression for Trace/Assert

#include <map>
#define IfFailedReturn(EXPR) do { hr = (EXPR); if (FAILED(hr)) { return hr; }} while(FALSE)
#define IfFailedGoLabel(expr, label) do { hr = (expr); if (FAILED(hr)) { goto label; } } while (FALSE)
#define IfFailGo(expr) IfFailedGoLabel(hr = (expr), Error)

#define IfFalseGo(expr) do { if(!(expr)) { hr = E_FAIL; goto Error; } } while(0)

#ifdef Assert
#undef Assert
#endif

#ifdef AssertMsg
#undef AssertMsg
#endif

#if defined(DBG)

#define AssertMsg(exp, comment)   \
do { \
if (!(exp)) \
{ \
    fprintf(stderr, "ASSERTION (%s, line %d) %s %s\n", __FILE__, __LINE__, _STRINGIZE(exp), comment); \
    fflush(stderr); \
    DebugBreak(); \
} \
} while (0)
#else
#define AssertMsg(exp, comment) ((void)0)
#endif //defined(DBG)

#define Assert(exp)             AssertMsg(exp, #exp)
#define _JSRT_
#include "chakracore.h"
#include "Core/CommonTypedefs.h"

#include <FileLoadHelpers.h>
