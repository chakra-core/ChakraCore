//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "StdAfx.h"

#define MAX_INPUT_LENGTH 1024

Debugger::Debugger(JsRuntimeHandle debugRuntime)
{
    this->debugRuntime = debugRuntime;
    this->debuggerController = new DebuggerController(debugRuntime);
    this->InstallUtilities();
}

Debugger::~Debugger()
{
    delete this->debuggerController;
    this->debuggerController = nullptr;
}

void Debugger::InstallUtilities()
{
    this->debuggerController->InstallHostCallback(L"GetUserInput", &Debugger::GetUserInput, this);
    this->debuggerController->InstallHostCallback(L"InsertBreakpoint", &Debugger::JsInsertBreakpoint, this);
    this->debuggerController->InstallHostCallback(L"ResumeFromBreakpoint", &Debugger::JsResumeFromBreakpoint, this);
}

void CALLBACK Debugger::JsDiagDebugEventHandler(_In_ JsDiagDebugEvent debugEvent, _In_ JsValueRef eventData, _In_opt_ void* callbackState)
{
    if (eventData != nullptr)
    {
        JsValueRef eventDataJson;
        if (JsNoError == ChakraRTInterface::JsStringifyObject(eventData, &eventDataJson))
        {
            LPCWSTR eventJSON;
            size_t length;
            if (JsNoError == ChakraRTInterface::JsStringToPointer(eventDataJson, &eventJSON, &length))
            {
                debuggerController->ProcessDebugEvent(DebugEventString(debugEvent), eventJSON);
            }
        }
    }
}

JsValueRef CALLBACK Debugger::GetUserInput(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsValueRef retVal = nullptr;

    fwprintf(stdout, L"dbg> ");

    wchar_t line[MAX_INPUT_LENGTH];
    if (fgetws(line, MAX_INPUT_LENGTH, stdin) != nullptr)
    {
        IfJsrtError(ChakraRTInterface::JsPointerToString(line, wcslen(line), &retVal));
    }
    else
    {
        fwprintf(stdout, L"FATAL ERROR");
    }

Error:
    if (retVal == nullptr)
    {
        // Something went wrong - attempt to just return undefined.
        ChakraRTInterface::JsGetUndefinedValue(&retVal);
    }

    return retVal;
}

JsValueRef CALLBACK Debugger::JsInsertBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = S_OK;

    JsValueRef retVal = nullptr;

    void *data;

    IfFailGo(DebuggerScriptWrapper::GetExternalData(callee, &data));

    Debugger *debugger = (Debugger*)data;

    if (argumentCount != 4)
    {
        hr = E_FAIL;
        goto Error;
    }

    // Marshal the arguments;
    ULONG args[3];
    for (int i = 1; i < argumentCount; i++)
    {
        double tmp;
        JsValueRef numValue;
        IfJsrtError(ChakraRTInterface::JsConvertValueToNumber(arguments[i], &numValue));
        IfJsrtError(ChakraRTInterface::JsNumberToDouble(numValue, &tmp));
        args[i - 1] = (ULONG)tmp;
    }

    unsigned int breakpointId = 0;
    IfJsrtError(ChakraRTInterface::JsDiagSetBreakpoint(debugger->debugRuntime, args[0], args[1], args[2], &breakpointId));

    IfJsrtError(ChakraRTInterface::JsDoubleToNumber(breakpointId, &retVal));

Error:
    if (retVal == nullptr)
    {
        // Something went wrong - attempt to just return undefined.
        ChakraRTInterface::JsGetUndefinedValue(&retVal);
    }

    return retVal;
}

JsValueRef CALLBACK Debugger::JsResumeFromBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = S_OK;

    JsValueRef retVal = nullptr;

    if (argumentCount != 2)
    {
        hr = E_FAIL;
        goto Error;
    }

    // Marshal the arguments;
    ULONG args;
    double tmp;
    JsValueRef numValue;
    IfJsrtError(ChakraRTInterface::JsConvertValueToNumber(arguments[1], &numValue));
    IfJsrtError(ChakraRTInterface::JsNumberToDouble(numValue, &tmp));
    args = (ULONG)tmp;

    JsDiagResumeType jsDiagResumeType = JsDiagResumeTypeStepIn;
    if (args == 0)
    {
        jsDiagResumeType = JsDiagResumeTypeStepIn;
    }
    else if (args == 1)
    {
        jsDiagResumeType = JsDiagResumeTypeStepOver;
    }
    else if (args == 2)
    {
        jsDiagResumeType = JsDiagResumeTypeStepOut;
    }

    IfJsrtError(ChakraRTInterface::JsDiagResume(jsDiagResumeType));

Error:
    if (retVal == nullptr)
    {
        // Something went wrong - attempt to just return undefined.
        ChakraRTInterface::JsGetUndefinedValue(&retVal);
    }

    return retVal;
}

LPCWSTR Debugger::DebugEventString(JsDiagDebugEvent debugEvent)
{
    switch (debugEvent)
    {
    case JsDiagDebugEventBreak: return L"Break";
    case JsDiagDebugEventSourceCompilation: return L"SourceCompilation";
    case JsDiagDebugEventCompileError: return L"CompileError";
    }
    return L"UNKNOWN";
}
