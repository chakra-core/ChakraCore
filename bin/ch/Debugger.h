//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

static const WCHAR controllerScript[] = {
#include "DbgController.js.encoded"
    L'\0'
};

class Debugger
{
public:
    static Debugger* GetDebugger(JsRuntimeHandle runtime);
    static void CloseDebugger();
    void StartDebugging(JsRuntimeHandle runtime);
    bool HandleDebugEvent(JsDiagDebugEvent debugEvent, JsValueRef eventData);
    bool VerifyAndWriteNewBaselineFile(LPCWSTR fileName);
    bool SourceRunDown();
private:
    Debugger();
    ~Debugger();
    bool Initialize(JsRuntimeHandle runtime);
    JsContextRef m_context;
    bool InstallDebugCallbacks(JsValueRef hostDebugObject);
    bool InstallHostCallback(JsValueRef hostDebugObject, const wchar_t *name, JsNativeFunction nativeFunction);
    bool CallFunction(wchar_t const * functionName, JsValueRef* arguments, unsigned short argumentCount, JsValueRef *result);
public:
    static void CALLBACK JsDiagDebugEventHandler(_In_ JsDiagDebugEvent debugEvent, _In_ JsValueRef eventData, _In_opt_ void* callbackState);
    static JsValueRef CALLBACK JsDiagGetSource(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagSetBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetStackTrace(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagRequestAsyncBreak(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetBreakpoints(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagRemoveBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagSetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagSetStepType(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetScripts(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetFunctionPosition(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetStackProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagGetObjectFromHandle(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsDiagEvaluate(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);

    static Debugger* debugger;
};
