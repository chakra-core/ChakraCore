//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

struct JsAPIHooks
{
    typedef JsErrorCode (WINAPI *JsrtCreateRuntimePtr)(JsRuntimeAttributes attributes, JsThreadServiceCallback threadService, JsRuntimeHandle *runtime);
    typedef JsErrorCode (WINAPI *JsrtCreateContextPtr)(JsRuntimeHandle runtime, JsContextRef *newContext);
    typedef JsErrorCode(WINAPI *JsrtSetObjectBeforeCollectCallbackPtr)(JsRef ref, void* callbackState, JsObjectBeforeCollectCallback objectBeforeCollectCallback);
    typedef JsErrorCode (WINAPI *JsrtSetRuntimeMemoryLimitPtr)(JsRuntimeHandle runtime, size_t memoryLimit);
    typedef JsErrorCode (WINAPI *JsrtSetCurrentContextPtr)(JsContextRef context);
    typedef JsErrorCode (WINAPI *JsrtGetCurrentContextPtr)(JsContextRef* context);
    typedef JsErrorCode (WINAPI *JsrtDisposeRuntimePtr)(JsRuntimeHandle runtime);
    typedef JsErrorCode (WINAPI *JsrtCreateObjectPtr)(JsValueRef *object);
    typedef JsErrorCode (WINAPI *JsrtCreateExternalObjectPtr)(void* data, JsFinalizeCallback callback, JsValueRef *object);
    typedef JsErrorCode (WINAPI *JsrtCreateFunctionPtr)(JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function);
    typedef JsErrorCode (WINAPI *JsCreateNamedFunctionPtr)(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function);
    typedef JsErrorCode (WINAPI *JsrtSetPropertyPtr)(JsValueRef object, JsPropertyIdRef property, JsValueRef value, bool useStrictRules);
    typedef JsErrorCode (WINAPI *JsrtGetGlobalObjectPtr)(JsValueRef *globalObject);
    typedef JsErrorCode (WINAPI *JsrtGetUndefinedValuePtr)(JsValueRef *globalObject);
    typedef JsErrorCode (WINAPI *JsrtConvertValueToStringPtr)(JsValueRef value, JsValueRef *stringValue);
    typedef JsErrorCode (WINAPI *JsrtConvertValueToNumberPtr)(JsValueRef value, JsValueRef *numberValue);
    typedef JsErrorCode (WINAPI *JsrtConvertValueToBooleanPtr)(JsValueRef value, JsValueRef *booleanValue);
    typedef JsErrorCode (WINAPI *JsrtBooleanToBoolPtr)(JsValueRef value, bool *boolValue);
    typedef JsErrorCode (WINAPI *JsrtGetPropertyPtr)(JsValueRef object, JsPropertyIdRef property, JsValueRef* value);
    typedef JsErrorCode (WINAPI *JsrtHasPropertyPtr)(JsValueRef object, JsPropertyIdRef property, bool *hasProperty);
    typedef JsErrorCode (WINAPI *JsInitializeModuleRecordPtr)(JsModuleRecord referencingModule, JsValueRef normalizedSpecifier, JsModuleRecord* moduleRecord);
    typedef JsErrorCode (WINAPI *JsParseModuleSourcePtr)(JsModuleRecord requestModule, JsSourceContext sourceContext, byte* sourceText, unsigned int sourceLength, JsParseModuleSourceFlags sourceFlag, JsValueRef* exceptionValueRef);
    typedef JsErrorCode (WINAPI *JsModuleEvaluationPtr)(JsModuleRecord requestModule, JsValueRef* result);
    typedef JsErrorCode (WINAPI *JsSetModuleHostInfoPtr)(JsModuleRecord requestModule, JsModuleHostInfoKind moduleHostInfo, void* hostInfo);
    typedef JsErrorCode (WINAPI *JsGetModuleHostInfoPtr)(JsModuleRecord requestModule, JsModuleHostInfoKind moduleHostInfo, void** hostInfo);
    typedef JsErrorCode (WINAPI *JsrtCallFunctionPtr)(JsValueRef function, JsValueRef* arguments, unsigned short argumentCount, JsValueRef *result);
    typedef JsErrorCode (WINAPI *JsrtNumberToDoublePtr)(JsValueRef value, double *doubleValue);
    typedef JsErrorCode (WINAPI *JsrtNumberToIntPtr)(JsValueRef value, int *intValue);
    typedef JsErrorCode (WINAPI *JsrtDoubleToNumberPtr)(double doubleValue, JsValueRef* value);
    typedef JsErrorCode (WINAPI *JsrtGetExternalDataPtr)(JsValueRef object, void **data);
    typedef JsErrorCode (WINAPI *JsrtSetExternalDataPtr)(JsValueRef object, void *data);
    typedef JsErrorCode (WINAPI *JsrtCreateArrayPtr)(unsigned int length, JsValueRef *result);
    typedef JsErrorCode (WINAPI *JsrtCreateArrayBufferPtr)(unsigned int byteLength, JsValueRef *result);
    typedef JsErrorCode (WINAPI *JsrtCreateSharedArrayBufferWithSharedContentPtr)(JsSharedArrayBufferContentHandle sharedContent, JsValueRef *result);
    typedef JsErrorCode (WINAPI *JsrtGetSharedArrayBufferContentPtr)(JsValueRef sharedArrayBuffer, JsSharedArrayBufferContentHandle *sharedContents);
    typedef JsErrorCode (WINAPI *JsrtReleaseSharedArrayBufferContentHandlePtr)(JsSharedArrayBufferContentHandle sharedContent);
    typedef JsErrorCode (WINAPI *JsrtGetArrayBufferStoragePtr)(JsValueRef instance, BYTE **buffer, unsigned int *bufferLength);
    typedef JsErrorCode (WINAPI *JsrtCreateErrorPtr)(JsValueRef message, JsValueRef *error);
    typedef JsErrorCode (WINAPI *JsrtHasExceptionPtr)(bool *hasException);
    typedef JsErrorCode (WINAPI *JsrtSetExceptionPtr)(JsValueRef exception);
    typedef JsErrorCode (WINAPI *JsrtGetAndClearExceptiopnPtr)(JsValueRef* exception);
    typedef JsErrorCode (WINAPI *JsrtGetRuntimePtr)(JsContextRef context, JsRuntimeHandle *runtime);
    typedef JsErrorCode (WINAPI *JsrtReleasePtr)(JsRef ref, unsigned int* count);
    typedef JsErrorCode (WINAPI *JsrtAddRefPtr)(JsRef ref, unsigned int* count);
    typedef JsErrorCode (WINAPI *JsrtGetValueType)(JsValueRef value, JsValueType *type);
    typedef JsErrorCode (WINAPI *JsrtSetIndexedPropertyPtr)(JsValueRef object, JsValueRef index, JsValueRef value);
    typedef JsErrorCode (WINAPI *JsrtSetPromiseContinuationCallbackPtr)(JsPromiseContinuationCallback callback, void *callbackState);
    typedef JsErrorCode (WINAPI *JsrtGetContextOfObject)(JsValueRef object, JsContextRef *callbackState);

    typedef JsErrorCode(WINAPI *JsrtDiagStartDebugging)(JsRuntimeHandle runtimeHandle, JsDiagDebugEventCallback debugEventCallback, void* callbackState);
    typedef JsErrorCode(WINAPI *JsrtDiagStopDebugging)(JsRuntimeHandle runtimeHandle, void** callbackState);
    typedef JsErrorCode(WINAPI *JsrtDiagGetSource)(unsigned int scriptId, JsValueRef *source);
    typedef JsErrorCode(WINAPI *JsrtDiagSetBreakpoint)(unsigned int scriptId, unsigned int lineNumber, unsigned int columnNumber, JsValueRef *breakPoint);
    typedef JsErrorCode(WINAPI *JsrtDiagGetStackTrace)(JsValueRef *stackTrace);
    typedef JsErrorCode(WINAPI *JsrtDiagRequestAsyncBreak)(JsRuntimeHandle runtimeHandle);
    typedef JsErrorCode(WINAPI *JsrtDiagGetBreakpoints)(JsValueRef * breakpoints);
    typedef JsErrorCode(WINAPI *JsrtDiagRemoveBreakpoint)(unsigned int breakpointId);
    typedef JsErrorCode(WINAPI *JsrtDiagSetBreakOnException)(JsRuntimeHandle runtimeHandle, JsDiagBreakOnExceptionAttributes exceptionAttributes);
    typedef JsErrorCode(WINAPI *JsrtDiagGetBreakOnException)(JsRuntimeHandle runtimeHandle, JsDiagBreakOnExceptionAttributes * exceptionAttributes);
    typedef JsErrorCode(WINAPI *JsrtDiagSetStepType)(JsDiagStepType stepType);
    typedef JsErrorCode(WINAPI *JsrtDiagGetScripts)(JsValueRef * scriptsArray);
    typedef JsErrorCode(WINAPI *JsrtDiagGetFunctionPosition)(JsValueRef value, JsValueRef * functionInfo);
    typedef JsErrorCode(WINAPI *JsrtDiagGetStackProperties)(unsigned int stackFrameIndex, JsValueRef * properties);
    typedef JsErrorCode(WINAPI *JsrtDiagGetProperties)(unsigned int objectHandle, unsigned int fromCount, unsigned int totalCount, JsValueRef * propertiesObject);
    typedef JsErrorCode(WINAPI *JsrtDiagGetObjectFromHandle)(unsigned int handle, JsValueRef * handleObject);
    typedef JsErrorCode(WINAPI *JsrtDiagEvaluate)(JsValueRef expression, unsigned int stackFrameIndex, JsParseScriptAttributes parseAttributes, bool forceSetValueProp, JsValueRef * evalResult);

    typedef JsErrorCode(WINAPI *JsrtRun)(JsValueRef script, JsSourceContext sourceContext, JsValueRef sourceUrl, JsParseScriptAttributes parseAttributes, JsValueRef *result);
    typedef JsErrorCode(WINAPI *JsrtParse)(JsValueRef script, JsSourceContext sourceContext, JsValueRef sourceUrl, JsParseScriptAttributes parseAttributes, JsValueRef *result);
    typedef JsErrorCode(WINAPI *JsrtSerialize)(JsValueRef script, JsValueRef *buffer, JsParseScriptAttributes parseAttributes);
    typedef JsErrorCode(WINAPI *JsrtRunSerialized)(JsValueRef buffer, JsSerializedLoadScriptCallback scriptLoadCallback, JsSourceContext sourceContext, JsValueRef sourceUrl, JsValueRef * result);
    typedef JsErrorCode(WINAPI *JsrtGetStringLength)(JsValueRef value, int *stringLength);
    typedef JsErrorCode(WINAPI *JsrtCopyString)(JsValueRef value, char* buffer, size_t bufferSize, size_t* length);
    typedef JsErrorCode(WINAPI *JsrtCreateString)(const char *content, size_t length, JsValueRef *value);
    typedef JsErrorCode(WINAPI *JsrtCreateStringUtf16)(const uint16_t *content, size_t length, JsValueRef *value);
    
    typedef JsErrorCode(WINAPI *JsrtCreateExternalArrayBuffer)(void *data, unsigned int byteLength, JsFinalizeCallback finalizeCallback, void *callbackState, JsValueRef *result);
    typedef JsErrorCode(WINAPI *JsrtCreatePropertyId)(const char *name, size_t length, JsPropertyIdRef *propertyId);

    typedef JsErrorCode(WINAPI *JsrtTTDCreateRecordRuntimePtr)(JsRuntimeAttributes attributes, size_t snapInterval, size_t snapHistoryLength, TTDOpenResourceStreamCallback openResourceStream, JsTTDWriteBytesToStreamCallback writeBytesToStream, JsTTDFlushAndCloseStreamCallback flushAndCloseStream, JsThreadServiceCallback threadService, JsRuntimeHandle *runtime);
    typedef JsErrorCode(WINAPI *JsrtTTDCreateReplayRuntimePtr)(JsRuntimeAttributes attributes, const char* infoUri, size_t infoUriCount, bool enableDebugging, TTDOpenResourceStreamCallback openResourceStream, JsTTDReadBytesFromStreamCallback readBytesFromStream, JsTTDFlushAndCloseStreamCallback flushAndCloseStream, JsThreadServiceCallback threadService, JsRuntimeHandle *runtime);
    typedef JsErrorCode(WINAPI *JsrtTTDCreateContextPtr)(JsRuntimeHandle runtime, bool useRuntimeTTDMode, JsContextRef *newContext);
    typedef JsErrorCode(WINAPI *JsrtTTDNotifyContextDestroyPtr)(JsContextRef context);

    typedef JsErrorCode(WINAPI *JsrtTTDStartPtr)();
    typedef JsErrorCode(WINAPI *JsrtTTDStopPtr)();

    typedef JsErrorCode(WINAPI *JsrtTTDNotifyYieldPtr)();
    typedef JsErrorCode(WINAPI *JsrtTTDHostExitPtr)(int statusCode);

    typedef JsErrorCode(WINAPI *JsrtTTDGetSnapTimeTopLevelEventMovePtr)(JsRuntimeHandle runtimeHandle, JsTTDMoveMode moveMode, uint32_t kthEvent, int64_t* targetEventTime, int64_t* targetStartSnapTime, int64_t* targetEndSnapTime);
    typedef JsErrorCode(WINAPI *JsrtTTDMoveToTopLevelEventPtr)(JsRuntimeHandle runtimeHandle, JsTTDMoveMode moveMode, int64_t snapshotStartTime, int64_t eventTime);
    typedef JsErrorCode(WINAPI *JsrtTTDReplayExecutionPtr)(JsTTDMoveMode* moveMode, int64_t* rootEventTime);

    JsrtCreateRuntimePtr pfJsrtCreateRuntime;
    JsrtCreateContextPtr pfJsrtCreateContext;
    JsrtSetObjectBeforeCollectCallbackPtr pfJsrtSetObjectBeforeCollectCallback;
    JsrtSetRuntimeMemoryLimitPtr pfJsrtSetRuntimeMemoryLimit;
    JsrtSetCurrentContextPtr pfJsrtSetCurrentContext;
    JsrtGetCurrentContextPtr pfJsrtGetCurrentContext;
    JsrtDisposeRuntimePtr pfJsrtDisposeRuntime;
    JsrtCreateObjectPtr pfJsrtCreateObject;
    JsrtCreateExternalObjectPtr pfJsrtCreateExternalObject;
    JsrtCreateFunctionPtr pfJsrtCreateFunction;
    JsCreateNamedFunctionPtr pfJsrtCreateNamedFunction;
    JsrtSetPropertyPtr pfJsrtSetProperty;
    JsrtGetGlobalObjectPtr pfJsrtGetGlobalObject;
    JsrtGetUndefinedValuePtr pfJsrtGetUndefinedValue;
    JsrtGetUndefinedValuePtr pfJsrtGetNullValue;
    JsrtGetUndefinedValuePtr pfJsrtGetTrueValue;
    JsrtGetUndefinedValuePtr pfJsrtGetFalseValue;
    JsrtConvertValueToStringPtr pfJsrtConvertValueToString;
    JsrtConvertValueToNumberPtr pfJsrtConvertValueToNumber;
    JsrtConvertValueToBooleanPtr pfJsrtConvertValueToBoolean;
    JsrtBooleanToBoolPtr pfJsrtBooleanToBool;
    JsrtGetPropertyPtr pfJsrtGetProperty;
    JsrtHasPropertyPtr pfJsrtHasProperty;
    JsParseModuleSourcePtr pfJsrtParseModuleSource;
    JsInitializeModuleRecordPtr pfJsrtInitializeModuleRecord;
    JsModuleEvaluationPtr pfJsrtModuleEvaluation;
    JsSetModuleHostInfoPtr pfJsrtSetModuleHostInfo;
    JsGetModuleHostInfoPtr pfJsrtGetModuleHostInfo;
    JsrtCallFunctionPtr pfJsrtCallFunction;
    JsrtNumberToDoublePtr pfJsrtNumberToDouble;
    JsrtNumberToIntPtr pfJsrtNumberToInt;
    JsrtDoubleToNumberPtr pfJsrtDoubleToNumber;
    JsrtGetExternalDataPtr pfJsrtGetExternalData;
    JsrtSetExternalDataPtr pfJsrtSetExternalData;
    JsrtCreateArrayPtr pfJsrtCreateArray;
    JsrtCreateArrayBufferPtr pfJsrtCreateArrayBuffer;
    JsrtCreateSharedArrayBufferWithSharedContentPtr pfJsrtCreateSharedArrayBufferWithSharedContent;
    JsrtGetSharedArrayBufferContentPtr pfJsrtGetSharedArrayBufferContent;    
    JsrtReleaseSharedArrayBufferContentHandlePtr pfJsrtReleaseSharedArrayBufferContentHandle;
    JsrtGetArrayBufferStoragePtr pfJsrtGetArrayBufferStorage;
    JsrtCreateErrorPtr pfJsrtCreateError;
    JsrtHasExceptionPtr pfJsrtHasException;
    JsrtSetExceptionPtr pfJsrtSetException;
    JsrtGetAndClearExceptiopnPtr pfJsrtGetAndClearException;
    JsrtGetRuntimePtr pfJsrtGetRuntime;
    JsrtReleasePtr pfJsrtRelease;
    JsrtAddRefPtr pfJsrtAddRef;
    JsrtGetValueType pfJsrtGetValueType;
    JsrtSetIndexedPropertyPtr pfJsrtSetIndexedProperty;
    JsrtSetPromiseContinuationCallbackPtr pfJsrtSetPromiseContinuationCallback;
    JsrtGetContextOfObject pfJsrtGetContextOfObject;
    JsrtDiagStartDebugging pfJsrtDiagStartDebugging;
    JsrtDiagStopDebugging pfJsrtDiagStopDebugging;
    JsrtDiagGetSource pfJsrtDiagGetSource;
    JsrtDiagSetBreakpoint pfJsrtDiagSetBreakpoint;
    JsrtDiagGetStackTrace pfJsrtDiagGetStackTrace;
    JsrtDiagRequestAsyncBreak pfJsrtDiagRequestAsyncBreak;
    JsrtDiagGetBreakpoints pfJsrtDiagGetBreakpoints;
    JsrtDiagRemoveBreakpoint pfJsrtDiagRemoveBreakpoint;
    JsrtDiagSetBreakOnException pfJsrtDiagSetBreakOnException;
    JsrtDiagGetBreakOnException pfJsrtDiagGetBreakOnException;
    JsrtDiagSetStepType pfJsrtDiagSetStepType;
    JsrtDiagGetScripts pfJsrtDiagGetScripts;
    JsrtDiagGetFunctionPosition pfJsrtDiagGetFunctionPosition;
    JsrtDiagGetStackProperties pfJsrtDiagGetStackProperties;
    JsrtDiagGetProperties pfJsrtDiagGetProperties;
    JsrtDiagGetObjectFromHandle pfJsrtDiagGetObjectFromHandle;
    JsrtDiagEvaluate pfJsrtDiagEvaluate;

    JsrtRun pfJsrtRun;
    JsrtParse pfJsrtParse;
    JsrtSerialize pfJsrtSerialize;
    JsrtRunSerialized pfJsrtRunSerialized;
    JsrtGetStringLength pfJsrtGetStringLength;
    JsrtCreateString pfJsrtCreateString;
    JsrtCreateStringUtf16 pfJsrtCreateStringUtf16;
    JsrtCopyString pfJsrtCopyString;
    JsrtCreatePropertyId pfJsrtCreatePropertyId;
    JsrtCreateExternalArrayBuffer pfJsrtCreateExternalArrayBuffer;

    JsrtTTDCreateRecordRuntimePtr pfJsrtTTDCreateRecordRuntime;
    JsrtTTDCreateReplayRuntimePtr pfJsrtTTDCreateReplayRuntime;
    JsrtTTDCreateContextPtr pfJsrtTTDCreateContext;
    JsrtTTDNotifyContextDestroyPtr pfJsrtTTDNotifyContextDestroy;

    JsrtTTDStartPtr pfJsrtTTDStart;
    JsrtTTDStopPtr pfJsrtTTDStop;

    JsrtTTDNotifyYieldPtr pfJsrtTTDNotifyYield;
    JsrtTTDHostExitPtr pfJsrtTTDHostExit;

    JsrtTTDGetSnapTimeTopLevelEventMovePtr pfJsrtTTDGetSnapTimeTopLevelEventMove;
    JsrtTTDMoveToTopLevelEventPtr pfJsrtTTDMoveToTopLevelEvent;
    JsrtTTDReplayExecutionPtr pfJsrtTTDReplayExecution;
};

LPCSTR GetChakraDllName();

class ChakraRTInterface
{
public:
    typedef void(__stdcall * HostPrintUsageFuncPtr)();

    struct ArgInfo
    {
        int argc;
        LPWSTR* argv;
        HostPrintUsageFuncPtr hostPrintUsage;
        char* filename;

        ArgInfo() :
            argc(0),
            argv(nullptr),
            hostPrintUsage(nullptr),
            filename(nullptr)
        {
        }

        ArgInfo(int argc, LPWSTR* argv, HostPrintUsageFuncPtr hostPrintUsage, char* filename) :
            argc(argc),
            argv(argv),
            hostPrintUsage(hostPrintUsage),
            filename(filename)
        {
        }

        ~ArgInfo()
        {
            if (filename != nullptr)
            {
                free(filename);
            }
        }
    };

#define CHECKED_CALL_RETURN(func, retVal, ...) (m_testHooksSetup && m_testHooks.pf##func? m_testHooks.pf##func(__VA_ARGS__) : retVal)
#define CHECKED_CALL(func, ...) (m_testHooksSetup && m_testHooks.pf##func? m_testHooks.pf##func(__VA_ARGS__) : E_NOTIMPL)

private:
    static bool m_testHooksSetup;
    static bool m_testHooksInitialized;
    static bool m_usageStringPrinted;
    static ArgInfo* m_argInfo;
    static TestHooks m_testHooks;
    static JsAPIHooks m_jsApiHooks;

private:
    static HRESULT ParseConfigFlags();

public:
    static HRESULT OnChakraCoreLoaded(TestHooks& testHooks);

    static bool LoadChakraDll(ArgInfo* argInfo, HINSTANCE *library);
    static void UnloadChakraDll(HINSTANCE library);

    static HRESULT SetAssertToConsoleFlag(bool flag) { return CHECKED_CALL(SetAssertToConsoleFlag, flag); }
    static HRESULT SetConfigFlags(__in int argc, __in_ecount(argc) LPWSTR argv[], ICustomConfigFlags* customConfigFlags) { return CHECKED_CALL(SetConfigFlags, argc, argv, customConfigFlags); }
    static HRESULT GetFileNameFlag(BSTR * filename) { return CHECKED_CALL(GetFilenameFlag, filename); }
    static HRESULT PrintConfigFlagsUsageString() { m_usageStringPrinted = true;  return CHECKED_CALL(PrintConfigFlagsUsageString); }

#ifdef CHECK_MEMORY_LEAK
    static bool IsEnabledCheckMemoryFlag() { return CHECKED_CALL_RETURN(IsEnabledCheckMemoryLeakFlag, FALSE); }
    static HRESULT SetCheckMemoryLeakFlag(bool flag) { return CHECKED_CALL(SetCheckMemoryLeakFlag, flag); }
    static HRESULT SetEnableCheckMemoryLeakOutput(bool flag) { return CHECKED_CALL(SetEnableCheckMemoryLeakOutput, flag); }
#endif
#ifdef DEBUG
    static HRESULT SetCheckOpHelpersFlag(bool flag) { return CHECKED_CALL(SetCheckOpHelpersFlag, flag); }
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    static HRESULT SetOOPCFGRegistrationFlag(bool flag) { return CHECKED_CALL(SetOOPCFGRegistrationFlag, flag); }
#endif

    static HRESULT GetCrashOnExceptionFlag(bool * flag)
    {
#ifdef SECURITY_TESTING
        return CHECKED_CALL(GetCrashOnExceptionFlag, flag);
#else
        return E_UNEXPECTED;
#endif
    }

#ifdef _WIN32
#if ENABLE_NATIVE_CODEGEN
    static void ConnectJITServer(HANDLE processHandle, void* serverSecurityDescriptor, UUID connectionId)
    {
        if (m_testHooksSetup && m_testHooks.pfnConnectJITServer != NULL)
        {
            m_testHooks.pfnConnectJITServer(processHandle, serverSecurityDescriptor, connectionId);
        }
    }
#endif
#endif

    static void NotifyUnhandledException(PEXCEPTION_POINTERS exceptionInfo)
    {
        if (m_testHooksSetup && m_testHooks.pfnNotifyUnhandledException != NULL)
        {
            m_testHooks.pfnNotifyUnhandledException(exceptionInfo);
        }
    }
#ifdef CHAKRA_STATIC_LIBRARY
#define HOOK_JS_API(x) ::Js##x
#else
#define HOOK_JS_API(x) m_jsApiHooks.pfJsrt##x
#endif

    static JsErrorCode WINAPI JsCreateRuntime(JsRuntimeAttributes attributes, JsThreadServiceCallback threadService, JsRuntimeHandle *runtime) { return HOOK_JS_API(CreateRuntime(attributes, threadService, runtime)); }
    static JsErrorCode WINAPI JsCreateContext(JsRuntimeHandle runtime, JsContextRef *newContext) { return HOOK_JS_API(CreateContext(runtime, newContext)); }
    static JsErrorCode WINAPI JsSetObjectBeforeCollectCallback(JsRef ref, void* callbackState, JsObjectBeforeCollectCallback objectBeforeCollectCallback) { return HOOK_JS_API(SetObjectBeforeCollectCallback(ref, callbackState, objectBeforeCollectCallback)); }
    static JsErrorCode WINAPI JsSetRuntimeMemoryLimit(JsRuntimeHandle runtime, size_t memory) { return HOOK_JS_API(SetRuntimeMemoryLimit(runtime, memory)); }
    static JsErrorCode WINAPI JsSetCurrentContext(JsContextRef context) { return HOOK_JS_API(SetCurrentContext(context)); }
    static JsErrorCode WINAPI JsGetCurrentContext(JsContextRef* context) { return HOOK_JS_API(GetCurrentContext(context)); }
    static JsErrorCode WINAPI JsDisposeRuntime(JsRuntimeHandle runtime) { return HOOK_JS_API(DisposeRuntime(runtime)); }
    static JsErrorCode WINAPI JsCreateObject(JsValueRef *object) { return HOOK_JS_API(CreateObject(object)); }
    static JsErrorCode WINAPI JsCreateExternalObject(void *data, JsFinalizeCallback callback, JsValueRef *object) { return HOOK_JS_API(CreateExternalObject(data, callback, object)); }
    static JsErrorCode WINAPI JsCreateFunction(JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function) { return HOOK_JS_API(CreateFunction(nativeFunction, callbackState, function)); }
    static JsErrorCode WINAPI JsCreateNamedFunction(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function) { return HOOK_JS_API(CreateNamedFunction(name, nativeFunction, callbackState, function)); }
    static JsErrorCode WINAPI JsSetProperty(JsValueRef object, JsPropertyIdRef property, JsValueRef value, bool useStrictRules) { return HOOK_JS_API(SetProperty(object, property, value, useStrictRules)); }
    static JsErrorCode WINAPI JsGetGlobalObject(JsValueRef *globalObject) { return HOOK_JS_API(GetGlobalObject(globalObject)); }
    static JsErrorCode WINAPI JsGetUndefinedValue(JsValueRef *globalObject) { return HOOK_JS_API(GetUndefinedValue(globalObject)); }
    static JsErrorCode WINAPI JsGetNullValue(JsValueRef *globalObject) { return HOOK_JS_API(GetNullValue(globalObject)); }
    static JsErrorCode WINAPI JsGetTrueValue(JsValueRef *globalObject) { return HOOK_JS_API(GetTrueValue(globalObject)); }
    static JsErrorCode WINAPI JsGetFalseValue(JsValueRef *globalObject) { return HOOK_JS_API(GetFalseValue(globalObject)); }
    static JsErrorCode WINAPI JsConvertValueToString(JsValueRef value, JsValueRef *stringValue) { return HOOK_JS_API(ConvertValueToString(value, stringValue)); }
    static JsErrorCode WINAPI JsConvertValueToNumber(JsValueRef value, JsValueRef *numberValue) { return HOOK_JS_API(ConvertValueToNumber(value, numberValue)); }
    static JsErrorCode WINAPI JsConvertValueToBoolean(JsValueRef value, JsValueRef *booleanValue) { return HOOK_JS_API(ConvertValueToBoolean(value, booleanValue)); }
    static JsErrorCode WINAPI JsBooleanToBool(JsValueRef value, bool* boolValue) { return HOOK_JS_API(BooleanToBool(value, boolValue)); }
    static JsErrorCode WINAPI JsGetProperty(JsValueRef object, JsPropertyIdRef property, JsValueRef* value) { return HOOK_JS_API(GetProperty(object, property, value)); }
    static JsErrorCode WINAPI JsHasProperty(JsValueRef object, JsPropertyIdRef property, bool *hasProperty) { return HOOK_JS_API(HasProperty(object, property, hasProperty)); }
    static JsErrorCode WINAPI JsCallFunction(JsValueRef function, JsValueRef* arguments, unsigned short argumentCount, JsValueRef *result) { return HOOK_JS_API(CallFunction(function, arguments, argumentCount, result)); }
    static JsErrorCode WINAPI JsNumberToDouble(JsValueRef value, double* doubleValue) { return HOOK_JS_API(NumberToDouble(value, doubleValue)); }
    static JsErrorCode WINAPI JsNumberToInt(JsValueRef value, int* intValue) { return HOOK_JS_API(NumberToInt(value, intValue)); }
    static JsErrorCode WINAPI JsDoubleToNumber(double doubleValue, JsValueRef* value) { return HOOK_JS_API(DoubleToNumber(doubleValue, value)); }
    static JsErrorCode WINAPI JsGetExternalData(JsValueRef object, void **data) { return HOOK_JS_API(GetExternalData(object, data)); }
    static JsErrorCode WINAPI JsSetExternalData(JsValueRef object, void *data)  { return HOOK_JS_API(SetExternalData(object, data)); }
    static JsErrorCode WINAPI JsCreateArray(unsigned int length, JsValueRef *result) { return HOOK_JS_API(CreateArray(length, result)); }
    static JsErrorCode WINAPI JsCreateArrayBuffer(unsigned int byteLength, JsValueRef *result) { return HOOK_JS_API(CreateArrayBuffer(byteLength, result)); }
    static JsErrorCode WINAPI JsCreateSharedArrayBufferWithSharedContent(JsSharedArrayBufferContentHandle sharedContent, JsValueRef *result) { return HOOK_JS_API(CreateSharedArrayBufferWithSharedContent(sharedContent, result)); }
    static JsErrorCode WINAPI JsGetSharedArrayBufferContent(JsValueRef sharedArrayBuffer, JsSharedArrayBufferContentHandle *sharedContents) { return HOOK_JS_API(GetSharedArrayBufferContent(sharedArrayBuffer, sharedContents)); }
    static JsErrorCode WINAPI JsReleaseSharedArrayBufferContentHandle(JsSharedArrayBufferContentHandle sharedContent) { return HOOK_JS_API(ReleaseSharedArrayBufferContentHandle(sharedContent)); }
    static JsErrorCode WINAPI JsGetArrayBufferStorage(JsValueRef instance, BYTE **buffer, unsigned int *bufferLength) { return HOOK_JS_API(GetArrayBufferStorage(instance, buffer, bufferLength)); }
    static JsErrorCode WINAPI JsCreateError(JsValueRef message, JsValueRef *error) { return HOOK_JS_API(CreateError(message, error)); }
    static JsErrorCode WINAPI JsHasException(bool *hasException) { return HOOK_JS_API(HasException(hasException)); }
    static JsErrorCode WINAPI JsSetException(JsValueRef exception) { return HOOK_JS_API(SetException(exception)); }
    static JsErrorCode WINAPI JsGetAndClearException(JsValueRef *exception) { return HOOK_JS_API(GetAndClearException(exception)); }
    static JsErrorCode WINAPI JsGetRuntime(JsContextRef context, JsRuntimeHandle *runtime) { return HOOK_JS_API(GetRuntime(context, runtime)); }
    static JsErrorCode WINAPI JsRelease(JsRef ref, unsigned int* count) { return HOOK_JS_API(Release(ref, count)); }
    static JsErrorCode WINAPI JsAddRef(JsRef ref, unsigned int* count) { return HOOK_JS_API(AddRef(ref, count)); }
    static JsErrorCode WINAPI JsGetValueType(JsValueRef value, JsValueType *type) { return HOOK_JS_API(GetValueType(value, type)); }
    static JsErrorCode WINAPI JsSetIndexedProperty(JsValueRef object, JsValueRef index, JsValueRef value) { return HOOK_JS_API(SetIndexedProperty(object, index, value)); }
    static JsErrorCode WINAPI JsSetPromiseContinuationCallback(JsPromiseContinuationCallback callback, void *callbackState) { return HOOK_JS_API(SetPromiseContinuationCallback(callback, callbackState)); }
    static JsErrorCode WINAPI JsGetContextOfObject(JsValueRef object, JsContextRef* context) { return HOOK_JS_API(GetContextOfObject(object, context)); }
    static JsErrorCode WINAPI JsDiagStartDebugging(JsRuntimeHandle runtimeHandle, JsDiagDebugEventCallback debugEventCallback, void* callbackState) { return HOOK_JS_API(DiagStartDebugging(runtimeHandle, debugEventCallback, callbackState)); }
    static JsErrorCode WINAPI JsDiagStopDebugging(JsRuntimeHandle runtimeHandle, void** callbackState) { return HOOK_JS_API(DiagStopDebugging(runtimeHandle, callbackState)); }
    static JsErrorCode WINAPI JsDiagGetSource(unsigned int scriptId, JsValueRef *source) { return HOOK_JS_API(DiagGetSource(scriptId, source)); }
    static JsErrorCode WINAPI JsDiagSetBreakpoint(unsigned int scriptId, unsigned int lineNumber, unsigned int columnNumber, JsValueRef *breakpoint) { return HOOK_JS_API(DiagSetBreakpoint(scriptId, lineNumber, columnNumber, breakpoint)); }
    static JsErrorCode WINAPI JsDiagGetStackTrace(JsValueRef *stackTrace) { return HOOK_JS_API(DiagGetStackTrace(stackTrace)); }
    static JsErrorCode WINAPI JsDiagRequestAsyncBreak(JsRuntimeHandle runtimeHandle) { return HOOK_JS_API(DiagRequestAsyncBreak(runtimeHandle)); }
    static JsErrorCode WINAPI JsDiagGetBreakpoints(JsValueRef * breakpoints) { return HOOK_JS_API(DiagGetBreakpoints(breakpoints)); }
    static JsErrorCode WINAPI JsDiagRemoveBreakpoint(unsigned int breakpointId) { return HOOK_JS_API(DiagRemoveBreakpoint(breakpointId)); }
    static JsErrorCode WINAPI JsDiagSetBreakOnException(JsRuntimeHandle runtimeHandle, JsDiagBreakOnExceptionAttributes exceptionAttributes) { return HOOK_JS_API(DiagSetBreakOnException(runtimeHandle, exceptionAttributes)); }
    static JsErrorCode WINAPI JsDiagGetBreakOnException(JsRuntimeHandle runtimeHandle, JsDiagBreakOnExceptionAttributes * exceptionAttributes) { return HOOK_JS_API(DiagGetBreakOnException(runtimeHandle, exceptionAttributes)); }
    static JsErrorCode WINAPI JsDiagSetStepType(JsDiagStepType stepType) { return HOOK_JS_API(DiagSetStepType(stepType)); }
    static JsErrorCode WINAPI JsDiagGetScripts(JsValueRef * scriptsArray) { return HOOK_JS_API(DiagGetScripts(scriptsArray)); }
    static JsErrorCode WINAPI JsDiagGetFunctionPosition(JsValueRef value, JsValueRef * functionPosition) { return HOOK_JS_API(DiagGetFunctionPosition(value, functionPosition)); }
    static JsErrorCode WINAPI JsDiagGetStackProperties(unsigned int stackFrameIndex, JsValueRef * properties) { return HOOK_JS_API(DiagGetStackProperties(stackFrameIndex, properties)); }
    static JsErrorCode WINAPI JsDiagGetProperties(unsigned int objectHandle, unsigned int fromCount, unsigned int totalCount, JsValueRef * propertiesObject) { return HOOK_JS_API(DiagGetProperties(objectHandle, fromCount, totalCount, propertiesObject)); }
    static JsErrorCode WINAPI JsDiagGetObjectFromHandle(unsigned int handle, JsValueRef * handleObject) { return HOOK_JS_API(DiagGetObjectFromHandle(handle, handleObject)); }
    static JsErrorCode WINAPI JsDiagEvaluate(JsValueRef expression, unsigned int stackFrameIndex, JsParseScriptAttributes parseAttributes, bool forceSetValueProp, JsValueRef * evalResult) { return HOOK_JS_API(DiagEvaluate(expression, stackFrameIndex, parseAttributes, forceSetValueProp, evalResult)); }
    static JsErrorCode WINAPI JsParseModuleSource(JsModuleRecord requestModule, JsSourceContext sourceContext, byte* sourceText, unsigned int sourceLength, JsParseModuleSourceFlags sourceFlag, JsValueRef* exceptionValueRef) {
        return HOOK_JS_API(ParseModuleSource(requestModule, sourceContext, sourceText, sourceLength, sourceFlag, exceptionValueRef));
    }
    static JsErrorCode WINAPI JsModuleEvaluation(JsModuleRecord requestModule, JsValueRef* result) { return HOOK_JS_API(ModuleEvaluation(requestModule, result)); }
    static JsErrorCode WINAPI JsInitializeModuleRecord(JsModuleRecord referencingModule, JsValueRef normalizedSpecifier, JsModuleRecord* moduleRecord) {
        return HOOK_JS_API(InitializeModuleRecord(referencingModule, normalizedSpecifier, moduleRecord));
    }
    static JsErrorCode WINAPI JsSetModuleHostInfo(JsModuleRecord requestModule, JsModuleHostInfoKind moduleHostInfo, void* hostInfo) { return HOOK_JS_API(SetModuleHostInfo(requestModule, moduleHostInfo, hostInfo)); }
    static JsErrorCode WINAPI JsGetModuleHostInfo(JsModuleRecord requestModule, JsModuleHostInfoKind moduleHostInfo, void** hostInfo) { return HOOK_JS_API(GetModuleHostInfo(requestModule, moduleHostInfo, hostInfo)); }

    static JsErrorCode WINAPI JsTTDCreateRecordRuntime(JsRuntimeAttributes attributes, size_t snapInterval, size_t snapHistoryLength, TTDOpenResourceStreamCallback openResourceStream, JsTTDWriteBytesToStreamCallback writeBytesToStream, JsTTDFlushAndCloseStreamCallback flushAndCloseStream, JsThreadServiceCallback threadService, JsRuntimeHandle *runtime) {  return HOOK_JS_API(TTDCreateRecordRuntime(attributes, snapInterval, snapHistoryLength, openResourceStream, writeBytesToStream, flushAndCloseStream, threadService, runtime)); }
    static JsErrorCode WINAPI JsTTDCreateReplayRuntime(JsRuntimeAttributes attributes, const char* infoUri, size_t infoUriCount, TTDOpenResourceStreamCallback openResourceStream, JsTTDReadBytesFromStreamCallback readBytesFromStream, JsTTDFlushAndCloseStreamCallback flushAndCloseStream, JsThreadServiceCallback threadService, JsRuntimeHandle *runtime) { return HOOK_JS_API(TTDCreateReplayRuntime(attributes, infoUri, infoUriCount, false, openResourceStream, readBytesFromStream, flushAndCloseStream, threadService, runtime)); }
    static JsErrorCode WINAPI JsTTDCreateContext(JsRuntimeHandle runtime, bool useRuntimeTTDMode, JsContextRef *newContext) { return HOOK_JS_API(TTDCreateContext(runtime, useRuntimeTTDMode, newContext)); }
    static JsErrorCode WINAPI JsTTDNotifyContextDestroy(JsContextRef context) { return HOOK_JS_API(TTDNotifyContextDestroy(context)); }

    static JsErrorCode WINAPI JsTTDStart() { return HOOK_JS_API(TTDStart()); }
    static JsErrorCode WINAPI JsTTDStop() { return HOOK_JS_API(TTDStop()); }

    static JsErrorCode WINAPI JsTTDNotifyYield() { return HOOK_JS_API(TTDNotifyYield()); }
    static JsErrorCode WINAPI JsTTDHostExit(int statusCode) { return HOOK_JS_API(TTDHostExit(statusCode)); }

    static JsErrorCode WINAPI JsTTDGetSnapTimeTopLevelEventMove(JsRuntimeHandle runtimeHandle, JsTTDMoveMode moveMode, uint32_t kthEvent, int64_t* targetEventTime, int64_t* targetStartSnapTime, int64_t* targetEndSnapTime) { return HOOK_JS_API(TTDGetSnapTimeTopLevelEventMove(runtimeHandle, moveMode, kthEvent, targetEventTime, targetStartSnapTime, targetEndSnapTime)); }
    static JsErrorCode WINAPI JsTTDMoveToTopLevelEvent(JsRuntimeHandle runtimeHandle, JsTTDMoveMode moveMode, int64_t snapshotStartTime, int64_t eventTime) { return HOOK_JS_API(TTDMoveToTopLevelEvent(runtimeHandle, moveMode, snapshotStartTime, eventTime)); }
    static JsErrorCode WINAPI JsTTDReplayExecution(JsTTDMoveMode* moveMode, int64_t* rootEventTime) { return HOOK_JS_API(TTDReplayExecution(moveMode, rootEventTime)); }

    static JsErrorCode WINAPI JsRun(JsValueRef script, JsSourceContext sourceContext, JsValueRef sourceUrl, JsParseScriptAttributes parseAttributes, JsValueRef *result) { return HOOK_JS_API(Run(script, sourceContext, sourceUrl, parseAttributes, result)); }
    static JsErrorCode WINAPI JsParse(JsValueRef script, JsSourceContext sourceContext, JsValueRef sourceUrl, JsParseScriptAttributes parseAttributes, JsValueRef *result) { return HOOK_JS_API(Parse(script, sourceContext, sourceUrl, parseAttributes, result)); }
    static JsErrorCode WINAPI JsSerialize(JsValueRef script, JsValueRef *buffer, JsParseScriptAttributes parseAttributes) { return HOOK_JS_API(Serialize(script, buffer, parseAttributes)); }
    static JsErrorCode WINAPI JsRunSerialized(JsValueRef buffer, JsSerializedLoadScriptCallback scriptLoadCallback, JsSourceContext sourceContext, JsValueRef sourceUrl, JsValueRef * result) { return HOOK_JS_API(RunSerialized(buffer, scriptLoadCallback, sourceContext, sourceUrl, result)); }
    static JsErrorCode WINAPI JsGetStringLength(JsValueRef value, int *stringLength) { return HOOK_JS_API(GetStringLength(value, stringLength)); }
    static JsErrorCode WINAPI JsCopyString(JsValueRef value, char* buffer, size_t bufferSize, size_t* length) { return HOOK_JS_API(CopyString(value, buffer, bufferSize, length)); }
    static JsErrorCode WINAPI JsCreateString(const char *content, size_t length, JsValueRef *value) { return HOOK_JS_API(CreateString(content, length, value)); }
    static JsErrorCode WINAPI JsCreateStringUtf16(const uint16_t *content, size_t length, JsValueRef *value) { return HOOK_JS_API(CreateStringUtf16(content, length, value)); }
    static JsErrorCode WINAPI JsCreatePropertyId(const char *name, size_t length, JsPropertyIdRef *propertyId) { return HOOK_JS_API(CreatePropertyId(name, length, propertyId)); }
    static JsErrorCode WINAPI JsCreateExternalArrayBuffer(void *data, unsigned int byteLength, JsFinalizeCallback finalizeCallback, void *callbackState, JsValueRef *result)  { return HOOK_JS_API(CreateExternalArrayBuffer(data, byteLength, finalizeCallback, callbackState, result)); }
};

class AutoRestoreContext
{
public:
    AutoRestoreContext(JsContextRef newContext)
    {
        JsErrorCode errorCode = ChakraRTInterface::JsGetCurrentContext(&oldContext);
        Assert(errorCode == JsNoError);

        if (oldContext != newContext)
        {
            errorCode = ChakraRTInterface::JsSetCurrentContext(newContext);
            Assert(errorCode == JsNoError);
            contextChanged = true;
        }
        else
        {
            contextChanged = false;
        }
    }

    ~AutoRestoreContext()
    {
        if (contextChanged && oldContext != JS_INVALID_REFERENCE)
        {
            ChakraRTInterface::JsSetCurrentContext(oldContext);
        }
    }
private:
    JsContextRef oldContext;
    bool contextChanged;
};
