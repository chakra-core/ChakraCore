//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

class Debugger
{
public:
    Debugger(JsRuntimeHandle debugRuntime);
    ~Debugger(void);
    static void CALLBACK JsDiagDebugEventHandler(_In_ JsDiagDebugEvent debugEvent, _In_ JsValueRef eventData, _In_opt_ void* callbackState);

    static JsValueRef CALLBACK GetUserInput(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsInsertBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static JsValueRef CALLBACK JsResumeFromBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);

    static LPCWSTR DebugEventString(JsDiagDebugEvent debugEvent);
private:
    static DebuggerController* debuggerController;
    JsRuntimeHandle debugRuntime;
    void InstallUtilities();
};
