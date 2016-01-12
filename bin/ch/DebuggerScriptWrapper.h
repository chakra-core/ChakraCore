//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

class DebuggerScriptWrapper
{
public:
    DebuggerScriptWrapper(JsRuntimeHandle debugRuntime);
    ~DebuggerScriptWrapper(void);
    HRESULT InstallHostCallback(LPCWSTR propName, JsNativeFunction function, void *data);
    HRESULT CallGlobalFunction(LPCWSTR function, JsValueRef *result, LPCWSTR arg1 = nullptr, LPCWSTR arg2 = nullptr, LPCWSTR arg3 = nullptr);
    static JsValueRef CALLBACK EchoCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState);
    static HRESULT GetExternalData(JsValueRef func, void **data);
private:
    HRESULT InstallUtility();
    JsRuntimeHandle m_runtime;
    JsContextRef m_context;
};
