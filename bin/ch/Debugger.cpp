//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

void CALLBACK Debugger::JsDiagDebugEventHandler(_In_ JsDiagDebugEvent debugEvent, _In_ JsValueRef eventData, _In_opt_ void* callbackState)
{
    Debugger* debugger = (Debugger*)callbackState;
    debugger->HandleDebugEvent(debugEvent, eventData);
}

JsValueRef Debugger::JsDiagGetSource(JsValueRef callee, bool isConstructCall, JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    int scriptId;
    JsValueRef source = JS_INVALID_REFERENCE;

    if (argumentCount > 1)
    {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[1], &scriptId));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetSource(scriptId, &source));
    }

    return source;
}

JsValueRef Debugger::JsDiagSetBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    int scriptId;
    int line;
    int column;
    JsValueRef bpObject = JS_INVALID_REFERENCE;

    if (argumentCount > 3)
    {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[1], &scriptId));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[2], &line));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[3], &column));

        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagSetBreakpoint(scriptId, line, column, &bpObject));
    }

    return bpObject;
}

JsValueRef Debugger::JsDiagGetStackTrace(JsValueRef callee, bool isConstructCall, JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    JsValueRef stackInfo = JS_INVALID_REFERENCE;
    IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetStackTrace(&stackInfo));
    return stackInfo;
}

JsValueRef Debugger::JsDiagRequestAsyncBreak(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    return JS_INVALID_REFERENCE;
}

JsValueRef Debugger::JsDiagGetBreakpoints(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsValueRef breakPoints = JS_INVALID_REFERENCE;
    IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetBreakpoints(&breakPoints));
    return breakPoints;
}

JsValueRef Debugger::JsDiagRemoveBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    int bpId;
    if (argumentCount > 1)
    {
        JsValueRef numberValue;
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsConvertValueToNumber(arguments[1], &numberValue));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(numberValue, &bpId));

        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagRemoveBreakpoint(bpId));
    }

    return JS_INVALID_REFERENCE;
}

JsValueRef Debugger::JsDiagSetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    int breakOnExceptionType;

    if (argumentCount > 1)
    {
        JsValueRef breakOnException;
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsConvertValueToNumber(arguments[1], &breakOnException));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(breakOnException, &breakOnExceptionType));

        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagSetBreakOnException((JsDiagBreakOnExceptionType)breakOnExceptionType));
    }

    return JS_INVALID_REFERENCE;
}

JsValueRef Debugger::JsDiagGetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    return JS_INVALID_REFERENCE;
}

JsValueRef Debugger::JsDiagSetStepType(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    int stepType;

    if (argumentCount > 1)
    {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[1], &stepType));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagSetStepType((JsDiagStepType)stepType));
    }

    return JS_INVALID_REFERENCE;
}

JsValueRef Debugger::JsDiagGetScripts(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsValueRef sourcesList = JS_INVALID_REFERENCE;
    IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetScripts(&sourcesList));
    return sourcesList;
}

JsValueRef Debugger::JsDiagGetFunctionPosition(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    return JS_INVALID_REFERENCE;
}

JsValueRef Debugger::JsDiagGetStackProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsValueRef properties = JS_INVALID_REFERENCE;
    int stackFrameIndex;

    if (argumentCount > 1)
    {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[1], &stackFrameIndex));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetStackProperties(stackFrameIndex, &properties));
    }

    return properties;
}

JsValueRef Debugger::JsDiagGetProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsValueRef properties = JS_INVALID_REFERENCE;
    int objectHandle;
    int fromCount;
    int totalCount;

    if (argumentCount > 3)
    {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[1], &objectHandle));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[2], &fromCount));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[3], &totalCount));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetProperties(objectHandle, fromCount, totalCount, &properties));
    }

    return properties;
}

JsValueRef Debugger::JsDiagGetObjectFromHandle(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsValueRef properties = JS_INVALID_REFERENCE;
    int objectHandle;

    if (argumentCount > 1)
    {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[1], &objectHandle));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetObjectFromHandle(objectHandle, &properties));
    }

    return properties;
}

JsValueRef Debugger::JsDiagEvaluate(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    int stackFrameIndex;
    JsValueRef result = JS_INVALID_REFERENCE;

    if (argumentCount > 2) {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[1], &stackFrameIndex));

        LPCWSTR str = nullptr;
        size_t length;
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsValueToWchar(arguments[2], &str, &length));

        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagEvaluate(str, stackFrameIndex, &result));
    }

    return result;
}

Debugger::Debugger()
{
    this->m_context = JS_INVALID_REFERENCE;
}

Debugger::~Debugger()
{
}

Debugger * Debugger::GetDebugger(JsRuntimeHandle runtime)
{
    if (Debugger::debugger == nullptr)
    {
        Debugger::debugger = new Debugger();
        Debugger::debugger->Initialize(runtime);
    }

    return Debugger::debugger;
}

bool Debugger::Initialize(JsRuntimeHandle runtime)
{
    // Create a new context and run dbgcontroller.js in that context
    // setup dbgcontroller.js callbacks
    // put runtime in debug mode

    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCreateContext(runtime, &m_context));
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetCurrentContext(m_context));

    JsValueRef globalFunc = JS_INVALID_REFERENCE;

    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsParseScriptWithFlags(controllerScript, JS_SOURCE_CONTEXT_NONE, _u("DbgController.js"), JsParseScriptAttributeLibraryCode, &globalFunc));

    JsValueRef undefinedValue;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetUndefinedValue(&undefinedValue));
    JsValueRef args[] = { undefinedValue };
    JsValueRef result = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCallFunction(globalFunc, args, _countof(args), &result));

    JsValueRef globalObj = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetGlobalObject(&globalObj));

    JsPropertyIdRef hostDebugObjectPropId;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetPropertyIdFromName(_u("hostDebugObject"), &hostDebugObjectPropId));

    JsPropertyIdRef hostDebugObject;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetProperty(globalObj, hostDebugObjectPropId, &hostDebugObject));

    this->InstallDebugCallbacks(hostDebugObject);

    if (!WScriptJsrt::Initialize())
    {
        return false;
    }

    return true;
}

bool Debugger::InstallDebugCallbacks(JsValueRef hostDebugObject)
{
    HRESULT hr = S_OK;
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagGetSource"), Debugger::JsDiagGetSource));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagSetBreakpoint"), Debugger::JsDiagSetBreakpoint));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagGetStackTrace"), Debugger::JsDiagGetStackTrace));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagRequestAsyncBreak"), Debugger::JsDiagRequestAsyncBreak));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagGetBreakpoints"), Debugger::JsDiagGetBreakpoints));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagRemoveBreakpoint"), Debugger::JsDiagRemoveBreakpoint));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagSetBreakOnException"), Debugger::JsDiagSetBreakOnException));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagGetBreakOnException"), Debugger::JsDiagGetBreakOnException));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagSetStepType"), Debugger::JsDiagSetStepType));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagGetScripts"), Debugger::JsDiagGetScripts));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagGetFunctionPosition"), Debugger::JsDiagGetFunctionPosition));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagGetStackProperties"), Debugger::JsDiagGetStackProperties));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagGetProperties"), Debugger::JsDiagGetProperties));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagGetObjectFromHandle"), Debugger::JsDiagGetObjectFromHandle));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, _u("JsDiagEvaluate"), Debugger::JsDiagEvaluate));
Error:
    return hr != S_OK;
}

bool Debugger::InstallHostCallback(JsValueRef hostDebugObject, const wchar_t * name, JsNativeFunction nativeFunction)
{
    
    JsPropertyIdRef propertyIdRef;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetPropertyIdFromName(name, &propertyIdRef));

    JsValueRef funcRef;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCreateFunction(nativeFunction, nullptr, &funcRef));

    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetProperty(hostDebugObject, propertyIdRef, funcRef, true));

    return true;
}

bool Debugger::CallFunction(wchar_t const * functionName, JsValueRef * arguments, unsigned short argumentCount, JsValueRef * result)
{
    JsValueRef globalObj = JS_INVALID_REFERENCE;
    JsValueRef targetFunc = JS_INVALID_REFERENCE;
    JsPropertyIdRef targetFuncId = JS_INVALID_REFERENCE;

    // Save the current context
    JsContextRef prevContext = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetCurrentContext(&prevContext));

    // Set the current context
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetCurrentContext(m_context));

    // Get the global object
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetGlobalObject(&globalObj));

    // Get a script string for the function name
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetPropertyIdFromName(functionName, &targetFuncId));

    // Get the target function
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetProperty(globalObj, targetFuncId, &targetFunc));

    // Call the function
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCallFunction(targetFunc, arguments, argumentCount, result));

    // Restore the previous context
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetCurrentContext(prevContext));

    return true;
}

void Debugger::StartDebugging(JsRuntimeHandle runtime)
{
    ChakraRTInterface::JsDiagStartDebugging(runtime, Debugger::JsDiagDebugEventHandler, this);
}

bool Debugger::HandleDebugEvent(JsDiagDebugEvent debugEvent, JsValueRef eventData)
{
    // Save the current context
    JsContextRef prevContext = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetCurrentContext(&prevContext));

    // Set the current context
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetCurrentContext(m_context));

    // Pass in undefined for 'this'
    JsValueRef undefinedRef;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetUndefinedValue(&undefinedRef));

    JsValueRef debugEventRef;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsDoubleToNumber(debugEvent, &debugEventRef));

    JsValueRef args[] = { undefinedRef, debugEventRef, eventData };

    JsValueRef result = JS_INVALID_REFERENCE;

    this->CallFunction(_u("HandleDebugEvent"), args, _countof(args), &result);

    // Restore the previous context
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetCurrentContext(prevContext));

    return true;
}

bool Debugger::VerifyAndWriteNewBaselineFile(LPCWSTR fileName)
{
    // Save the current context
    JsContextRef prevContext = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetCurrentContext(&prevContext));

    // Set the current context
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetCurrentContext(m_context));

    // Pass in undefined for 'this'
    JsValueRef undefinedRef;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetUndefinedValue(&undefinedRef));

    JsValueRef args[] = { undefinedRef };

    JsValueRef result = JS_INVALID_REFERENCE;

    this->CallFunction(_u("GetOutputJson"), args, _countof(args), &result);

    LPCWSTR baselineData;
    size_t baselineDataLength;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsStringToPointer(result, &baselineData, &baselineDataLength));

#pragma warning(disable: 38021) // From MSDN: For the code page UTF-8 dwFlags must be set to either 0 or WC_ERR_INVALID_CHARS. Otherwise, the function fails with ERROR_INVALID_FLAGS.
    int multiByteDataLength = WideCharToMultiByte(CP_UTF8, 0, baselineData, -1, NULL, 0, NULL, NULL);
    LPSTR baselineDataANSI = new char[multiByteDataLength];
    if (WideCharToMultiByte(CP_UTF8, 0, baselineData, -1, baselineDataANSI, multiByteDataLength, NULL, NULL) == 0)
    {
#pragma warning(default: 38021) // From MSDN: For the code page UTF-8 dwFlags must be set to either 0 or WC_ERR_INVALID_CHARS. Otherwise, the function fails with ERROR_INVALID_FLAGS.
        return false;
    }

    wchar_t baselineFilename[256];
    swprintf_s(baselineFilename, _countof(baselineFilename), _u("%s.dbg.baseline"), fileName);

    FILE *file = nullptr;
    if (_wfopen_s(&file, baselineFilename, _u("wt")) != 0)
    {
        return false;
    }

    int countWritten = static_cast<int>(fwrite(baselineDataANSI, sizeof(baselineDataANSI[0]), strlen(baselineDataANSI), file));
    if (countWritten != (int)strlen(baselineDataANSI))
    {
        return false;
    }

    fclose(file);

    // Restore the previous context
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetCurrentContext(prevContext));

    return true;
}

bool Debugger::SourceRunDown()
{
    JsValueRef sourcesList = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsDiagGetScripts(&sourcesList));

    // Save the current context
    JsContextRef prevContext = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetCurrentContext(&prevContext));

    // Set the current context
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetCurrentContext(m_context));

    // Pass in undefined for 'this'
    JsValueRef undefinedRef;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetUndefinedValue(&undefinedRef));

    JsValueRef args[] = { undefinedRef, sourcesList };

    JsValueRef result = JS_INVALID_REFERENCE;

    this->CallFunction(_u("HandleSourceRunDown"), args, _countof(args), &result);

    // Restore the previous context
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsSetCurrentContext(prevContext));

    return true;
}

void Debugger::CloseDebugger()
{
    if (Debugger::debugger != nullptr)
    {
        delete Debugger::debugger;
        Debugger::debugger = nullptr;
    }
}
