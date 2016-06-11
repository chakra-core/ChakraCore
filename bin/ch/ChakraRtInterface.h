//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

struct JsAPIHooks
{
    typedef JsErrorCode (WINAPI *JsrtCreateRuntimePtr)(JsRuntimeAttributes attributes, JsThreadServiceCallback threadService, JsRuntimeHandle *runtime);
    typedef JsErrorCode (WINAPI *JsrtCreateContextPtr)(JsRuntimeHandle runtime, JsContextRef *newContext);
    typedef JsErrorCode (WINAPI *JsrtSetCurrentContextPtr)(JsContextRef context);
    typedef JsErrorCode (WINAPI *JsrtGetCurrentContextPtr)(JsContextRef* context);
    typedef JsErrorCode (WINAPI *JsrtDisposeRuntimePtr)(JsRuntimeHandle runtime);
    typedef JsErrorCode (WINAPI *JsrtCreateObjectPtr)(JsValueRef *object);
    typedef JsErrorCode (WINAPI *JsrtCreateExternalObjectPtr)(void* data, JsFinalizeCallback callback, JsValueRef *object);
    typedef JsErrorCode (WINAPI *JsrtCreateFunctionPtr)(JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function);
    typedef JsErrorCode (WINAPI *JsCreateNamedFunctionPtr)(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function);
    typedef JsErrorCode (WINAPI *JsrtPointerToStringPtr)(const char16 *stringValue, size_t length, JsValueRef *value);
    typedef JsErrorCode (WINAPI *JsrtSetPropertyPtr)(JsValueRef object, JsPropertyIdRef property, JsValueRef value, bool useStrictRules);
    typedef JsErrorCode (WINAPI *JsrtGetGlobalObjectPtr)(JsValueRef *globalObject);
    typedef JsErrorCode (WINAPI *JsrtGetUndefinedValuePtr)(JsValueRef *globalObject);
    typedef JsErrorCode (WINAPI *JsrtConvertValueToStringPtr)(JsValueRef value, JsValueRef *stringValue);
    typedef JsErrorCode (WINAPI *JsrtConvertValueToNumberPtr)(JsValueRef value, JsValueRef *numberValue);
    typedef JsErrorCode (WINAPI *JsrtConvertValueToBooleanPtr)(JsValueRef value, JsValueRef *booleanValue);
    typedef JsErrorCode (WINAPI *JsrtStringToPointerPtr)(JsValueRef value, const char16 **stringValue, size_t *length);
    typedef JsErrorCode (WINAPI *JsrtBooleanToBoolPtr)(JsValueRef value, bool *boolValue);
    typedef JsErrorCode (WINAPI *JsrtGetPropertyIdFromNamePtr)(const char16 *name, JsPropertyIdRef *propertyId);
    typedef JsErrorCode (WINAPI *JsrtGetPropertyPtr)(JsValueRef object, JsPropertyIdRef property, JsValueRef* value);
    typedef JsErrorCode (WINAPI *JsrtHasPropertyPtr)(JsValueRef object, JsPropertyIdRef property, bool *hasProperty);
    typedef JsErrorCode (WINAPI *JsrtRunScriptPtr)(const char16 *script, DWORD_PTR sourceContext, const char16 *sourceUrl, JsValueRef* result);
    typedef JsErrorCode (WINAPI *JsrtRunModulePtr)(const char16 *script, DWORD_PTR sourceContext, const char16 *sourceUrl, JsValueRef* result);
    typedef JsErrorCode (WINAPI *JsrtCallFunctionPtr)(JsValueRef function, JsValueRef* arguments, unsigned short argumentCount, JsValueRef *result);
    typedef JsErrorCode (WINAPI *JsrtNumberToDoublePtr)(JsValueRef value, double *doubleValue);
    typedef JsErrorCode (WINAPI *JsrtNumberToIntPtr)(JsValueRef value, int *intValue);
    typedef JsErrorCode (WINAPI *JsrtDoubleToNumberPtr)(double doubleValue, JsValueRef* value);
    typedef JsErrorCode (WINAPI *JsrtGetExternalDataPtr)(JsValueRef object, void **data);
    typedef JsErrorCode (WINAPI *JsrtCreateArrayPtr)(unsigned int length, JsValueRef *result);
    typedef JsErrorCode (WINAPI *JsrtCreateErrorPtr)(JsValueRef message, JsValueRef *error);
    typedef JsErrorCode(WINAPI *JsrtHasExceptionPtr)(bool *hasException);
    typedef JsErrorCode (WINAPI *JsrtSetExceptionPtr)(JsValueRef exception);
    typedef JsErrorCode (WINAPI *JsrtGetAndClearExceptiopnPtr)(JsValueRef* exception);
    typedef JsErrorCode (WINAPI *JsrtGetRuntimePtr)(JsContextRef context, JsRuntimeHandle *runtime);
    typedef JsErrorCode (WINAPI *JsrtReleasePtr)(JsRef ref, unsigned int* count);
    typedef JsErrorCode (WINAPI *JsrtAddRefPtr)(JsRef ref, unsigned int* count);
    typedef JsErrorCode (WINAPI *JsrtGetValueType)(JsValueRef value, JsValueType *type);
    typedef JsErrorCode (WINAPI *JsrtSetIndexedPropertyPtr)(JsValueRef object, JsValueRef index, JsValueRef value);
    typedef JsErrorCode (WINAPI *JsrtSerializeScriptPtr)(const char16 *script, BYTE *buffer, unsigned int *bufferSize);
    typedef JsErrorCode (WINAPI *JsrtRunSerializedScriptPtr)(const char16 *script, BYTE *buffer, DWORD_PTR sourceContext, const char16 *sourceUrl, JsValueRef* result);
    typedef JsErrorCode (WINAPI *JsrtSetPromiseContinuationCallbackPtr)(JsPromiseContinuationCallback callback, void *callbackState);
    typedef JsErrorCode (WINAPI *JsrtGetContextOfObject)(JsValueRef object, JsContextRef *callbackState);

    typedef JsErrorCode(WINAPI *JsrtParseScriptWithAttributes)(const wchar_t *script, JsSourceContext sourceContext, const wchar_t *sourceUrl, JsParseScriptAttributes parseAttributes, JsValueRef *result);
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
    typedef JsErrorCode(WINAPI *JsrtDiagEvaluate)(const wchar_t * expression, unsigned int stackFrameIndex, JsValueRef * evalResult);
    JsrtCreateRuntimePtr pfJsrtCreateRuntime;
    JsrtCreateContextPtr pfJsrtCreateContext;
    JsrtSetCurrentContextPtr pfJsrtSetCurrentContext;
    JsrtGetCurrentContextPtr pfJsrtGetCurrentContext;
    JsrtDisposeRuntimePtr pfJsrtDisposeRuntime;
    JsrtCreateObjectPtr pfJsrtCreateObject;
    JsrtCreateExternalObjectPtr pfJsrtCreateExternalObject;
    JsrtCreateFunctionPtr pfJsrtCreateFunction;
    JsCreateNamedFunctionPtr pfJsrtCreateNameFunction;
    JsrtPointerToStringPtr pfJsrtPointerToString;
    JsrtSetPropertyPtr pfJsrtSetProperty;
    JsrtGetGlobalObjectPtr pfJsrtGetGlobalObject;
    JsrtGetUndefinedValuePtr pfJsrtGetUndefinedValue;
    JsrtGetUndefinedValuePtr pfJsrtGetTrueValue;
    JsrtGetUndefinedValuePtr pfJsrtGetFalseValue;
    JsrtConvertValueToStringPtr pfJsrtConvertValueToString;
    JsrtConvertValueToNumberPtr pfJsrtConvertValueToNumber;
    JsrtConvertValueToBooleanPtr pfJsrtConvertValueToBoolean;
    JsrtStringToPointerPtr pfJsrtStringToPointer;
    JsrtBooleanToBoolPtr pfJsrtBooleanToBool;
    JsrtGetPropertyIdFromNamePtr pfJsrtGetPropertyIdFromName;
    JsrtGetPropertyPtr pfJsrtGetProperty;
    JsrtHasPropertyPtr pfJsrtHasProperty;
    JsrtRunScriptPtr pfJsrtRunScript;
    JsrtRunModulePtr pfJsrtRunModule;
    JsrtCallFunctionPtr pfJsrtCallFunction;
    JsrtNumberToDoublePtr pfJsrtNumbertoDouble;
    JsrtNumberToIntPtr pfJsrtNumbertoInt;
    JsrtDoubleToNumberPtr pfJsrtDoubleToNumber;
    JsrtGetExternalDataPtr pfJsrtGetExternalData;
    JsrtCreateArrayPtr pfJsrtCreateArray;
    JsrtCreateErrorPtr pfJsrtCreateError;
    JsrtHasExceptionPtr pfJsrtHasException;
    JsrtSetExceptionPtr pfJsrtSetException;
    JsrtGetAndClearExceptiopnPtr pfJsrtGetAndClearException;
    JsrtGetRuntimePtr pfJsrtGetRuntime;
    JsrtReleasePtr pfJsrtRelease;
    JsrtAddRefPtr pfJsrtAddRef;
    JsrtGetValueType pfJsrtGetValueType;
    JsrtSetIndexedPropertyPtr pfJsrtSetIndexedProperty;
    JsrtSerializeScriptPtr pfJsrtSerializeScript;
    JsrtRunSerializedScriptPtr pfJsrtRunSerializedScript;
    JsrtSetPromiseContinuationCallbackPtr pfJsrtSetPromiseContinuationCallback;
    JsrtGetContextOfObject pfJsrtGetContextOfObject;
    JsrtParseScriptWithAttributes pfJsrtParseScriptWithAttributes;
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
};

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

    static HINSTANCE LoadChakraDll(ArgInfo* argInfo);
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

    static HRESULT GetCrashOnExceptionFlag(bool * flag)
    {
#ifdef SECURITY_TESTING
        return CHECKED_CALL(GetCrashOnExceptionFlag, flag);
#else
        return E_UNEXPECTED;
#endif
    }

    static void NotifyUnhandledException(PEXCEPTION_POINTERS exceptionInfo)
    {
        if (m_testHooksSetup && m_testHooks.pfnNotifyUnhandledException != NULL)
        {
            m_testHooks.pfnNotifyUnhandledException(exceptionInfo);
        }
    }

    static JsErrorCode WINAPI JsCreateRuntime(JsRuntimeAttributes attributes, JsThreadServiceCallback threadService, JsRuntimeHandle *runtime) { return m_jsApiHooks.pfJsrtCreateRuntime(attributes, threadService, runtime); }
    static JsErrorCode WINAPI JsCreateContext(JsRuntimeHandle runtime, JsContextRef *newContext) { return m_jsApiHooks.pfJsrtCreateContext(runtime, newContext); }
    static JsErrorCode WINAPI JsSetCurrentContext(JsContextRef context) { return m_jsApiHooks.pfJsrtSetCurrentContext(context); }
    static JsErrorCode WINAPI JsGetCurrentContext(JsContextRef* context) { return m_jsApiHooks.pfJsrtGetCurrentContext(context); }
    static JsErrorCode WINAPI JsDisposeRuntime(JsRuntimeHandle runtime) { return m_jsApiHooks.pfJsrtDisposeRuntime(runtime); }
    static JsErrorCode WINAPI JsCreateObject(JsValueRef *object) { return m_jsApiHooks.pfJsrtCreateObject(object); }
    static JsErrorCode WINAPI JsCreateExternalObject(void *data, JsFinalizeCallback callback, JsValueRef *object) { return m_jsApiHooks.pfJsrtCreateExternalObject(data, callback, object); }
    static JsErrorCode WINAPI JsCreateFunction(JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function) { return m_jsApiHooks.pfJsrtCreateFunction(nativeFunction, callbackState, function); }
    static JsErrorCode WINAPI JsCreateNamedFunction(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function) { return m_jsApiHooks.pfJsrtCreateNameFunction(name, nativeFunction, callbackState, function); }
    static JsErrorCode WINAPI JsPointerToString(const char16 *stringValue, size_t length, JsValueRef *value) { return m_jsApiHooks.pfJsrtPointerToString(stringValue, length, value); }
    static JsErrorCode WINAPI JsSetProperty(JsValueRef object, JsPropertyIdRef property, JsValueRef value, bool useStrictRules) { return m_jsApiHooks.pfJsrtSetProperty(object, property, value, useStrictRules); }
    static JsErrorCode WINAPI JsGetGlobalObject(JsValueRef *globalObject) { return m_jsApiHooks.pfJsrtGetGlobalObject(globalObject); }
    static JsErrorCode WINAPI JsGetUndefinedValue(JsValueRef *globalObject) { return m_jsApiHooks.pfJsrtGetUndefinedValue(globalObject); }
    static JsErrorCode WINAPI JsGetTrueValue(JsValueRef *globalObject) { return m_jsApiHooks.pfJsrtGetTrueValue(globalObject); }
    static JsErrorCode WINAPI JsGetFalseValue(JsValueRef *globalObject) { return m_jsApiHooks.pfJsrtGetFalseValue(globalObject); }
    static JsErrorCode WINAPI JsConvertValueToString(JsValueRef value, JsValueRef *stringValue) { return m_jsApiHooks.pfJsrtConvertValueToString(value, stringValue); }
    static JsErrorCode WINAPI JsConvertValueToNumber(JsValueRef value, JsValueRef *numberValue) { return m_jsApiHooks.pfJsrtConvertValueToNumber(value, numberValue); }
    static JsErrorCode WINAPI JsConvertValueToBoolean(JsValueRef value, JsValueRef *booleanValue) { return m_jsApiHooks.pfJsrtConvertValueToBoolean(value, booleanValue); }
    static JsErrorCode WINAPI JsStringToPointer(JsValueRef value, const char16 **stringValue, size_t *length) { return m_jsApiHooks.pfJsrtStringToPointer(value, stringValue, length); }
    static JsErrorCode WINAPI JsBooleanToBool(JsValueRef value, bool* boolValue) { return m_jsApiHooks.pfJsrtBooleanToBool(value, boolValue); }
    static JsErrorCode WINAPI JsGetPropertyIdFromName(const char16 *name, JsPropertyIdRef *propertyId) { return m_jsApiHooks.pfJsrtGetPropertyIdFromName(name, propertyId); }
    static JsErrorCode WINAPI JsGetProperty(JsValueRef object, JsPropertyIdRef property, JsValueRef* value) { return m_jsApiHooks.pfJsrtGetProperty(object, property, value); }
    static JsErrorCode WINAPI JsHasProperty(JsValueRef object, JsPropertyIdRef property, bool *hasProperty) { return m_jsApiHooks.pfJsrtHasProperty(object, property, hasProperty); }
    static JsErrorCode WINAPI JsRunScript(const char16 *script, DWORD_PTR sourceContext, const char16 *sourceUrl, JsValueRef* result) { return m_jsApiHooks.pfJsrtRunScript(script, sourceContext, sourceUrl, result); }
    static JsErrorCode WINAPI JsRunModule(const char16 *script, DWORD_PTR sourceContext, const char16 *sourceUrl, JsValueRef* result) { return m_jsApiHooks.pfJsrtRunModule(script, sourceContext, sourceUrl, result); }
    static JsErrorCode WINAPI JsCallFunction(JsValueRef function, JsValueRef* arguments, unsigned short argumentCount, JsValueRef *result) { return m_jsApiHooks.pfJsrtCallFunction(function, arguments, argumentCount, result); }
    static JsErrorCode WINAPI JsNumberToDouble(JsValueRef value, double* doubleValue) { return m_jsApiHooks.pfJsrtNumbertoDouble(value, doubleValue); }
    static JsErrorCode WINAPI JsNumberToInt(JsValueRef value, int* intValue) { return m_jsApiHooks.pfJsrtNumbertoInt(value, intValue); }
    static JsErrorCode WINAPI JsDoubleToNumber(double doubleValue, JsValueRef* value) { return m_jsApiHooks.pfJsrtDoubleToNumber(doubleValue, value); }
    static JsErrorCode WINAPI JsGetExternalData(JsValueRef object, void **data) { return m_jsApiHooks.pfJsrtGetExternalData(object, data); }
    static JsErrorCode WINAPI JsCreateArray(unsigned int length, JsValueRef *result) { return m_jsApiHooks.pfJsrtCreateArray(length, result); }
    static JsErrorCode WINAPI JsCreateError(JsValueRef message, JsValueRef *error) { return m_jsApiHooks.pfJsrtCreateError(message, error); }
    static JsErrorCode WINAPI JsHasException(bool *hasException) { return m_jsApiHooks.pfJsrtHasException(hasException); }
    static JsErrorCode WINAPI JsSetException(JsValueRef exception) { return m_jsApiHooks.pfJsrtSetException(exception); }
    static JsErrorCode WINAPI JsGetAndClearException(JsValueRef *exception) { return m_jsApiHooks.pfJsrtGetAndClearException(exception); }
    static JsErrorCode WINAPI JsGetRuntime(JsContextRef context, JsRuntimeHandle *runtime) { return m_jsApiHooks.pfJsrtGetRuntime(context, runtime); }
    static JsErrorCode WINAPI JsRelease(JsRef ref, unsigned int* count) { return m_jsApiHooks.pfJsrtRelease(ref, count); }
    static JsErrorCode WINAPI JsAddRef(JsRef ref, unsigned int* count) { return m_jsApiHooks.pfJsrtAddRef(ref, count); }
    static JsErrorCode WINAPI JsGetValueType(JsValueRef value, JsValueType *type) { return m_jsApiHooks.pfJsrtGetValueType(value, type); }
    static JsErrorCode WINAPI JsSetIndexedProperty(JsValueRef object, JsValueRef index, JsValueRef value) { return m_jsApiHooks.pfJsrtSetIndexedProperty(object, index, value); }
    static JsErrorCode WINAPI JsSerializeScript(const char16 *script, BYTE *buffer, unsigned int *bufferSize) { return m_jsApiHooks.pfJsrtSerializeScript(script, buffer, bufferSize); }
    static JsErrorCode WINAPI JsRunSerializedScript(const char16 *script, BYTE *buffer, DWORD_PTR sourceContext, const char16 *sourceUrl, JsValueRef* result) { return m_jsApiHooks.pfJsrtRunSerializedScript(script, buffer, sourceContext, sourceUrl, result); }
    static JsErrorCode WINAPI JsSetPromiseContinuationCallback(JsPromiseContinuationCallback callback, void *callbackState) { return m_jsApiHooks.pfJsrtSetPromiseContinuationCallback(callback, callbackState); }
    static JsErrorCode WINAPI JsGetContextOfObject(JsValueRef object, JsContextRef* context) { return m_jsApiHooks.pfJsrtGetContextOfObject(object, context); }
    static JsErrorCode WINAPI JsParseScriptWithAttributes(const wchar_t *script, JsSourceContext sourceContext, const wchar_t *sourceUrl, JsParseScriptAttributes parseAttributes, JsValueRef *result) { return m_jsApiHooks.pfJsrtParseScriptWithAttributes(script, sourceContext, sourceUrl, parseAttributes, result); }
    static JsErrorCode WINAPI JsDiagStartDebugging(JsRuntimeHandle runtimeHandle, JsDiagDebugEventCallback debugEventCallback, void* callbackState) { return m_jsApiHooks.pfJsrtDiagStartDebugging(runtimeHandle, debugEventCallback, callbackState); }
    static JsErrorCode WINAPI JsDiagStopDebugging(JsRuntimeHandle runtimeHandle, void** callbackState) { return m_jsApiHooks.pfJsrtDiagStopDebugging(runtimeHandle, callbackState); }
    static JsErrorCode WINAPI JsDiagGetSource(unsigned int scriptId, JsValueRef *source) { return m_jsApiHooks.pfJsrtDiagGetSource(scriptId, source); }
    static JsErrorCode WINAPI JsDiagSetBreakpoint(unsigned int scriptId, unsigned int lineNumber, unsigned int columnNumber, JsValueRef *breakpoint) { return m_jsApiHooks.pfJsrtDiagSetBreakpoint(scriptId, lineNumber, columnNumber, breakpoint); }
    static JsErrorCode WINAPI JsDiagGetStackTrace(JsValueRef *stackTrace) { return m_jsApiHooks.pfJsrtDiagGetStackTrace(stackTrace); }
    static JsErrorCode WINAPI JsDiagRequestAsyncBreak(JsRuntimeHandle runtimeHandle) { return m_jsApiHooks.pfJsrtDiagRequestAsyncBreak(runtimeHandle); }
    static JsErrorCode WINAPI JsDiagGetBreakpoints(JsValueRef * breakpoints) { return m_jsApiHooks.pfJsrtDiagGetBreakpoints(breakpoints); }
    static JsErrorCode WINAPI JsDiagRemoveBreakpoint(unsigned int breakpointId) { return m_jsApiHooks.pfJsrtDiagRemoveBreakpoint(breakpointId); }
    static JsErrorCode WINAPI JsDiagSetBreakOnException(JsRuntimeHandle runtimeHandle, JsDiagBreakOnExceptionAttributes exceptionAttributes) { return m_jsApiHooks.pfJsrtDiagSetBreakOnException(runtimeHandle, exceptionAttributes); }
    static JsErrorCode WINAPI JsDiagGetBreakOnException(JsRuntimeHandle runtimeHandle, JsDiagBreakOnExceptionAttributes * exceptionAttributes) { return m_jsApiHooks.pfJsrtDiagGetBreakOnException(runtimeHandle, exceptionAttributes); }
    static JsErrorCode WINAPI JsDiagSetStepType(JsDiagStepType stepType) { return m_jsApiHooks.pfJsrtDiagSetStepType(stepType); }
    static JsErrorCode WINAPI JsDiagGetScripts(JsValueRef * scriptsArray) { return m_jsApiHooks.pfJsrtDiagGetScripts(scriptsArray); }
    static JsErrorCode WINAPI JsDiagGetFunctionPosition(JsValueRef value, JsValueRef * functionPosition) { return m_jsApiHooks.pfJsrtDiagGetFunctionPosition(value, functionPosition); }
    static JsErrorCode WINAPI JsDiagGetStackProperties(unsigned int stackFrameIndex, JsValueRef * properties) { return m_jsApiHooks.pfJsrtDiagGetStackProperties(stackFrameIndex, properties); }
    static JsErrorCode WINAPI JsDiagGetProperties(unsigned int objectHandle, unsigned int fromCount, unsigned int totalCount, JsValueRef * propertiesObject) { return m_jsApiHooks.pfJsrtDiagGetProperties(objectHandle, fromCount, totalCount, propertiesObject); }
    static JsErrorCode WINAPI JsDiagGetObjectFromHandle(unsigned int handle, JsValueRef * handleObject) { return m_jsApiHooks.pfJsrtDiagGetObjectFromHandle(handle, handleObject); }
    static JsErrorCode WINAPI JsDiagEvaluate(const wchar_t * expression, unsigned int stackFrameIndex, JsValueRef * evalResult) { return m_jsApiHooks.pfJsrtDiagEvaluate(expression, stackFrameIndex, evalResult); }
    static JsErrorCode WINAPI JsValueToWchar(JsValueRef value, const wchar_t **stringValue, size_t *length)
    {
        JsValueRef strValue;
        IfJsrtErrorFailLogAndRetErrorCode(ChakraRTInterface::JsConvertValueToString(value, &strValue));
        IfJsrtErrorFailLogAndRetErrorCode(ChakraRTInterface::JsStringToPointer(strValue, stringValue, length));
        return JsNoError;
    }
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
