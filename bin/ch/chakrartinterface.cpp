//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "StdAfx.h"

LPCWSTR chakraDllName = L"chakracore.dll";

bool ChakraRTInterface::m_testHooksSetup = false;
bool ChakraRTInterface::m_testHooksInitialized = false;
bool ChakraRTInterface::m_usageStringPrinted = false;

ChakraRTInterface::ArgInfo ChakraRTInterface::m_argInfo = { 0 };
TestHooks ChakraRTInterface::m_testHooks = { 0 };
JsAPIHooks ChakraRTInterface::m_jsApiHooks = { 0 };

/*static*/
HINSTANCE ChakraRTInterface::LoadChakraDll(ArgInfo& argInfo)
{
    m_argInfo = argInfo;

    wchar_t filename[_MAX_PATH];
    wchar_t drive[_MAX_DRIVE];
    wchar_t dir[_MAX_DIR];

    wchar_t modulename[_MAX_PATH];
    GetModuleFileName(NULL, modulename, _MAX_PATH);
    _wsplitpath_s(modulename, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
    _wmakepath_s(filename, drive, dir, chakraDllName, nullptr);
    LPCWSTR dllName = filename;

    HINSTANCE library = LoadLibraryEx(dllName, nullptr, 0);
    if (library == nullptr)
    {
        int ret = GetLastError();
        fwprintf(stderr, L"FATAL ERROR: Unable to load %ls GetLastError=0x%x\n", chakraDllName, ret);
        return nullptr;
    }

    if (m_usageStringPrinted)
    {
        UnloadChakraDll(library);
        return nullptr;
    }

    m_jsApiHooks.pfJsrtCreateRuntime = (JsAPIHooks::JsrtCreateRuntimePtr)GetProcAddress(library, "JsCreateRuntime");
    m_jsApiHooks.pfJsrtCreateContext = (JsAPIHooks::JsrtCreateContextPtr)GetProcAddress(library, "JsCreateContext");
    m_jsApiHooks.pfJsrtSetCurrentContext = (JsAPIHooks::JsrtSetCurrentContextPtr)GetProcAddress(library, "JsSetCurrentContext");
    m_jsApiHooks.pfJsrtGetCurrentContext = (JsAPIHooks::JsrtGetCurrentContextPtr)GetProcAddress(library, "JsGetCurrentContext");
    m_jsApiHooks.pfJsrtDisposeRuntime = (JsAPIHooks::JsrtDisposeRuntimePtr)GetProcAddress(library, "JsDisposeRuntime");
    m_jsApiHooks.pfJsrtCreateObject = (JsAPIHooks::JsrtCreateObjectPtr)GetProcAddress(library, "JsCreateObject");
    m_jsApiHooks.pfJsrtCreateExternalObject = (JsAPIHooks::JsrtCreateExternalObjectPtr)GetProcAddress(library, "JsCreateExternalObject");
    m_jsApiHooks.pfJsrtCreateFunction = (JsAPIHooks::JsrtCreateFunctionPtr)GetProcAddress(library, "JsCreateFunction");
    m_jsApiHooks.pfJsrtCreateNameFunction = (JsAPIHooks::JsCreateNamedFunctionPtr)GetProcAddress(library, "JsCreateNamedFunction");
    m_jsApiHooks.pfJsrtPointerToString = (JsAPIHooks::JsrtPointerToStringPtr)GetProcAddress(library, "JsPointerToString");
    m_jsApiHooks.pfJsrtSetProperty = (JsAPIHooks::JsrtSetPropertyPtr)GetProcAddress(library, "JsSetProperty");
    m_jsApiHooks.pfJsrtGetGlobalObject = (JsAPIHooks::JsrtGetGlobalObjectPtr)GetProcAddress(library, "JsGetGlobalObject");
    m_jsApiHooks.pfJsrtGetUndefinedValue = (JsAPIHooks::JsrtGetUndefinedValuePtr)GetProcAddress(library, "JsGetUndefinedValue");
    m_jsApiHooks.pfJsrtGetTrueValue = (JsAPIHooks::JsrtGetUndefinedValuePtr)GetProcAddress(library, "JsGetTrueValue");
    m_jsApiHooks.pfJsrtGetFalseValue = (JsAPIHooks::JsrtGetUndefinedValuePtr)GetProcAddress(library, "JsGetFalseValue");
    m_jsApiHooks.pfJsrtConvertValueToString = (JsAPIHooks::JsrtConvertValueToStringPtr)GetProcAddress(library, "JsConvertValueToString");
    m_jsApiHooks.pfJsrtConvertValueToNumber = (JsAPIHooks::JsrtConvertValueToNumberPtr)GetProcAddress(library, "JsConvertValueToNumber");
    m_jsApiHooks.pfJsrtConvertValueToBoolean = (JsAPIHooks::JsrtConvertValueToBooleanPtr)GetProcAddress(library, "JsConvertValueToBoolean");
    m_jsApiHooks.pfJsrtConvertValueToObject = (JsAPIHooks::JsrtConvertValueToObjectPtr)GetProcAddress(library, "JsConvertValueToObject");
    m_jsApiHooks.pfJsrtStringToPointer = (JsAPIHooks::JsrtStringToPointerPtr)GetProcAddress(library, "JsStringToPointer");
    m_jsApiHooks.pfJsrtBooleanToBool = (JsAPIHooks::JsrtBooleanToBoolPtr)GetProcAddress(library, "JsBooleanToBool");
    m_jsApiHooks.pfJsrtGetPropertyIdFromName = (JsAPIHooks::JsrtGetPropertyIdFromNamePtr)GetProcAddress(library, "JsGetPropertyIdFromName");
    m_jsApiHooks.pfJsrtGetProperty = (JsAPIHooks::JsrtGetPropertyPtr)GetProcAddress(library, "JsGetProperty");
    m_jsApiHooks.pfJsrtHasProperty = (JsAPIHooks::JsrtHasPropertyPtr)GetProcAddress(library, "JsHasProperty");
    m_jsApiHooks.pfJsrtParseScriptWithFlags = (JsAPIHooks::JsrtParseScriptWithFlagsPtr)GetProcAddress(library, "JsParseScriptWithFlags");
    m_jsApiHooks.pfJsrtRunScript = (JsAPIHooks::JsrtRunScriptPtr)GetProcAddress(library, "JsRunScript");
    m_jsApiHooks.pfJsrtRunModule = (JsAPIHooks::JsrtRunModulePtr)GetProcAddress(library, "JsExperimentalApiRunModule");
    m_jsApiHooks.pfJsrtCallFunction = (JsAPIHooks::JsrtCallFunctionPtr)GetProcAddress(library, "JsCallFunction");
    m_jsApiHooks.pfJsrtNumbertoDouble = (JsAPIHooks::JsrtNumberToDoublePtr)GetProcAddress(library, "JsNumberToDouble");
    m_jsApiHooks.pfJsrtNumbertoInt = (JsAPIHooks::JsrtNumberToIntPtr)GetProcAddress(library, "JsNumberToInt");
    m_jsApiHooks.pfJsrtIntToNumber = (JsAPIHooks::JsrtIntToNumberPtr)GetProcAddress(library, "JsIntToNumber");
    m_jsApiHooks.pfJsrtDoubleToNumber = (JsAPIHooks::JsrtDoubleToNumberPtr)GetProcAddress(library, "JsDoubleToNumber");
    m_jsApiHooks.pfJsrtGetExternalData = (JsAPIHooks::JsrtGetExternalDataPtr)GetProcAddress(library, "JsGetExternalData");
    m_jsApiHooks.pfJsrtCreateArray = (JsAPIHooks::JsrtCreateArrayPtr)GetProcAddress(library, "JsCreateArray");
    m_jsApiHooks.pfJsrtSetException = (JsAPIHooks::JsrtSetExceptionPtr)GetProcAddress(library, "JsSetException");
    m_jsApiHooks.pfJsrtGetAndClearException = (JsAPIHooks::JsrtGetAndClearExceptiopnPtr)GetProcAddress(library, "JsGetAndClearException");
    m_jsApiHooks.pfJsrtCreateError = (JsAPIHooks::JsrtCreateErrorPtr)GetProcAddress(library, "JsCreateError");
    m_jsApiHooks.pfJsrtGetRuntime = (JsAPIHooks::JsrtGetRuntimePtr)GetProcAddress(library, "JsGetRuntime");
    m_jsApiHooks.pfJsrtRelease = (JsAPIHooks::JsrtReleasePtr)GetProcAddress(library, "JsRelease");
    m_jsApiHooks.pfJsrtAddRef = (JsAPIHooks::JsrtAddRefPtr)GetProcAddress(library, "JsAddRef");
    m_jsApiHooks.pfJsrtGetValueType = (JsAPIHooks::JsrtGetValueType)GetProcAddress(library, "JsGetValueType");
    m_jsApiHooks.pfJsrtSetIndexedProperty = (JsAPIHooks::JsrtSetIndexedPropertyPtr)GetProcAddress(library, "JsSetIndexedProperty");
    m_jsApiHooks.pfJsrtSerializeScript = (JsAPIHooks::JsrtSerializeScriptPtr)GetProcAddress(library, "JsSerializeScript");
    m_jsApiHooks.pfJsrtRunSerializedScript = (JsAPIHooks::JsrtRunSerializedScriptPtr)GetProcAddress(library, "JsRunSerializedScript");
    m_jsApiHooks.pfJsrtSetPromiseContinuationCallback = (JsAPIHooks::JsrtSetPromiseContinuationCallbackPtr)GetProcAddress(library, "JsSetPromiseContinuationCallback");
    m_jsApiHooks.pfJsrtGetContextOfObject = (JsAPIHooks::JsrtGetContextOfObjectPtr)GetProcAddress(library, "JsGetContextOfObject");

    m_jsApiHooks.pfJsrtDiagStartDebugging = (JsAPIHooks::JsrtDiagStartDebuggingPtr)GetProcAddress(library, "JsDiagStartDebugging");
    m_jsApiHooks.pfJsrtDiagGetScripts = (JsAPIHooks::JsrtDiagGetScriptsPtr)GetProcAddress(library, "JsDiagGetScripts");
    m_jsApiHooks.pfJsrtDiagGetSource = (JsAPIHooks::JsrtDiagGetSourcePtr)GetProcAddress(library, "JsDiagGetSource");
    m_jsApiHooks.pfJsrtDiagResume = (JsAPIHooks::JsrtDiagResumePtr)GetProcAddress(library, "JsDiagResume");
    m_jsApiHooks.pfJsrtDiagSetBreakpoint = (JsAPIHooks::JsrtDiagSetBreakpointPtr)GetProcAddress(library, "JsDiagSetBreakpoint");
    m_jsApiHooks.pfJsrtDiagGetFunctionPosition = (JsAPIHooks::JsrtDiagGetFunctionPositionPtr)GetProcAddress(library, "JsDiagGetFunctionPosition");
    m_jsApiHooks.pfJsrtDiagGetStacktrace = (JsAPIHooks::JsrtDiagGetStacktracePtr)GetProcAddress(library, "JsDiagGetStacktrace");
    m_jsApiHooks.pfJsrtDiagGetStackProperties = (JsAPIHooks::JsrtDiagGetStackPropertiesPtr)GetProcAddress(library, "JsDiagGetStackProperties");
    m_jsApiHooks.pfJsrtDiagLookupHandles = (JsAPIHooks::JsrtDiagLookupHandlesPtr)GetProcAddress(library, "JsDiagLookupHandles");
    m_jsApiHooks.pfJsrtDiagEvaluate = (JsAPIHooks::JsrtDiagEvaluatePtr)GetProcAddress(library, "JsDiagEvaluate");
    m_jsApiHooks.pfJsrtDiagGetBreakpoints = (JsAPIHooks::JsrtDiagGetBreakpointsPtr)GetProcAddress(library, "JsDiagGetBreakpoints");
    m_jsApiHooks.pfJsrtDiagGetProperties = (JsAPIHooks::JsrtDiagGetPropertiesPtr)GetProcAddress(library, "JsDiagGetProperties");
    m_jsApiHooks.pfJsrtDiagRemoveBreakpoint = (JsAPIHooks::JsrtDiagRemoveBreakpointPtr)GetProcAddress(library, "JsDiagRemoveBreakpoint");

    m_jsApiHooks.pfJsrtDiagSetBreakOnException = (JsAPIHooks::JsrtDiagSetBreakOnExceptionPtr)GetProcAddress(library, "JsDiagSetBreakOnException");
    m_jsApiHooks.pfJsrtDiagGetBreakOnException = (JsAPIHooks::JsrtDiagGetBreakOnExceptionPtr)GetProcAddress(library, "JsDiagGetBreakOnException");


#if DBG || ENABLE_DEBUG_CONFIG_OPTIONS
    m_jsApiHooks.pfJsrtTTDCreateRecordRuntime = (JsAPIHooks::JsrtTTDCreateRecordRuntimePtr)GetProcAddress(library, "JsTTDCreateRecordRuntime");
    m_jsApiHooks.pfJsrtTTDCreateDebugRuntime = (JsAPIHooks::JsrtTTDCreateDebugRuntimePtr)GetProcAddress(library, "JsTTDCreateDebugRuntime");
    m_jsApiHooks.pfJsrtTTDCreateContext = (JsAPIHooks::JsrtTTDCreateContextPtr)GetProcAddress(library, "JsTTDCreateContext");
    m_jsApiHooks.pfJsrtTTDRunScript = (JsAPIHooks::JsrtTTDRunScriptPtr)GetProcAddress(library, "JsTTDRunScript");
    m_jsApiHooks.pfJsrtTTDCallFunction = (JsAPIHooks::JsrtTTDCallFunctionPtr)GetProcAddress(library, "JsTTDCallFunction");

    m_jsApiHooks.pfJsrtTTDSetDebuggerCallback = (JsAPIHooks::JsrtTTDSetDebuggerCallbackPtr)GetProcAddress(library, "JsTTDSetDebuggerCallback");
    m_jsApiHooks.pfJsrtTTDSetIOCallbacks = (JsAPIHooks::JsrtTTDSetIOCallbacksPtr)GetProcAddress(library, "JsTTDSetIOCallbacks");

    m_jsApiHooks.pfJsrtTTDStartTimeTravelRecording = (JsAPIHooks::JsrtTTDStartTimeTravelRecordingPtr)GetProcAddress(library, "JsTTDStartTimeTravelRecording");
    m_jsApiHooks.pfJsrtTTDStopTimeTravelRecording = (JsAPIHooks::JsrtTTDStopTimeTravelRecordingPtr)GetProcAddress(library, "JsTTDStopTimeTravelRecording");
    m_jsApiHooks.pfJsrtTTDEmitTimeTravelRecording = (JsAPIHooks::JsrtTTDEmitTimeTravelRecordingPtr)GetProcAddress(library, "JsTTDEmitTimeTravelRecording");

    m_jsApiHooks.pfJsrtTTDStartTimeTravelDebugging = (JsAPIHooks::JsrtTTDStartTimeTravelDebuggingPtr)GetProcAddress(library, "JsTTDStartTimeTravelDebugging");
    m_jsApiHooks.pfJsrtTTDPauseTimeTravelBeforeRuntimeOperation = (JsAPIHooks::JsrtTTDPauseTimeTravelBeforeRuntimeOperationPtr)GetProcAddress(library, "JsTTDPauseTimeTravelBeforeRuntimeOperation");
    m_jsApiHooks.pfJsrtTTDReStartTimeTravelAfterRuntimeOperation = (JsAPIHooks::JsrtTTDReStartTimeTravelAfterRuntimeOperationPtr)GetProcAddress(library, "JsTTDReStartTimeTravelAfterRuntimeOperation");

    m_jsApiHooks.pfJsrtTTDPrintVariable = (JsAPIHooks::JsrtTTDPrintVariablePtr)GetProcAddress(library, "JsTTDPrintVariable");

    m_jsApiHooks.pfJsrtTTDGetExecutionTimeInfo = (JsAPIHooks::JsrtTTDGetExecutionTimeInfoPtr)GetProcAddress(library, "JsTTDGetExecutionTimeInfo");
    m_jsApiHooks.pfJsrtTTDGetLastExceptionThrowTimeInfo = (JsAPIHooks::JsrtTTDGetLastExceptionThrowTimeInfoPtr)GetProcAddress(library, "JsTTDGetLastExceptionThrowTimeInfo");
    m_jsApiHooks.pfJsrtTTDGetLastFunctionReturnTimeInfo = (JsAPIHooks::JsrtTTDGetLastFunctionReturnTimeInfoPtr)GetProcAddress(library, "JsTTDGetLastFunctionReturnTimeInfo");

    m_jsApiHooks.pfJsrtTTDNotifyHostCallbackCreatedOrCanceled = (JsAPIHooks::JsrtTTDNotifyHostCallbackCreatedOrCanceledPtr)GetProcAddress(library, "JsTTDNotifyHostCallbackCreatedOrCanceled");
    m_jsApiHooks.pfJsrtTTDGetCurrentCallbackOperationTimeInfo = (JsAPIHooks::JsrtTTDGetCurrentCallbackOperationTimeInfoPtr)GetProcAddress(library, "JsTTDGetCurrentCallbackOperationTimeInfo");

    m_jsApiHooks.pfJsrtTTDSetBP = (JsAPIHooks::JsrtTTDSetBPPtr)GetProcAddress(library, "JsTTDSetBP");
    m_jsApiHooks.pfJsrtTTDSetStepBP = (JsAPIHooks::JsrtTTDSetStepBPPtr)GetProcAddress(library, "JsTTDSetStepBP");
    m_jsApiHooks.pfJsrtTTDSetContinueBP = (JsAPIHooks::JsrtTTDSetContinueBPPtr)GetProcAddress(library, "JsTTDSetContinueBP");

    m_jsApiHooks.pfJsrtTTDPrepContextsForTopLevelEventMove = (JsAPIHooks::JsrtTTDPrepContextsForTopLevelEventMovePtr)GetProcAddress(library, "JsTTDPrepContextsForTopLevelEventMove");
    m_jsApiHooks.pfJsrtTTDMoveToTopLevelEvent = (JsAPIHooks::JsrtTTDMoveToTopLevelEventPtr)GetProcAddress(library, "JsTTDMoveToTopLevelEvent");
    m_jsApiHooks.pfJsrtTTDReplayExecution = (JsAPIHooks::JsrtTTDReplayExecutionPtr)GetProcAddress(library, "JsTTDReplayExecution");
#else
    m_jsApiHooks.pfJsrtTTDSetDebuggerCallback = nullptr;
    m_jsApiHooks.pfJsrtTTDSetIOCallbacks = nullptr;
    m_jsApiHooks.pfJsrtTTDPrintVariable = nullptr;

    m_jsApiHooks.pfJsrtTTDGetExecutionTimeInfo = nullptr;
    m_jsApiHooks.pfJsrtTTDGetLastExceptionThrowTimeInfo = nullptr;
    m_jsApiHooks.pfJsrtTTDGetLastFunctionReturnTimeInfo = nullptr;

    m_jsApiHooks.pfJsrtTTDNotifyHostCallbackCreatedOrCanceled = nullptr;
    m_jsApiHooks.pfJsrtTTDGetCurrentCallbackOperationTimeInfo = nullptr;

    m_jsApiHooks.pfJsrtTTDSetBP = nullptr;
    m_jsApiHooks.pfJsrtTTDSetStepBP = nullptr;
    m_jsApiHooks.pfJsrtTTDSetContinueBP = nullptr;

    m_jsApiHooks.pfJsrtTTDPrepContextsForTopLevelEventMove = nullptr;
    m_jsApiHooks.pfJsrtTTDMoveToTopLevelEvent = nullptr;
    m_jsApiHooks.pfJsrtTTDReplayExecution = nullptr;
#endif

    return library;
}

/*static*/
void ChakraRTInterface::UnloadChakraDll(HINSTANCE library)
{
    Assert(library != nullptr);
    FARPROC pDllCanUnloadNow = GetProcAddress(library, "DllCanUnloadNow");
    if (pDllCanUnloadNow != nullptr)
    {
        pDllCanUnloadNow();
    }
    FreeLibrary(library);
}

/*static*/
HRESULT ChakraRTInterface::ParseConfigFlags()
{
    HRESULT hr = S_OK;

    if (m_testHooks.pfSetAssertToConsoleFlag)
    {
        SetAssertToConsoleFlag(true);
    }

    if (m_testHooks.pfSetConfigFlags)
    {
        hr = SetConfigFlags(m_argInfo.argc, m_argInfo.argv, &HostConfigFlags::flags);
        if (hr != S_OK && !m_usageStringPrinted)
        {
            m_argInfo.hostPrintUsage();
            m_usageStringPrinted = true;
        }
    }

    if (hr == S_OK)
    {
        Assert(m_argInfo.filename != nullptr);

        *(m_argInfo.filename) = nullptr;
        Assert(m_testHooks.pfGetFilenameFlag != nullptr);
        hr = GetFileNameFlag(m_argInfo.filename);
        if (hr != S_OK)
        {
            wprintf(L"Error: no script file specified.");
            m_argInfo.hostPrintUsage();
            m_usageStringPrinted = true;
        }
    }

    return S_OK;
}

/*static*/
HRESULT ChakraRTInterface::OnChakraCoreLoaded(TestHooks& testHooks)
{
    if (!m_testHooksInitialized)
    {
        m_testHooks = testHooks;
        m_testHooksSetup = true;
        m_testHooksInitialized = true;
        return ParseConfigFlags();
    }

    return S_OK;
}
