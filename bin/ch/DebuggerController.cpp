//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "StdAfx.h"

DebuggerController::DebuggerController(JsRuntimeHandle debugRuntime)
{
    this->debuggerScriptWrapper = new DebuggerScriptWrapper(debugRuntime);
}

DebuggerController::~DebuggerController()
{
    delete this->debuggerScriptWrapper;
    this->debuggerScriptWrapper = nullptr;
}

HRESULT DebuggerController::InstallHostCallback(LPCWSTR propName, JsNativeFunction function, void *data)
{
    return this->debuggerScriptWrapper->InstallHostCallback(propName, function, data);
}

HRESULT DebuggerController::ProcessDebugEvent(__in __nullterminated LPCWSTR event, __in __nullterminated LPCWSTR eventJSON)
{
    return this->debuggerScriptWrapper->CallGlobalFunction(L"ProcessDebugEvent", NULL, event, eventJSON);
}
