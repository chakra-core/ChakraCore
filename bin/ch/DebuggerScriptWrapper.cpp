//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "StdAfx.h"

#define EXTERNAL_DATA_PROP L"#callback#"

static const WCHAR debuggerScript[] = {
#include "DebuggerScript.js.encoded"
    L'\0'
};

DebuggerScriptWrapper::DebuggerScriptWrapper(JsRuntimeHandle debugRuntime)
{
    if (debugRuntime == JS_INVALID_RUNTIME_HANDLE)
    {
        IfJsrtError(ChakraRTInterface::JsCreateRuntime(JsRuntimeAttributeDisableBackgroundWork, nullptr, &m_runtime));
    }
    else
    {
        m_runtime = debugRuntime;
    }

    IfJsrtError(ChakraRTInterface::JsCreateContext(m_runtime, &m_context));
    IfJsrtError(ChakraRTInterface::JsSetCurrentContext(m_context));

    InstallUtility();

    IfJsrtError(ChakraRTInterface::JsRunScript(debuggerScript, (DWORD_PTR)-3 /*A large number which can't be address*/, L"DebuggerScript.js", nullptr));

Error:
    return;
}

DebuggerScriptWrapper::~DebuggerScriptWrapper()
{
    ChakraRTInterface::JsSetCurrentContext(nullptr);
    if (this->m_runtime != JS_INVALID_RUNTIME_HANDLE)
    {
        ChakraRTInterface::JsDisposeRuntime(m_runtime);
    }
}

HRESULT DebuggerScriptWrapper::InstallUtility()
{
    HRESULT hr = S_OK;
    IfFailedReturn(InstallHostCallback(L"Echo", &DebuggerScriptWrapper::EchoCallback, nullptr));
    return hr;
}

HRESULT DebuggerScriptWrapper::InstallHostCallback(LPCWSTR propName, JsNativeFunction function, void *data)
{
    HRESULT hr = S_OK;

    // Get the Global object
    JsValueRef globalObj;
    IfJsrtErrorHR(ChakraRTInterface::JsGetGlobalObject(&globalObj));

    // Get the property ID for WScript
    JsPropertyIdRef wscriptId;
    IfJsrtErrorHR(ChakraRTInterface::JsGetPropertyIdFromName(L"WScript", &wscriptId));

    // Check if WScript exists on global
    JsValueRef wscriptObject;
    bool result;
    IfJsrtErrorHR(ChakraRTInterface::JsHasProperty(globalObj, wscriptId, &result));
    if (!result)
    {
        // Create the WScript object
        IfJsrtErrorHR(ChakraRTInterface::JsCreateObject(&wscriptObject));

        // Store on global
        IfJsrtErrorHR(ChakraRTInterface::JsSetProperty(globalObj, wscriptId, wscriptObject, true));
    }
    else
    {
        // Get the WScript object
        IfJsrtErrorHR(ChakraRTInterface::JsGetProperty(globalObj, wscriptId, &wscriptObject));
    }

    // Create the callback function
    JsValueRef jsFunction;
    IfJsrtErrorHR(ChakraRTInterface::JsCreateFunction(function, nullptr, &jsFunction));

    // Create an object to hold the callback's external data
    JsPropertyIdRef dataId;
    JsValueRef dataObj;
    IfJsrtErrorHR(ChakraRTInterface::JsGetPropertyIdFromName(EXTERNAL_DATA_PROP, &dataId));
    IfJsrtErrorHR(ChakraRTInterface::JsCreateExternalObject(data, nullptr, &dataObj));
    IfJsrtErrorHR(ChakraRTInterface::JsSetProperty(jsFunction, dataId, dataObj, true));

    // Create the target property Id
    JsPropertyIdRef newPropId;
    IfJsrtErrorHR(ChakraRTInterface::JsGetPropertyIdFromName(propName, &newPropId));

    // Store the callback function on WScript
    IfJsrtErrorHR(ChakraRTInterface::JsSetProperty(wscriptObject, newPropId, jsFunction, true));


Error:
    return hr;
}

HRESULT DebuggerScriptWrapper::CallGlobalFunction(LPCWSTR function, JsValueRef* result, LPCWSTR arg1, LPCWSTR arg2, LPCWSTR arg3)
{
    JsValueRef globalObj = nullptr;
    JsValueRef targetFunc = nullptr;
    JsPropertyIdRef targetFuncId;

    // Save the current context
    JsContextRef prevContext = nullptr;
    if (ChakraRTInterface::JsGetCurrentContext(&prevContext) != JsNoError)
        return E_FAIL;

    // Set the current context
    if (ChakraRTInterface::JsSetCurrentContext(m_context) != JsNoError)
        return E_FAIL;

    // Get the global object
    if (ChakraRTInterface::JsGetGlobalObject(&globalObj) != JsNoError)
        return E_FAIL;

    // Get a script string for the function name
    if (ChakraRTInterface::JsGetPropertyIdFromName(function, &targetFuncId) != JsNoError)
        return E_FAIL;

    // Get the target function
    if (ChakraRTInterface::JsGetProperty(globalObj, targetFuncId, &targetFunc) != JsNoError)
        return E_FAIL;

    static const int MaxArgs = 3;
    int argCount = 1;
    WCHAR const* args[MaxArgs] = { arg1, arg2, arg3 };
    JsValueRef jsArgs[MaxArgs + 1];

    // Pass in undefined for 'this'
    if (ChakraRTInterface::JsGetUndefinedValue(&jsArgs[0]) != JsNoError)
        return E_FAIL;

    // Marshal the arguments
    for (int i = 0; i < MaxArgs; ++i, argCount++)
    {
        if (args[i] == nullptr)
            break;
        else
        {
            if (ChakraRTInterface::JsPointerToString(args[i], wcslen(args[i]), &jsArgs[i + 1]) != JsNoError)
                return E_FAIL;
        }
    }

    // Call the function
    if (ChakraRTInterface::JsCallFunction(targetFunc, jsArgs, (unsigned short)argCount, result) != JsNoError)
        return E_FAIL;

    // Restore the previous context
    if (ChakraRTInterface::JsSetCurrentContext(prevContext) != JsNoError)
        return E_FAIL;

    return S_OK;
}

HRESULT DebuggerScriptWrapper::GetExternalData(JsValueRef func, void **data)
{
    HRESULT hr = S_OK;

    JsPropertyIdRef dataId;
    JsValueRef dataProp;

    IfJsrtErrorHR(ChakraRTInterface::JsGetPropertyIdFromName(EXTERNAL_DATA_PROP, &dataId));

    IfJsrtErrorHR(ChakraRTInterface::JsGetProperty(func, dataId, &dataProp));

    IfJsrtErrorHR(ChakraRTInterface::JsGetExternalData(dataProp, data));

Error:
    return hr;
}

JsValueRef __stdcall DebuggerScriptWrapper::EchoCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    for (unsigned int i = 1; i < argumentCount; i++)
    {
        if (i > 1)
        {
            wprintf(L" ");
        }
        JsValueRef strValue;
        if (ChakraRTInterface::JsConvertValueToString(arguments[i], &strValue) == JsNoError)
        {
            LPCWSTR str = nullptr;
            size_t length;
            if (ChakraRTInterface::JsStringToPointer(strValue, &str, &length) == JsNoError)
            {
                wprintf(L"%ls", str);
            }
        }
    }

    wprintf(L"\n");

    JsValueRef undefinedValue;
    if (ChakraRTInterface::JsGetUndefinedValue(&undefinedValue) == JsNoError)
    {
        return undefinedValue;
    }
    else
    {
        return nullptr;
    }
}
