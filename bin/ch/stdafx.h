//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if DBG
#else // DBG
#pragma warning(disable: 4189)  // initialized but unused variable (e.g. variable that may only used by assert)
#endif

#define Unused(var) var;

#define IfFailedReturn(EXPR) do { hr = (EXPR); if (FAILED(hr)) { return hr; }} while(FALSE)
#define IfFailedGoLabel(expr, label) do { hr = (expr); if (FAILED(hr)) { goto label; } } while (FALSE)
#define IfFailGo(expr) IfFailedGoLabel(hr = (expr), Error)

#define IfJsrtErrorFail(expr, ret) do { if ((expr) != JsNoError) return ret; } while (0)
#define IfJsrtErrorHR(expr) do { if((expr) != JsNoError) { hr = E_FAIL; goto Error; } } while(0)
#define IfJsrtError(expr) do { if((expr) != JsNoError) { goto Error; } } while(0)
#define IfJsrtErrorSetGo(expr) do { errorCode = (expr); if(errorCode != JsNoError) { hr = E_FAIL; goto Error; } } while(0)
#define IfFalseGo(expr) do { if(!(expr)) { hr = E_FAIL; goto Error; } } while(0)

#define WIN32_LEAN_AND_MEAN 1

#include "CommonDefines.h"
#include <map>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <CommonPal.h>
#endif // _WIN32

#include <stdarg.h>
#ifdef _MSC_VER
#include <stdio.h>
#include <io.h>
#endif // _MSC_VER

#if defined(_DEBUG)
#define _DEBUG_WAS_DEFINED
#undef _DEBUG
#endif

#ifdef _WIN32
#include <atlbase.h>
#endif // _WIN32

#ifdef _DEBUG_WAS_DEFINED
#define _DEBUG
#undef _DEBUG_WAS_DEFINED
#endif

#ifdef Assert
#undef Assert
#endif

#ifdef AssertMsg
#undef AssertMsg
#endif

#if defined(DBG)

#define _STRINGIZE_(x) #x
#if !defined(_STRINGIZE)
#define _STRINGIZE(x) _STRINGIZE_(x)
#endif

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
#include "ChakraCore.h"
#include "Core/CommonTypedefs.h"
#include "TestHooksRt.h"

typedef void * Var;

#include "Codex/Utf8Helper.h"
using utf8::NarrowStringToWideDynamic;
using utf8::WideStringToNarrowDynamic;
#include "Helpers.h"

#define IfJsErrorFailLog(expr) \
do { \
    JsErrorCode jsErrorCode = expr; \
    if ((jsErrorCode) != JsNoError) { \
        fwprintf(stderr, _u("ERROR: ") _u(#expr) _u(" failed. JsErrorCode=0x%x (%s)\n"), jsErrorCode, Helpers::JsErrorCodeToString(jsErrorCode)); \
        fflush(stderr); \
        goto Error; \
    } \
} while (0)

#define IfJsErrorFailLogAndRet(expr) \
do { \
    JsErrorCode jsErrorCode = expr; \
    if ((jsErrorCode) != JsNoError) { \
        fwprintf(stderr, _u("ERROR: ") _u(#expr) _u(" failed. JsErrorCode=0x%x (%s)\n"), jsErrorCode, Helpers::JsErrorCodeToString(jsErrorCode)); \
        fflush(stderr); \
        Assert(false); \
        return JS_INVALID_REFERENCE; \
    } \
} while (0)

#define IfJsrtErrorFailLogAndRetFalse(expr) \
do { \
    JsErrorCode jsErrorCode = expr; \
    if ((jsErrorCode) != JsNoError) { \
        fwprintf(stderr, _u("ERROR: ") _u(#expr) _u(" failed. JsErrorCode=0x%x (%s)\n"), jsErrorCode, Helpers::JsErrorCodeToString(jsErrorCode)); \
        fflush(stderr); \
        Assert(false); \
        return false; \
    } \
} while (0)

#define IfJsrtErrorFailLogAndRetErrorCode(expr) \
do { \
    JsErrorCode jsErrorCode = expr; \
    if ((jsErrorCode) != JsNoError) { \
        fwprintf(stderr, _u("ERROR: ") _u(#expr) _u(" failed. JsErrorCode=0x%x (%s)\n"), jsErrorCode, Helpers::JsErrorCodeToString(jsErrorCode)); \
        fflush(stderr); \
        return (jsErrorCode); \
    } \
} while (0)

#ifndef ENABLE_TEST_HOOKS
#define ENABLE_TEST_HOOKS
#endif

#include "TestHooks.h"
#include "ChakraRtInterface.h"
#include "HostConfigFlags.h"
#include "MessageQueue.h"
#include "WScriptJsrt.h"
#include "Debugger.h"

#ifdef _WIN32
#include <strsafe.h>
#include "JITProcessManager.h"
#endif

template<class T, bool JSRTHeap>
class AutoStringPtr
{
    T* data;
public:
    AutoStringPtr():data(nullptr) { }
    ~AutoStringPtr()
    {
        if (data == nullptr)
        {
            return;
        }

        if (JSRTHeap)
        {
            ChakraRTInterface::JsStringFree((char*)data);
        }
        else
        {
            free(data);
        }
    }

    T* operator*() { return data; }
    T** operator&()  { return &data; }
};

typedef AutoStringPtr<char, true> AutoString;
typedef AutoStringPtr<wchar_t, false> AutoWideString;
