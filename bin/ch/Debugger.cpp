//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#define MAX_BASELINE_SIZE       (1024*1024*200)

void CHAKRA_CALLBACK Debugger::DebugEventHandler(_In_ JsDiagDebugEvent debugEvent, _In_ JsValueRef eventData, _In_opt_ void* callbackState)
{
    Debugger* debugger = (Debugger*)callbackState;
    debugger->HandleDebugEvent(debugEvent, eventData);
}

JsValueRef Debugger::GetSource(JsValueRef callee, bool isConstructCall, JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
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

JsValueRef Debugger::SetBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
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

JsValueRef Debugger::GetStackTrace(JsValueRef callee, bool isConstructCall, JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    JsValueRef stackInfo = JS_INVALID_REFERENCE;
    IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetStackTrace(&stackInfo));
    return stackInfo;
}

JsValueRef Debugger::GetBreakpoints(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsValueRef breakpoints = JS_INVALID_REFERENCE;
    IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetBreakpoints(&breakpoints));
    return breakpoints;
}

JsValueRef Debugger::RemoveBreakpoint(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
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

JsValueRef Debugger::SetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    int exceptionAttributes;

    if (argumentCount > 1)
    {
        JsValueRef breakOnException;
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsConvertValueToNumber(arguments[1], &breakOnException));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(breakOnException, &exceptionAttributes));

        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagSetBreakOnException(Debugger::GetRuntime(), (JsDiagBreakOnExceptionAttributes)exceptionAttributes));
    }

    return JS_INVALID_REFERENCE;
}

JsValueRef Debugger::GetBreakOnException(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsDiagBreakOnExceptionAttributes exceptionAttributes;
    IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetBreakOnException(Debugger::GetRuntime(), &exceptionAttributes));

    JsValueRef exceptionAttributesRef;
    IfJsErrorFailLogAndRet(ChakraRTInterface::JsDoubleToNumber((double)exceptionAttributes, &exceptionAttributesRef));
    return exceptionAttributesRef;
}

JsValueRef Debugger::SetStepType(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    int stepType;

    if (argumentCount > 1)
    {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[1], &stepType));
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagSetStepType((JsDiagStepType)stepType));
    }

    return JS_INVALID_REFERENCE;
}

JsValueRef Debugger::GetScripts(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsValueRef sourcesList = JS_INVALID_REFERENCE;
    IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagGetScripts(&sourcesList));
    return sourcesList;
}

JsValueRef Debugger::GetStackProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
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

JsValueRef Debugger::GetProperties(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
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

JsValueRef Debugger::GetObjectFromHandle(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
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

JsValueRef Debugger::Evaluate(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    int stackFrameIndex;
    JsValueRef result = JS_INVALID_REFERENCE;

    if (argumentCount > 2)
    {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsNumberToInt(arguments[1], &stackFrameIndex));
        ChakraRTInterface::JsDiagEvaluate(arguments[2], stackFrameIndex, JsParseScriptAttributeNone, /* forceSetValueProp */ false, &result);
    }

    return result;
}

Debugger::Debugger(JsRuntimeHandle runtime)
{
    this->m_runtime = runtime;
    this->m_context = JS_INVALID_REFERENCE;
    this->m_isDetached = true;
}

Debugger::~Debugger()
{
    if (this->m_context != JS_INVALID_REFERENCE)
    {
        ChakraRTInterface::JsRelease(this->m_context, nullptr);
        this->m_context = JS_INVALID_REFERENCE;
    }
    this->m_runtime = JS_INVALID_RUNTIME_HANDLE;
}

Debugger * Debugger::GetDebugger(JsRuntimeHandle runtime)
{
    if (Debugger::debugger == nullptr)
    {
        Debugger::debugger = new Debugger(runtime);
        Debugger::debugger->Initialize();
    }

    return Debugger::debugger;
}

void Debugger::CloseDebugger()
{
    if (Debugger::debugger != nullptr)
    {
        delete Debugger::debugger;
        Debugger::debugger = nullptr;
    }
}

JsRuntimeHandle Debugger::GetRuntime()
{
    return Debugger::debugger != nullptr ? Debugger::debugger->m_runtime : JS_INVALID_RUNTIME_HANDLE;
}

bool Debugger::Initialize()
{
    // Create a new context and run dbgcontroller.js in that context
    // setup dbgcontroller.js callbacks

    Assert(this->m_context == JS_INVALID_REFERENCE);
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCreateContext(this->m_runtime, &this->m_context));
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsAddRef(this->m_context, nullptr)); // Pin context

    AutoRestoreContext autoRestoreContext(this->m_context);

    JsValueRef globalFunc = JS_INVALID_REFERENCE;
    JsValueRef scriptSource;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCreateExternalArrayBuffer(
        (void*)controllerScript, (unsigned int)strlen(controllerScript),
        nullptr, nullptr, &scriptSource));
    JsValueRef fname;
    ChakraRTInterface::JsCreateString(
        "DbgController.js", strlen("DbgController.js"), &fname);
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsParse(scriptSource,
        JS_SOURCE_CONTEXT_NONE, fname, JsParseScriptAttributeLibraryCode,
        &globalFunc));

    JsValueRef undefinedValue;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetUndefinedValue(&undefinedValue));

    JsValueRef args[] = { undefinedValue };
    JsValueRef result = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCallFunction(globalFunc, args, _countof(args), &result));

    JsValueRef globalObj = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetGlobalObject(&globalObj));

    JsPropertyIdRef hostDebugObjectPropId;
    IfJsrtErrorFailLogAndRetFalse(CreatePropertyIdFromString("hostDebugObject", &hostDebugObjectPropId));

    JsPropertyIdRef hostDebugObject;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetProperty(globalObj, hostDebugObjectPropId, &hostDebugObject));

    this->InstallDebugCallbacks(hostDebugObject);

    if (!WScriptJsrt::Initialize())
    {
        return false;
    }

    if (HostConfigFlags::flags.dbgbaselineIsEnabled)
    {
        this->SetBaseline();
    }

    if (HostConfigFlags::flags.InspectMaxStringLengthIsEnabled)
    {
        this->SetInspectMaxStringLength();
    }

    return true;
}

bool Debugger::InstallDebugCallbacks(JsValueRef hostDebugObject)
{
    HRESULT hr = S_OK;
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagGetSource", Debugger::GetSource));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagSetBreakpoint", Debugger::SetBreakpoint));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagGetStackTrace", Debugger::GetStackTrace));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagGetBreakpoints", Debugger::GetBreakpoints));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagRemoveBreakpoint", Debugger::RemoveBreakpoint));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagSetBreakOnException", Debugger::SetBreakOnException));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagGetBreakOnException", Debugger::GetBreakOnException));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagSetStepType", Debugger::SetStepType));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagGetScripts", Debugger::GetScripts));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagGetStackProperties", Debugger::GetStackProperties));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagGetProperties", Debugger::GetProperties));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagGetObjectFromHandle", Debugger::GetObjectFromHandle));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(hostDebugObject, "JsDiagEvaluate", Debugger::Evaluate));
Error:
    return hr != S_OK;
}

bool Debugger::SetBaseline()
{
#ifdef _WIN32
    LPSTR script = nullptr;
    FILE *file = nullptr;
    int numChars = 0;
    HRESULT hr = S_OK;

    if (_wfopen_s(&file, HostConfigFlags::flags.dbgbaseline, _u("rb")) != 0)
    {
        Helpers::LogError(_u("opening baseline file '%s'"), HostConfigFlags::flags.dbgbaseline);
    }

    if(file != nullptr)
    {
        int fileSize = _filelength(_fileno(file));
        if (fileSize <= MAX_BASELINE_SIZE)
        {
            script = new char[fileSize + 1];

            numChars = static_cast<int>(fread(script, sizeof(script[0]), fileSize, file));
            if (numChars == fileSize)
            {
                script[numChars] = '\0';

                JsValueRef wideScriptRef;
                IfJsErrorFailLogAndHR(ChakraRTInterface::JsCreateString(
                  script, strlen(script), &wideScriptRef));

                this->CallFunctionNoResult("SetBaseline", wideScriptRef);
            }
            else
            {
                Helpers::LogError(_u("failed to read from baseline file"));
                IfFailGo(E_FAIL);
            }
        }
        else
        {
            Helpers::LogError(_u("baseline file too large"));
            IfFailGo(E_FAIL);
        }
    }
Error:
    if (script)
    {
        delete[] script;
    }

    if (file)
    {
        fclose(file);
    }

    return hr == S_OK;
#else
    // xplat-todo: Implement this on Linux
    return false;
#endif
}

bool Debugger::SetInspectMaxStringLength()
{
    Assert(HostConfigFlags::flags.InspectMaxStringLength > 0);

    JsValueRef maxStringRef;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsDoubleToNumber(HostConfigFlags::flags.InspectMaxStringLength, &maxStringRef));

    return this->CallFunctionNoResult("SetInspectMaxStringLength", maxStringRef);
}

bool Debugger::CallFunction(char const * functionName, JsValueRef *result, JsValueRef arg1, JsValueRef arg2)
{
    AutoRestoreContext autoRestoreContext(this->m_context);

    // Get the global object
    JsValueRef globalObj = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetGlobalObject(&globalObj));

    // Get a script string for the function name
    JsPropertyIdRef targetFuncId = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(CreatePropertyIdFromString(functionName, &targetFuncId));

    // Get the target function
    JsValueRef targetFunc = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetProperty(globalObj, targetFuncId, &targetFunc));

    static const unsigned short MaxArgs = 2;
    JsValueRef args[MaxArgs + 1];

    // Pass in undefined for 'this'
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetUndefinedValue(&args[0]));

    unsigned short argCount = 1;

    if (arg1 != JS_INVALID_REFERENCE)
    {
        args[argCount++] = arg1;
    }

    Assert(arg2 == JS_INVALID_REFERENCE || argCount != 1);

    if (arg2 != JS_INVALID_REFERENCE)
    {
        args[argCount++] = arg2;
    }

    // Call the function
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCallFunction(targetFunc, args, argCount, result));

    return true;
}

bool Debugger::CallFunctionNoResult(char const * functionName, JsValueRef arg1, JsValueRef arg2)
{
    JsValueRef result = JS_INVALID_REFERENCE;
    return this->CallFunction(functionName, &result, arg1, arg2);
}

bool Debugger::DumpFunctionPosition(JsValueRef functionPosition)
{
    return this->CallFunctionNoResult("DumpFunctionPosition", functionPosition);
}

bool Debugger::StartDebugging(JsRuntimeHandle runtime)
{
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsDiagStartDebugging(runtime, Debugger::DebugEventHandler, this));

    this->m_isDetached = false;

    return true;
}

bool Debugger::StopDebugging(JsRuntimeHandle runtime)
{
    void* callbackState = nullptr;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsDiagStopDebugging(runtime, &callbackState));

    Assert(callbackState == this);

    this->m_isDetached = true;

    return true;
}

bool Debugger::HandleDebugEvent(JsDiagDebugEvent debugEvent, JsValueRef eventData)
{
    JsValueRef debugEventRef;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsDoubleToNumber(debugEvent, &debugEventRef));

    return this->CallFunctionNoResult("HandleDebugEvent", debugEventRef, eventData);
}

bool Debugger::CompareOrWriteBaselineFile(LPCSTR fileName)
{
    AutoRestoreContext autoRestoreContext(this->m_context);

    // Pass in undefined for 'this'
    JsValueRef undefinedRef;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsGetUndefinedValue(&undefinedRef));

    JsValueRef result = JS_INVALID_REFERENCE;

    bool testPassed = false;

    if (HostConfigFlags::flags.dbgbaselineIsEnabled)
    {
        this->CallFunction("Verify", &result);
        JsValueRef numberVal;
        IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsConvertValueToNumber(result, &numberVal));
        int val;
        IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsNumberToInt(numberVal, &val));
        testPassed = (val == 0);
    }

    if (!testPassed)
    {
        this->CallFunction("GetOutputJson", &result);

        AutoString baselineData;
        IfJsrtErrorFailLogAndRetFalse(baselineData.Initialize(result));

        char16 baselineFilename[256];
        swprintf_s(baselineFilename, HostConfigFlags::flags.dbgbaselineIsEnabled ? _u("%S.dbg.baseline.rebase") : _u("%S.dbg.baseline"), fileName);

        FILE *file = nullptr;
        if (_wfopen_s(&file, baselineFilename, _u("wt")) != 0)
        {
            return false;
        }

        if (file != nullptr)
        {
            int countWritten = static_cast<int>(fwrite(baselineData.GetString(), sizeof(char), baselineData.GetLength(), file));
            Assert(baselineData.GetLength() <= INT_MAX);
            if (countWritten != (int)baselineData.GetLength())
            {
                Assert(false);
                return false;
            }

            fclose(file);
        }

        if (!HostConfigFlags::flags.dbgbaselineIsEnabled)
        {
            wprintf(_u("No baseline file specified, baseline created at '%s'\n"), baselineFilename);
        }
        else
        {
            Helpers::LogError(_u("Rebase file created at '%s'\n"), baselineFilename);
        }
    }

    return true;
}

bool Debugger::SourceRunDown()
{
    AutoRestoreContext autoRestoreContext(this->m_context);

    JsValueRef sourcesList = JS_INVALID_REFERENCE;
    IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsDiagGetScripts(&sourcesList));

    return this->CallFunctionNoResult("HandleSourceRunDown", sourcesList);
}
