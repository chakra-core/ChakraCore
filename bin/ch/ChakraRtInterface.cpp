//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#ifdef _WIN32
LPCSTR chakraDllName = "chakracore.dll";
#else
#include <dlfcn.h>
#ifdef __APPLE__
LPCSTR chakraDllName = "libChakraCore.dylib";
#else
LPCSTR chakraDllName = "libChakraCore.so";
#endif
#endif

bool ChakraRTInterface::m_testHooksSetup = false;
bool ChakraRTInterface::m_testHooksInitialized = false;
bool ChakraRTInterface::m_usageStringPrinted = false;

ChakraRTInterface::ArgInfo* ChakraRTInterface::m_argInfo = nullptr;
TestHooks ChakraRTInterface::m_testHooks = { 0 };
JsAPIHooks ChakraRTInterface::m_jsApiHooks = { 0 };

LPCSTR GetChakraDllName()
{
    return chakraDllName;
}

// Wrapper functions to abstract out loading ChakraCore
// and resolving its symbols
// Currently, these functions resolve to the PAL on Linux
// but in the future, we can easily switch to a different mechanism
HINSTANCE LoadChakraCore(LPCSTR libPath)
{
    return LoadLibraryExA(libPath, nullptr, 0);
}

void UnloadChakraCore(HINSTANCE module)
{
    FreeLibrary(module);
}

void* GetChakraCoreSymbol(HINSTANCE module, const char* symbol)
{
    return reinterpret_cast<void*>(GetProcAddress(module, symbol));
}

/*static*/
bool ChakraRTInterface::LoadChakraDll(ArgInfo* argInfo, HINSTANCE *outLibrary)
{
    m_argInfo = argInfo;
#ifndef CHAKRA_STATIC_LIBRARY
    HINSTANCE library = nullptr;

    char dllName[_MAX_PATH];
    GetBinaryPathWithFileNameA(dllName, _MAX_PATH, chakraDllName);

    library = LoadChakraCore(dllName);
    *outLibrary = library;
    if (library == nullptr)
    {
        int ret = GetLastError();
        fwprintf(stderr, _u("FATAL ERROR: Unable to load %S GetLastError=0x%x\n"), dllName, ret);
        return false;
    }

    if (m_usageStringPrinted)
    {
        UnloadChakraDll(library);
        return false;
    }

    m_jsApiHooks.pfJsrtCreateRuntime = (JsAPIHooks::JsrtCreateRuntimePtr)GetChakraCoreSymbol(library, "JsCreateRuntime");
    m_jsApiHooks.pfJsrtCreateContext = (JsAPIHooks::JsrtCreateContextPtr)GetChakraCoreSymbol(library, "JsCreateContext");
    m_jsApiHooks.pfJsrtSetObjectBeforeCollectCallback = (JsAPIHooks::JsrtSetObjectBeforeCollectCallbackPtr)GetChakraCoreSymbol(library, "JsSetObjectBeforeCollectCallback");
    m_jsApiHooks.pfJsrtSetRuntimeMemoryLimit = (JsAPIHooks::JsrtSetRuntimeMemoryLimitPtr)GetChakraCoreSymbol(library, "JsSetRuntimeMemoryLimit");
    m_jsApiHooks.pfJsrtSetCurrentContext = (JsAPIHooks::JsrtSetCurrentContextPtr)GetChakraCoreSymbol(library, "JsSetCurrentContext");
    m_jsApiHooks.pfJsrtGetCurrentContext = (JsAPIHooks::JsrtGetCurrentContextPtr)GetChakraCoreSymbol(library, "JsGetCurrentContext");
    m_jsApiHooks.pfJsrtDisposeRuntime = (JsAPIHooks::JsrtDisposeRuntimePtr)GetChakraCoreSymbol(library, "JsDisposeRuntime");
    m_jsApiHooks.pfJsrtCreateObject = (JsAPIHooks::JsrtCreateObjectPtr)GetChakraCoreSymbol(library, "JsCreateObject");
    m_jsApiHooks.pfJsrtCreateExternalObject = (JsAPIHooks::JsrtCreateExternalObjectPtr)GetChakraCoreSymbol(library, "JsCreateExternalObject");
    m_jsApiHooks.pfJsrtCreateFunction = (JsAPIHooks::JsrtCreateFunctionPtr)GetChakraCoreSymbol(library, "JsCreateFunction");
    m_jsApiHooks.pfJsrtCreateNamedFunction = (JsAPIHooks::JsCreateNamedFunctionPtr)GetChakraCoreSymbol(library, "JsCreateNamedFunction");
    m_jsApiHooks.pfJsrtSetProperty = (JsAPIHooks::JsrtSetPropertyPtr)GetChakraCoreSymbol(library, "JsSetProperty");
    m_jsApiHooks.pfJsrtGetGlobalObject = (JsAPIHooks::JsrtGetGlobalObjectPtr)GetChakraCoreSymbol(library, "JsGetGlobalObject");
    m_jsApiHooks.pfJsrtGetUndefinedValue = (JsAPIHooks::JsrtGetUndefinedValuePtr)GetChakraCoreSymbol(library, "JsGetUndefinedValue");
    m_jsApiHooks.pfJsrtGetNullValue = (JsAPIHooks::JsrtGetUndefinedValuePtr)GetChakraCoreSymbol(library, "JsGetNullValue");
    m_jsApiHooks.pfJsrtGetTrueValue = (JsAPIHooks::JsrtGetUndefinedValuePtr)GetChakraCoreSymbol(library, "JsGetTrueValue");
    m_jsApiHooks.pfJsrtGetFalseValue = (JsAPIHooks::JsrtGetUndefinedValuePtr)GetChakraCoreSymbol(library, "JsGetFalseValue");
    m_jsApiHooks.pfJsrtConvertValueToString = (JsAPIHooks::JsrtConvertValueToStringPtr)GetChakraCoreSymbol(library, "JsConvertValueToString");
    m_jsApiHooks.pfJsrtConvertValueToNumber = (JsAPIHooks::JsrtConvertValueToNumberPtr)GetChakraCoreSymbol(library, "JsConvertValueToNumber");
    m_jsApiHooks.pfJsrtConvertValueToBoolean = (JsAPIHooks::JsrtConvertValueToBooleanPtr)GetChakraCoreSymbol(library, "JsConvertValueToBoolean");
    m_jsApiHooks.pfJsrtBooleanToBool = (JsAPIHooks::JsrtBooleanToBoolPtr)GetChakraCoreSymbol(library, "JsBooleanToBool");
    m_jsApiHooks.pfJsrtGetProperty = (JsAPIHooks::JsrtGetPropertyPtr)GetChakraCoreSymbol(library, "JsGetProperty");
    m_jsApiHooks.pfJsrtHasProperty = (JsAPIHooks::JsrtHasPropertyPtr)GetChakraCoreSymbol(library, "JsHasProperty");
    m_jsApiHooks.pfJsrtCallFunction = (JsAPIHooks::JsrtCallFunctionPtr)GetChakraCoreSymbol(library, "JsCallFunction");
    m_jsApiHooks.pfJsrtNumberToDouble = (JsAPIHooks::JsrtNumberToDoublePtr)GetChakraCoreSymbol(library, "JsNumberToDouble");
    m_jsApiHooks.pfJsrtNumberToInt = (JsAPIHooks::JsrtNumberToIntPtr)GetChakraCoreSymbol(library, "JsNumberToInt");
    m_jsApiHooks.pfJsrtDoubleToNumber = (JsAPIHooks::JsrtDoubleToNumberPtr)GetChakraCoreSymbol(library, "JsDoubleToNumber");
    m_jsApiHooks.pfJsrtGetExternalData = (JsAPIHooks::JsrtGetExternalDataPtr)GetChakraCoreSymbol(library, "JsGetExternalData");
    m_jsApiHooks.pfJsrtSetExternalData = (JsAPIHooks::JsrtSetExternalDataPtr)GetChakraCoreSymbol(library, "JsSetExternalData");
    m_jsApiHooks.pfJsrtCreateArray = (JsAPIHooks::JsrtCreateArrayPtr)GetChakraCoreSymbol(library, "JsCreateArray");
    m_jsApiHooks.pfJsrtCreateArrayBuffer = (JsAPIHooks::JsrtCreateArrayBufferPtr)GetChakraCoreSymbol(library, "JsCreateArrayBuffer");
    m_jsApiHooks.pfJsrtCreateSharedArrayBufferWithSharedContent = (JsAPIHooks::JsrtCreateSharedArrayBufferWithSharedContentPtr)GetChakraCoreSymbol(library, "JsCreateSharedArrayBufferWithSharedContent");
    m_jsApiHooks.pfJsrtGetSharedArrayBufferContent = (JsAPIHooks::JsrtGetSharedArrayBufferContentPtr)GetChakraCoreSymbol(library, "JsGetSharedArrayBufferContent");
    m_jsApiHooks.pfJsrtReleaseSharedArrayBufferContentHandle = (JsAPIHooks::JsrtReleaseSharedArrayBufferContentHandlePtr)GetChakraCoreSymbol(library, "JsReleaseSharedArrayBufferContentHandle");
    m_jsApiHooks.pfJsrtGetArrayBufferStorage = (JsAPIHooks::JsrtGetArrayBufferStoragePtr)GetChakraCoreSymbol(library, "JsGetArrayBufferStorage");
    m_jsApiHooks.pfJsrtHasException = (JsAPIHooks::JsrtHasExceptionPtr)GetChakraCoreSymbol(library, "JsHasException");
    m_jsApiHooks.pfJsrtSetException = (JsAPIHooks::JsrtSetExceptionPtr)GetChakraCoreSymbol(library, "JsSetException");
    m_jsApiHooks.pfJsrtGetAndClearException = (JsAPIHooks::JsrtGetAndClearExceptiopnPtr)GetChakraCoreSymbol(library, "JsGetAndClearException");
    m_jsApiHooks.pfJsrtCreateError = (JsAPIHooks::JsrtCreateErrorPtr)GetChakraCoreSymbol(library, "JsCreateError");
    m_jsApiHooks.pfJsrtGetRuntime = (JsAPIHooks::JsrtGetRuntimePtr)GetChakraCoreSymbol(library, "JsGetRuntime");
    m_jsApiHooks.pfJsrtRelease = (JsAPIHooks::JsrtReleasePtr)GetChakraCoreSymbol(library, "JsRelease");
    m_jsApiHooks.pfJsrtAddRef = (JsAPIHooks::JsrtAddRefPtr)GetChakraCoreSymbol(library, "JsAddRef");
    m_jsApiHooks.pfJsrtGetValueType = (JsAPIHooks::JsrtGetValueType)GetChakraCoreSymbol(library, "JsGetValueType");
    m_jsApiHooks.pfJsrtSetIndexedProperty = (JsAPIHooks::JsrtSetIndexedPropertyPtr)GetChakraCoreSymbol(library, "JsSetIndexedProperty");
    m_jsApiHooks.pfJsrtSetPromiseContinuationCallback = (JsAPIHooks::JsrtSetPromiseContinuationCallbackPtr)GetChakraCoreSymbol(library, "JsSetPromiseContinuationCallback");
    m_jsApiHooks.pfJsrtGetContextOfObject = (JsAPIHooks::JsrtGetContextOfObject)GetChakraCoreSymbol(library, "JsGetContextOfObject");
    m_jsApiHooks.pfJsrtInitializeModuleRecord = (JsAPIHooks::JsInitializeModuleRecordPtr)GetChakraCoreSymbol(library, "JsInitializeModuleRecord");
    m_jsApiHooks.pfJsrtParseModuleSource = (JsAPIHooks::JsParseModuleSourcePtr)GetChakraCoreSymbol(library, "JsParseModuleSource");
    m_jsApiHooks.pfJsrtSetModuleHostInfo = (JsAPIHooks::JsSetModuleHostInfoPtr)GetChakraCoreSymbol(library, "JsSetModuleHostInfo");
    m_jsApiHooks.pfJsrtGetModuleHostInfo = (JsAPIHooks::JsGetModuleHostInfoPtr)GetChakraCoreSymbol(library, "JsGetModuleHostInfo");
    m_jsApiHooks.pfJsrtModuleEvaluation = (JsAPIHooks::JsModuleEvaluationPtr)GetChakraCoreSymbol(library, "JsModuleEvaluation");
    m_jsApiHooks.pfJsrtDiagStartDebugging = (JsAPIHooks::JsrtDiagStartDebugging)GetChakraCoreSymbol(library, "JsDiagStartDebugging");
    m_jsApiHooks.pfJsrtDiagStopDebugging = (JsAPIHooks::JsrtDiagStopDebugging)GetChakraCoreSymbol(library, "JsDiagStopDebugging");
    m_jsApiHooks.pfJsrtDiagGetSource = (JsAPIHooks::JsrtDiagGetSource)GetChakraCoreSymbol(library, "JsDiagGetSource");
    m_jsApiHooks.pfJsrtDiagSetBreakpoint = (JsAPIHooks::JsrtDiagSetBreakpoint)GetChakraCoreSymbol(library, "JsDiagSetBreakpoint");
    m_jsApiHooks.pfJsrtDiagGetStackTrace = (JsAPIHooks::JsrtDiagGetStackTrace)GetChakraCoreSymbol(library, "JsDiagGetStackTrace");
    m_jsApiHooks.pfJsrtDiagRequestAsyncBreak = (JsAPIHooks::JsrtDiagRequestAsyncBreak)GetChakraCoreSymbol(library, "JsDiagRequestAsyncBreak");
    m_jsApiHooks.pfJsrtDiagGetBreakpoints = (JsAPIHooks::JsrtDiagGetBreakpoints)GetChakraCoreSymbol(library, "JsDiagGetBreakpoints");
    m_jsApiHooks.pfJsrtDiagRemoveBreakpoint = (JsAPIHooks::JsrtDiagRemoveBreakpoint)GetChakraCoreSymbol(library, "JsDiagRemoveBreakpoint");
    m_jsApiHooks.pfJsrtDiagSetBreakOnException = (JsAPIHooks::JsrtDiagSetBreakOnException)GetChakraCoreSymbol(library, "JsDiagSetBreakOnException");
    m_jsApiHooks.pfJsrtDiagGetBreakOnException = (JsAPIHooks::JsrtDiagGetBreakOnException)GetChakraCoreSymbol(library, "JsDiagGetBreakOnException");
    m_jsApiHooks.pfJsrtDiagSetStepType = (JsAPIHooks::JsrtDiagSetStepType)GetChakraCoreSymbol(library, "JsDiagSetStepType");
    m_jsApiHooks.pfJsrtDiagGetScripts = (JsAPIHooks::JsrtDiagGetScripts)GetChakraCoreSymbol(library, "JsDiagGetScripts");
    m_jsApiHooks.pfJsrtDiagGetFunctionPosition = (JsAPIHooks::JsrtDiagGetFunctionPosition)GetChakraCoreSymbol(library, "JsDiagGetFunctionPosition");
    m_jsApiHooks.pfJsrtDiagGetStackProperties = (JsAPIHooks::JsrtDiagGetStackProperties)GetChakraCoreSymbol(library, "JsDiagGetStackProperties");
    m_jsApiHooks.pfJsrtDiagGetProperties = (JsAPIHooks::JsrtDiagGetProperties)GetChakraCoreSymbol(library, "JsDiagGetProperties");
    m_jsApiHooks.pfJsrtDiagGetObjectFromHandle = (JsAPIHooks::JsrtDiagGetObjectFromHandle)GetChakraCoreSymbol(library, "JsDiagGetObjectFromHandle");
    m_jsApiHooks.pfJsrtDiagEvaluate = (JsAPIHooks::JsrtDiagEvaluate)GetChakraCoreSymbol(library, "JsDiagEvaluate");
    m_jsApiHooks.pfJsrtRun = (JsAPIHooks::JsrtRun)GetChakraCoreSymbol(library, "JsRun");
    m_jsApiHooks.pfJsrtParse = (JsAPIHooks::JsrtParse)GetChakraCoreSymbol(library, "JsParse");
    m_jsApiHooks.pfJsrtSerialize = (JsAPIHooks::JsrtSerialize)GetChakraCoreSymbol(library, "JsSerialize");
    m_jsApiHooks.pfJsrtRunSerialized = (JsAPIHooks::JsrtRunSerialized)GetChakraCoreSymbol(library, "JsRunSerialized");
    m_jsApiHooks.pfJsrtGetStringLength = (JsAPIHooks::JsrtGetStringLength)GetChakraCoreSymbol(library, "JsGetStringLength");
    m_jsApiHooks.pfJsrtCreateString = (JsAPIHooks::JsrtCreateString)GetChakraCoreSymbol(library, "JsCreateString");
    m_jsApiHooks.pfJsrtCreateStringUtf16 = (JsAPIHooks::JsrtCreateStringUtf16)GetChakraCoreSymbol(library, "JsCreateStringUtf16");
    m_jsApiHooks.pfJsrtCopyString = (JsAPIHooks::JsrtCopyString)GetChakraCoreSymbol(library, "JsCopyString");
    m_jsApiHooks.pfJsrtCreatePropertyId = (JsAPIHooks::JsrtCreatePropertyId)GetChakraCoreSymbol(library, "JsCreatePropertyId");
    m_jsApiHooks.pfJsrtCreateExternalArrayBuffer = (JsAPIHooks::JsrtCreateExternalArrayBuffer)GetChakraCoreSymbol(library, "JsCreateExternalArrayBuffer");

    m_jsApiHooks.pfJsrtTTDCreateRecordRuntime = (JsAPIHooks::JsrtTTDCreateRecordRuntimePtr)GetChakraCoreSymbol(library, "JsTTDCreateRecordRuntime");
    m_jsApiHooks.pfJsrtTTDCreateReplayRuntime = (JsAPIHooks::JsrtTTDCreateReplayRuntimePtr)GetChakraCoreSymbol(library, "JsTTDCreateReplayRuntime");
    m_jsApiHooks.pfJsrtTTDCreateContext = (JsAPIHooks::JsrtTTDCreateContextPtr)GetChakraCoreSymbol(library, "JsTTDCreateContext");
    m_jsApiHooks.pfJsrtTTDNotifyContextDestroy = (JsAPIHooks::JsrtTTDNotifyContextDestroyPtr)GetChakraCoreSymbol(library, "JsTTDNotifyContextDestroy");

    m_jsApiHooks.pfJsrtTTDStart = (JsAPIHooks::JsrtTTDStartPtr)GetChakraCoreSymbol(library, "JsTTDStart");
    m_jsApiHooks.pfJsrtTTDStop = (JsAPIHooks::JsrtTTDStopPtr)GetChakraCoreSymbol(library, "JsTTDStop");

    m_jsApiHooks.pfJsrtTTDNotifyYield = (JsAPIHooks::JsrtTTDNotifyYieldPtr)GetChakraCoreSymbol(library, "JsTTDNotifyYield");
    m_jsApiHooks.pfJsrtTTDHostExit = (JsAPIHooks::JsrtTTDHostExitPtr)GetChakraCoreSymbol(library, "JsTTDHostExit");

    m_jsApiHooks.pfJsrtTTDGetSnapTimeTopLevelEventMove = (JsAPIHooks::JsrtTTDGetSnapTimeTopLevelEventMovePtr)GetChakraCoreSymbol(library, "JsTTDGetSnapTimeTopLevelEventMove");
    m_jsApiHooks.pfJsrtTTDMoveToTopLevelEvent = (JsAPIHooks::JsrtTTDMoveToTopLevelEventPtr)GetChakraCoreSymbol(library, "JsTTDMoveToTopLevelEvent");
    m_jsApiHooks.pfJsrtTTDReplayExecution = (JsAPIHooks::JsrtTTDReplayExecutionPtr)GetChakraCoreSymbol(library, "JsTTDReplayExecution");
#endif

    return true;
}

/*static*/
void ChakraRTInterface::UnloadChakraDll(HINSTANCE library)
{
#ifndef CHAKRA_STATIC_LIBRARY
    Assert(library != nullptr);
    FARPROC pDllCanUnloadNow = (FARPROC) GetChakraCoreSymbol(library, "DllCanUnloadNow");
    if (pDllCanUnloadNow != nullptr)
    {
        pDllCanUnloadNow();
    }
#ifdef _WIN32
    UnloadChakraCore(library);
#else  // !_WIN32
    // PAL thread shutdown needs more time after execution completion.
    // Do not FreeLibrary. Invoke DllMain(DLL_PROCESS_DETACH) directly.
    typedef BOOL (__stdcall *PDLLMAIN)(HINSTANCE, DWORD, LPVOID);
    PDLLMAIN pDllMain = (PDLLMAIN) GetChakraCoreSymbol(library, "DllMain");
    if (pDllMain)
    {
        pDllMain(library, DLL_PROCESS_DETACH, NULL);
    }
#endif
#endif
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
        hr = SetConfigFlags(m_argInfo->argc, m_argInfo->argv, &HostConfigFlags::flags);
        if (hr != S_OK && !m_usageStringPrinted)
        {
            m_argInfo->hostPrintUsage();
            m_usageStringPrinted = true;
        }
    }

    if (hr == S_OK)
    {
        m_argInfo->filename = nullptr;
        Assert(m_testHooks.pfGetFilenameFlag != nullptr);

        char16* fileNameWide = nullptr;
        hr = GetFileNameFlag(&fileNameWide);

        if (hr != S_OK)
        {
            wprintf(_u("Error: no script file specified."));
            m_argInfo->hostPrintUsage();
            m_usageStringPrinted = true;
        }
        else
        {
            hr = WideStringToNarrowDynamic(fileNameWide, &m_argInfo->filename);
            SysFreeString(fileNameWide);
            if (FAILED(hr))
            {
                Assert(hr == E_OUTOFMEMORY);
                wprintf(_u("Error: Ran out of memory"));
                return hr;
            }
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
