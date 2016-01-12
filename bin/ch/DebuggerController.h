//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

class DebuggerController
{
public:
    DebuggerController(JsRuntimeHandle debugRuntime);
    ~DebuggerController(void);
    HRESULT InstallHostCallback(LPCWSTR propName, JsNativeFunction function, void *data);
    HRESULT ProcessDebugEvent(__in __nullterminated LPCWSTR event, __in __nullterminated LPCWSTR eventJSON);
private:
    DebuggerScriptWrapper *debuggerScriptWrapper;
};
