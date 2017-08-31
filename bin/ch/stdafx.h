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
#define IfJsrtErrorHRLabel(expr, label) do { if((expr) != JsNoError) { hr = E_FAIL; goto label; } } while(0)
#define IfJsrtError(expr) do { if((expr) != JsNoError) { goto Error; } } while(0)
#define IfJsrtErrorSetGo(expr) do { errorCode = (expr); if(errorCode != JsNoError) { hr = E_FAIL; goto Error; } } while(0)
#define IfJsrtErrorSetGoLabel(expr, label) do { errorCode = (expr); if(errorCode != JsNoError) { hr = E_FAIL; goto label; } } while(0)
#define IfFalseGo(expr) do { if(!(expr)) { hr = E_FAIL; goto Error; } } while(0)
#define IfFalseGoLabel(expr, label) do { if(!(expr)) { hr = E_FAIL; goto label; } } while(0)

#include "CommonDefines.h"
#include <map>
#include <string>

#include <CommonPal.h>

#include <stdarg.h>
#ifdef _MSC_VER
#include <stdio.h>
#include <io.h>
#endif // _MSC_VER

#if defined(_DEBUG)
#define _DEBUG_WAS_DEFINED
#undef _DEBUG
#endif

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

#include "PlatformAgnostic/SystemInfo.h"

#define IfJsErrorFailLog(expr) \
do { \
    JsErrorCode jsErrorCode = expr; \
    if ((jsErrorCode) != JsNoError) { \
        fwprintf(stderr, _u("ERROR: ") _u(#expr) _u(" failed. JsErrorCode=0x%x (%s)\n"), jsErrorCode, Helpers::JsErrorCodeToString(jsErrorCode)); \
        fflush(stderr); \
        goto Error; \
    } \
} while (0)

#define IfJsErrorFailLogAndHR(expr) \
do { \
    JsErrorCode jsErrorCode = expr; \
    if ((jsErrorCode) != JsNoError) { \
        hr = E_FAIL; \
        fwprintf(stderr, _u("ERROR: ") _u(#expr) _u(" failed. JsErrorCode=0x%x (%s)\n"), jsErrorCode, Helpers::JsErrorCodeToString(jsErrorCode)); \
        fflush(stderr); \
        goto Error; \
    } \
} while (0)

#define IfJsErrorFailLogLabel(expr, label) \
do { \
    JsErrorCode jsErrorCode = expr; \
    if ((jsErrorCode) != JsNoError) { \
        fwprintf(stderr, _u("ERROR: ") _u(#expr) _u(" failed. JsErrorCode=0x%x (%s)\n"), jsErrorCode, Helpers::JsErrorCodeToString(jsErrorCode)); \
        fflush(stderr); \
        goto label; \
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
#include "RuntimeThreadData.h"
#include "WScriptJsrt.h"
#include "Debugger.h"

#ifdef _WIN32
#include <strsafe.h>
#include "JITProcessManager.h"
#endif

class AutoString
{
    size_t length;
    char* data;
    LPWSTR data_wide;
    JsErrorCode errorCode;
    bool dontFree;
public:
    AutoString():length(0), data(nullptr),
        data_wide(nullptr), errorCode(JsNoError), dontFree(false)
    { }

    AutoString(JsValueRef value):length(0), data(nullptr),
        data_wide(nullptr), errorCode(JsNoError), dontFree(false)
    {
        Initialize(value);
    }

    JsErrorCode Initialize(JsValueRef value)
    {
        JsValueRef strValue;
        JsValueType type;
        ChakraRTInterface::JsGetValueType(value, &type);
        if (type != JsString)
        {
            errorCode = ChakraRTInterface::JsConvertValueToString(value, &strValue);
        }
        else
        {
            strValue = value;
        }
        int strLen = 0;
        size_t actualLen = 0;
        if (errorCode == JsNoError)
        {
            errorCode = ChakraRTInterface::JsGetStringLength(strValue, &strLen);
            if (errorCode == JsNoError)
            {
                // Assume ascii characters
                data = (char*)malloc((strLen + 1) * sizeof(char));
                errorCode = ChakraRTInterface::JsCopyString(strValue, data, strLen, &length, &actualLen);
                if (errorCode == JsNoError)
                {
                    // If non-ascii, take slow path
                    if (length != actualLen)
                    {
                        free(data);
                        data = (char*)malloc((actualLen + 1) * sizeof(char));

                        errorCode = ChakraRTInterface::JsCopyString(strValue, data, actualLen + 1, &length, nullptr);
                        if (errorCode == JsNoError)
                        {
                            AssertMsg(actualLen == length, "If you see this message.. There is something seriously wrong. Good Luck!");
                            
                        }
                    }
                }
            }
        }
        if (errorCode == JsNoError)
        {
            *(data + actualLen) = char(0);
        }
        return errorCode;
    }

    void MakePersistent()
    {
        dontFree = true;
    }

    LPCSTR GetString()
    {
        return data;
    }

    LPWSTR GetWideString()
    {
        if(data_wide || !data)
        {
            return data_wide;
        }
        NarrowStringToWideDynamic(data, &data_wide);
        return data_wide;
    }

    bool HasError()
    {
        return errorCode != JsNoError;
    }

    JsErrorCode GetError()
    {
        return errorCode;
    }

    size_t GetLength()
    {
        return length;
    }

    ~AutoString()
    {
        // we need persistent source string
        // for externalArrayBuffer source
        // externalArrayBuffer finalize should
        // free this memory
        if (!dontFree && data != nullptr)
        {
            free(data);
            data = nullptr;
        }

        // Free this anyway.
        if (data_wide != nullptr)
        {
            free(data_wide);
            data_wide = nullptr;
        }
    }

    char* operator*() { return data; }
    char** operator&()  { return &data; }
};

inline JsErrorCode CreatePropertyIdFromString(const char* str, JsPropertyIdRef *Id)
{
    return ChakraRTInterface::JsCreatePropertyId(str, strlen(str), Id);
}

void GetBinaryPathWithFileNameA(char *path, const size_t buffer_size, const char* filename);
extern "C" HRESULT __stdcall OnChakraCoreLoadedEntry(TestHooks& testHooks);
