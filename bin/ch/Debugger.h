//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef _WIN32
static const char controllerScript[] = {
#include "DbgController.js.encoded"
    '\0'
};
#else
#include "DbgController.js.h"
#endif

class Debugger
{
public:
    static Debugger* GetDebugger(JsRuntimeHandle runtime);
    static void CloseDebugger();
    static JsRuntimeHandle GetRuntime();
    bool StartDebugging(JsRuntimeHandle runtime);
    bool StopDebugging(JsRuntimeHandle runtime);
    bool HandleDebugEvent(JsDiagDebugEvent debugEvent, JsValueRef eventData);
    bool CompareOrWriteBaselineFile(LPCSTR fileName);
    bool SourceRunDown();
    bool DumpFunctionPosition(JsValueRef functionPosition);
    bool IsDetached() const { return m_isDetached; }
private:
    Debugger(JsRuntimeHandle runtime);
    ~Debugger();
    bool Initialize();
    JsRuntimeHandle m_runtime;
    JsContextRef m_context;
    bool m_isDetached;
    bool InstallDebugCallbacks(JsValueRef hostDebugObject);
    bool SetBaseline();
    bool SetInspectMaxStringLength();
    bool CallFunction(char const * functionName, JsValueRef *result, JsValueRef arg1 = JS_INVALID_REFERENCE, JsValueRef arg2 = JS_INVALID_REFERENCE);
    bool CallFunctionNoResult(char const * functionName, JsValueRef arg1 = JS_INVALID_REFERENCE, JsValueRef arg2 = JS_INVALID_REFERENCE);
public:
    static void CALLBACK DebugEventHandler(_In_ JsDiagDebugEvent debugEvent, _In_ JsValueRef eventData, _In_opt_ void* callbackState);
    static JsValueRef CALLBACK GetSource(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK SetBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK GetStackTrace(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK GetBreakpoints(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK RemoveBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK SetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK GetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK SetStepType(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK GetScripts(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK GetStackProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK GetProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK GetObjectFromHandle(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK Evaluate(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);

    static Debugger* debugger;
};
