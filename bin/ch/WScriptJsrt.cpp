//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#if defined(_X86_) || defined(_M_IX86)
#define CPU_ARCH_TEXT "x86"
#elif defined(_AMD64_) || defined(_IA64_) || defined(_M_AMD64) || defined(_M_IA64)
#define CPU_ARCH_TEXT "x86_64"
#elif defined(_ARM_) || defined(_M_ARM)
#define CPU_ARCH_TEXT "ARM"
#elif defined(_ARM64_) || defined(_M_ARM64)
#define CPU_ARCH_TEXT "ARM64"
#endif

// do not change the order below
// otherwise, i.e. android system can be marked as posix? etc..
#ifdef _WIN32
#define DEST_PLATFORM_TEXT "win32"
#else // ! _WIN32
#if defined(__APPLE__)
#include <mach-o/dyld.h> // _NSGetExecutablePath
#ifdef __IOS__
#define DEST_PLATFORM_TEXT "ios"
#else // ! iOS
#define DEST_PLATFORM_TEXT "darwin"
#endif // iOS ?
#elif defined(__ANDROID__)
#include <unistd.h> // readlink
#define DEST_PLATFORM_TEXT "android"
#elif defined(__linux__)
#include <unistd.h> // readlink
#define DEST_PLATFORM_TEXT "posix"
#elif defined(__FreeBSD__) || defined(__unix__)
#define DEST_PLATFORM_TEXT "bsd"
#endif // FreeBSD or unix ?
#endif // _WIN32 ?

MessageQueue* WScriptJsrt::messageQueue = nullptr;
std::map<std::string, JsModuleRecord>  WScriptJsrt::moduleRecordMap;
DWORD_PTR WScriptJsrt::sourceContext = 0;

#define ERROR_MESSAGE_TO_STRING(errorCode, errorMessage, errorMessageString)        \
    JsErrorCode errorCode = JsNoError;                                              \
    do                                                                              \
    {                                                                               \
        const char *outOfMemoryString =                                             \
                                    "Failed to convert wide string. Out of memory?";\
                                                                                    \
        char *errorMessageNarrow;                                                   \
        if (FAILED(WideStringToNarrowDynamic(errorMessage, &errorMessageNarrow)))   \
        {                                                                           \
            errorCode = ChakraRTInterface::JsPointerToStringUtf8(outOfMemoryString, \
                        strlen(outOfMemoryString), &errorMessageString);            \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            errorCode = ChakraRTInterface::JsPointerToStringUtf8(errorMessageNarrow,\
                        strlen(errorMessageNarrow), &errorMessageString);           \
            free(errorMessageNarrow);                                               \
        }                                                                           \
    }                                                                               \
    while(0)

DWORD_PTR WScriptJsrt::GetNextSourceContext()
{
    return sourceContext++;
}

bool WScriptJsrt::CreateArgumentsObject(JsValueRef *argsObject)
{
    LPWSTR *argv = HostConfigFlags::argsVal;
    JsValueRef retArr;

    Assert(argsObject);
    *argsObject = nullptr;

    IfJsrtErrorFail(ChakraRTInterface::JsCreateArray(HostConfigFlags::argsCount, &retArr), false);

    for (int i = 0; i < HostConfigFlags::argsCount; i++)
    {
        JsValueRef value;
        JsValueRef index;

        char *argNarrow;
        if (FAILED(WideStringToNarrowDynamic(argv[i], &argNarrow)))
        {
            return false;
        }
        JsErrorCode errCode  = ChakraRTInterface::JsPointerToStringUtf8(argNarrow, strlen(argNarrow), &value);
        free(argNarrow);
        IfJsrtErrorFail(errCode, false);

        IfJsrtErrorFail(ChakraRTInterface::JsDoubleToNumber(i, &index), false);
        IfJsrtErrorFail(ChakraRTInterface::JsSetIndexedProperty(retArr, index, value), false);
    }

    *argsObject = retArr;

    return true;
}

JsValueRef __stdcall WScriptJsrt::EchoCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    for (unsigned int i = 1; i < argumentCount; i++)
    {
        JsValueRef strValue;
        JsErrorCode error = ChakraRTInterface::JsConvertValueToString(arguments[i], &strValue);
        if (error == JsNoError)
        {
            AutoString str;
            size_t length;
            error = ChakraRTInterface::JsStringToPointerUtf8Copy(strValue, &str, &length);
            if (error == JsNoError)
            {
                if (i > 1)
                {
                    wprintf(_u(" "));
                }
                LPWSTR wstr = nullptr;
                NarrowStringToWideDynamic(*str, &wstr);
                wprintf(_u("%ls"), wstr);
                free(wstr);
            }
        }

        if (error == JsErrorScriptException)
        {
            return nullptr;
        }
    }

    wprintf(_u("\n"));
    fflush(stdout);

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

JsValueRef __stdcall WScriptJsrt::QuitCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    int exitCode = 0;

    if (argumentCount > 1)
    {
        double exitCodeDouble;
        IfJsrtErrorFail(ChakraRTInterface::JsNumberToDouble(arguments[1], &exitCodeDouble), JS_INVALID_REFERENCE);
        exitCode = (int)exitCodeDouble;
    }

    ExitProcess(exitCode);
}

JsValueRef __stdcall WScriptJsrt::LoadScriptFileCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    return LoadScriptFileHelper(callee, arguments, argumentCount, false);
}

JsValueRef WScriptJsrt::LoadScriptFileHelper(JsValueRef callee, JsValueRef *arguments, unsigned short argumentCount, bool isSourceModule)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;
    LPCWSTR errorMessage = _u("");

    if (argumentCount < 2 || argumentCount > 4)
    {
        errorCode = JsErrorInvalidArgument;
        errorMessage = _u("Need more or fewer arguments for WScript.LoadScript");
    }
    else
    {
        LPCSTR fileContent;
        AutoString fileName;
        AutoString scriptInjectType;
        size_t fileNameLength;
        size_t scriptInjectTypeLength;

        IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointerUtf8Copy(arguments[1], &fileName, &fileNameLength));

        if (argumentCount > 2)
        {
            IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointerUtf8Copy(arguments[2], &scriptInjectType, &scriptInjectTypeLength));
        }

        if (errorCode == JsNoError)
        {
            hr = Helpers::LoadScriptFromFile(*fileName, fileContent);
            if (FAILED(hr))
            {
                fwprintf(stderr, _u("Couldn't load file.\n"));
            }
            else
            {
                returnValue = LoadScript(callee, *fileName, fileContent, *scriptInjectType ? *scriptInjectType : "self", isSourceModule);
            }
        }
    }

Error:
    if (errorCode != JsNoError)
    {
        JsValueRef errorObject;
        JsValueRef errorMessageString;

        if (wcscmp(errorMessage, _u("")) == 0) {
            errorMessage = ConvertErrorCodeToMessage(errorCode);
        }

        ERROR_MESSAGE_TO_STRING(errCode, errorMessage, errorMessageString);

        ChakraRTInterface::JsCreateError(errorMessageString, &errorObject);
        ChakraRTInterface::JsSetException(errorObject);
    }

    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::LoadScriptCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    return LoadScriptHelper(callee, isConstructCall, arguments, argumentCount, callbackState, false);
}

JsValueRef __stdcall WScriptJsrt::LoadModuleCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    return LoadScriptHelper(callee, isConstructCall, arguments, argumentCount, callbackState, true);
}

JsValueRef WScriptJsrt::LoadScriptHelper(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState, bool isSourceModule)
{
    HRESULT hr = E_FAIL;
    JsErrorCode errorCode = JsNoError;
    LPCWSTR errorMessage = _u("");
    JsValueRef returnValue = JS_INVALID_REFERENCE;

    if (argumentCount < 2 || argumentCount > 4)
    {
        errorCode = JsErrorInvalidArgument;
        errorMessage = _u("Need more or fewer arguments for WScript.LoadScript");
    }
    else
    {
        AutoString fileContent;
        char* fileNameNarrow = nullptr;
        bool freeFileName = false;
        AutoString scriptInjectType;
        size_t fileContentLength;
        size_t scriptInjectTypeLength;
        char fileNameBuffer[MAX_PATH];

        IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointerUtf8Copy(arguments[1], &fileContent, &fileContentLength));

        if (argumentCount > 2)
        {
            IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointerUtf8Copy(arguments[2], &scriptInjectType, &scriptInjectTypeLength));

            if (argumentCount > 3)
            {
                size_t unused;
                IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointerUtf8Copy(arguments[3], &fileNameNarrow, &unused));
                freeFileName = true;
            }
        }

        if (!freeFileName && isSourceModule)
        {
            sprintf_s(fileNameBuffer, MAX_PATH, "moduleScript%i.js", (int)sourceContext);
            fileNameNarrow = fileNameBuffer;
        }

        if (*fileContent)
        {
            // TODO: This is CESU-8. How to tell the engine?
            // TODO: How to handle this source (script) life time?
            returnValue = LoadScript(callee, fileNameNarrow, *fileContent, *scriptInjectType ? *scriptInjectType : "self", isSourceModule);
            if (freeFileName)
            {
                ChakraRTInterface::JsStringFree(fileNameNarrow);
            }
        }
    }

Error:
    if (errorCode != JsNoError)
    {
        JsValueRef errorObject;
        JsValueRef errorMessageString;

        if (wcscmp(errorMessage, _u("")) == 0) {
            errorMessage = ConvertErrorCodeToMessage(errorCode);
        }

        ERROR_MESSAGE_TO_STRING(errCode, errorMessage, errorMessageString);

        ChakraRTInterface::JsCreateError(errorMessageString, &errorObject);
        ChakraRTInterface::JsSetException(errorObject);
    }

    return returnValue;
}

JsErrorCode WScriptJsrt::InitializeModuleInfo(JsValueRef specifier, JsModuleRecord moduleRecord)
{
    JsErrorCode errorCode = JsNoError;
    errorCode = ChakraRTInterface::JsSetModuleHostInfo(moduleRecord, JsModuleHostInfo_FetchImportedModuleCallback, (void*)WScriptJsrt::FetchImportedModule);
    if (errorCode == JsNoError)
    {
        errorCode = ChakraRTInterface::JsSetModuleHostInfo(moduleRecord, JsModuleHostInfo_NotifyModuleReadyCallback, (void*)WScriptJsrt::NotifyModuleReadyCallback);
    }
    if (errorCode == JsNoError)
    {
        errorCode = ChakraRTInterface::JsSetModuleHostInfo(moduleRecord, JsModuleHostInfo_HostDefined, specifier);
    }
    IfJsrtErrorFailLogAndRetErrorCode(errorCode);
    return JsNoError;
}

JsErrorCode WScriptJsrt::LoadModuleFromString(LPCSTR fileName, LPCSTR fileContent)
{
    DWORD_PTR dwSourceCookie = WScriptJsrt::GetNextSourceContext();
    JsModuleRecord requestModule = JS_INVALID_REFERENCE;
    auto moduleRecordEntry = moduleRecordMap.find(std::string(fileName));
    JsErrorCode errorCode = JsNoError;
    // we need to create a new moduleRecord if the specifier (fileName) is not found; otherwise we'll use the old one.
    if (moduleRecordEntry == moduleRecordMap.end())
    {
        JsValueRef specifier;
        errorCode = ChakraRTInterface::JsPointerToStringUtf8(fileName, strlen(fileName), &specifier);
        if (errorCode == JsNoError)
        {
            errorCode = ChakraRTInterface::JsInitializeModuleRecord(nullptr, specifier, &requestModule);
        }
        if (errorCode == JsNoError)
        {
            errorCode = InitializeModuleInfo(specifier, requestModule);
        }
        if (errorCode == JsNoError)
        {
            moduleRecordMap[std::string(fileName)] = requestModule;
        }
    }
    else
    {
        requestModule = moduleRecordEntry->second;
    }
    IfJsrtErrorFailLogAndRetErrorCode(errorCode);
    JsValueRef errorObject = JS_INVALID_REFERENCE;

    // ParseModuleSource is sync, while additional fetch & evaluation are async.
    errorCode = ChakraRTInterface::JsParseModuleSource(requestModule, dwSourceCookie, (LPBYTE)fileContent,
        (unsigned int)strlen(fileContent), JsParseModuleSourceFlags_DataIsUTF8, &errorObject);
    if ((errorCode != JsNoError) && errorObject != JS_INVALID_REFERENCE)
    {
        ChakraRTInterface::JsSetException(errorObject);
        return errorCode;
    }
    return JsNoError;
}

JsValueRef WScriptJsrt::LoadScript(JsValueRef callee, LPCSTR fileName, LPCSTR fileContent, LPCSTR scriptInjectType, bool isSourceModule)
{
    HRESULT hr = E_FAIL;
    JsErrorCode errorCode = JsNoError;
    LPCWSTR errorMessage = _u("Internal error.");
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode innerErrorCode = JsNoError;
    JsContextRef currentContext = JS_INVALID_REFERENCE;
    JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;

    char fullPathNarrow[_MAX_PATH];
    size_t len = 0;

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetCurrentContext(&currentContext));
    IfJsrtErrorSetGo(ChakraRTInterface::JsGetRuntime(currentContext, &runtime));

    if (fileName)
    {
        if (_fullpath(fullPathNarrow, fileName, _MAX_PATH) == nullptr)
        {
            IfFailGo(E_FAIL);
        }
        // canonicalize that path name to lower case for the profile storage
        // REVIEW: This doesn't work for UTF8...
        len = strlen(fullPathNarrow);
        for (size_t i = 0; i < len; i++)
        {
            fullPathNarrow[i] = (char)tolower(fullPathNarrow[i]);
        }
    }
    else
    {
        // No fileName provided (WScript.LoadScript()), use dummy "script.js"
        strcpy_s(fullPathNarrow, "script.js");
    }

    // this is called with LoadModuleCallback method as well where caller pass in a string that should be
    // treated as a module source text instead of opening a new file.
    if (isSourceModule || (strcmp(scriptInjectType, "module") == 0))
    {
        errorCode = LoadModuleFromString(fileName, fileContent);
    }
    else if (strcmp(scriptInjectType, "self") == 0)
    {
        JsContextRef calleeContext;
        IfJsrtErrorSetGo(ChakraRTInterface::JsGetContextOfObject(callee, &calleeContext));

        IfJsrtErrorSetGo(ChakraRTInterface::JsSetCurrentContext(calleeContext));

        errorCode = ChakraRTInterface::JsRunScriptUtf8(fileContent, GetNextSourceContext(), fullPathNarrow, &returnValue);

        if(errorCode == JsNoError)
        {
            errorCode = ChakraRTInterface::JsGetGlobalObject(&returnValue);
        }

        IfJsrtErrorSetGo(ChakraRTInterface::JsSetCurrentContext(currentContext));
    }
    else if (strcmp(scriptInjectType, "samethread") == 0)
    {
        JsValueRef newContext = JS_INVALID_REFERENCE;

        // Create a new context and set it as the current context
        IfJsrtErrorSetGo(ChakraRTInterface::JsCreateContext(runtime, &newContext));
        IfJsrtErrorSetGo(ChakraRTInterface::JsSetCurrentContext(newContext));

        // Initialize the host objects
        Initialize();

        errorCode = ChakraRTInterface::JsRunScriptUtf8(fileContent, GetNextSourceContext(), fullPathNarrow, &returnValue);

        if (errorCode == JsNoError)
        {
            errorCode = ChakraRTInterface::JsGetGlobalObject(&returnValue);
        }

        // Set the context back to the old one
        ChakraRTInterface::JsSetCurrentContext(currentContext);
    }
    else
    {
        errorCode = JsErrorInvalidArgument;
        errorMessage = _u("Unsupported argument type inject type.");
    }

Error:
    JsValueRef value = returnValue;
    if (errorCode != JsNoError)
    {
        if (innerErrorCode != JsNoError)
        {
            // Failed to retrieve the inner error message, so set a custom error string
            errorMessage = ConvertErrorCodeToMessage(errorCode);
        }

        JsValueRef error = JS_INVALID_REFERENCE;
        JsValueRef messageProperty = JS_INVALID_REFERENCE;

        ERROR_MESSAGE_TO_STRING(errCode, errorMessage, messageProperty);

        if (errCode == JsNoError)
        {
            errCode = ChakraRTInterface::JsCreateError(messageProperty, &error);
            if (errCode == JsNoError)
            {
                errCode = ChakraRTInterface::JsSetException(error);
            }
        }

        ChakraRTInterface::JsDoubleToNumber(errorCode, &value);
    }

    _flushall();

    return value;
}

JsValueRef WScriptJsrt::SetTimeoutCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    LPCWSTR errorMessage = _u("invalid call to WScript.SetTimeout");

    JsValueRef function;
    JsValueRef timerId;
    unsigned int time;
    double tmp;
    CallbackMessage *msg = nullptr;

    if (argumentCount != 3)
    {
        goto Error;
    }

    function = arguments[1];

    IfJsrtError(ChakraRTInterface::JsNumberToDouble(arguments[2], &tmp));

    time = static_cast<int>(tmp);
    msg = new CallbackMessage(time, function);
    messageQueue->InsertSorted(msg);

    IfJsrtError(ChakraRTInterface::JsDoubleToNumber(static_cast<double>(msg->GetId()), &timerId));
    return timerId;

Error:
    JsValueRef errorObject;
    JsValueRef errorMessageString;

    ERROR_MESSAGE_TO_STRING(errorCode, errorMessage, errorMessageString);

    if (errorCode != JsNoError)
    {
        errorCode = ChakraRTInterface::JsCreateError(errorMessageString, &errorObject);

        if (errorCode != JsNoError)
        {
            ChakraRTInterface::JsSetException(errorObject);
        }
    }

    return JS_INVALID_REFERENCE;
}

JsValueRef WScriptJsrt::ClearTimeoutCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    LPCWSTR errorMessage = _u("invalid call to WScript.ClearTimeout");

    if (argumentCount != 2)
    {
        goto Error;
    }

    unsigned int timerId;
    double tmp;
    JsValueRef undef;
    JsValueRef global;

    IfJsrtError(ChakraRTInterface::JsNumberToDouble(arguments[1], &tmp));

    timerId = static_cast<int>(tmp);
    messageQueue->RemoveById(timerId);

    IfJsrtError(ChakraRTInterface::JsGetGlobalObject(&global));
    IfJsrtError(ChakraRTInterface::JsGetUndefinedValue(&undef));

    return undef;

Error:
    JsValueRef errorObject;
    JsValueRef errorMessageString;

    ERROR_MESSAGE_TO_STRING(errorCode, errorMessage, errorMessageString);

    if (errorCode != JsNoError)
    {
        errorCode = ChakraRTInterface::JsCreateError(errorMessageString, &errorObject);

        if (errorCode != JsNoError)
        {
            ChakraRTInterface::JsSetException(errorObject);
        }
    }

    return JS_INVALID_REFERENCE;
}

template <class DebugOperationFunc>
void QueueDebugOperation(JsValueRef function, const DebugOperationFunc& operation)
{
    WScriptJsrt::PushMessage(WScriptJsrt::CallbackMessage::Create(function, operation));
}

JsValueRef WScriptJsrt::AttachCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    LPCWSTR errorMessage = _u("WScript.Attach requires a function, like WScript.Attach(foo);");
    JsValueType argumentType = JsUndefined;
    if (argumentCount != 2)
    {
        goto Error;
    }
    IfJsrtError(ChakraRTInterface::JsGetValueType(arguments[1], &argumentType));
    if (argumentType != JsFunction)
    {
        goto Error;
    }
    QueueDebugOperation(arguments[1], [](WScriptJsrt::CallbackMessage& msg)
    {
        JsContextRef currentContext = JS_INVALID_REFERENCE;
        ChakraRTInterface::JsGetCurrentContext(&currentContext);
        JsRuntimeHandle currentRuntime = JS_INVALID_RUNTIME_HANDLE;
        ChakraRTInterface::JsGetRuntime(currentContext, &currentRuntime);

        Debugger* debugger = Debugger::GetDebugger(currentRuntime);
        debugger->StartDebugging(currentRuntime);
        debugger->SourceRunDown();

        return msg.CallFunction("");
    });
Error:
    JsValueRef errorObject;
    JsValueRef errorMessageString;

    ERROR_MESSAGE_TO_STRING(errorCode, errorMessage, errorMessageString);

    if (errorCode != JsNoError)
    {
        errorCode = ChakraRTInterface::JsCreateError(errorMessageString, &errorObject);
        if (errorCode != JsNoError)
        {
            ChakraRTInterface::JsSetException(errorObject);
        }
    }
    return JS_INVALID_REFERENCE;
}

JsValueRef WScriptJsrt::DetachCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    LPCWSTR errorMessage = _u("WScript.Detach requires a function, like WScript.Detach(foo);");
    JsValueType argumentType = JsUndefined;
    if (argumentCount != 2)
    {
        goto Error;
    }
    IfJsrtError(ChakraRTInterface::JsGetValueType(arguments[1], &argumentType));
    if (argumentType != JsFunction)
    {
        goto Error;
    }
    QueueDebugOperation(arguments[1], [](WScriptJsrt::CallbackMessage& msg)
    {
        JsContextRef currentContext = JS_INVALID_REFERENCE;
        ChakraRTInterface::JsGetCurrentContext(&currentContext);
        JsRuntimeHandle currentRuntime = JS_INVALID_RUNTIME_HANDLE;
        ChakraRTInterface::JsGetRuntime(currentContext, &currentRuntime);
        if (Debugger::debugger != nullptr)
        {
            Debugger* debugger = Debugger::GetDebugger(currentRuntime);
            debugger->StopDebugging(currentRuntime);
        }
        return msg.CallFunction("");
    });
Error:
    JsValueRef errorObject;
    JsValueRef errorMessageString;

    ERROR_MESSAGE_TO_STRING(errorCode, errorMessage, errorMessageString);

    if (errorCode != JsNoError)
    {
        errorCode = ChakraRTInterface::JsCreateError(errorMessageString, &errorObject);
        if (errorCode != JsNoError)
        {
            ChakraRTInterface::JsSetException(errorObject);
        }
    }
    return JS_INVALID_REFERENCE;
}

JsValueRef WScriptJsrt::DumpFunctionPositionCallback(JsValueRef callee, bool isConstructCall, JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    JsValueRef functionPosition = JS_INVALID_REFERENCE;

    if (argumentCount > 1)
    {
        if (ChakraRTInterface::JsDiagGetFunctionPosition(arguments[1], &functionPosition) != JsNoError)
        {
            // If we can't get the functionPosition pass undefined
            IfJsErrorFailLogAndRet(ChakraRTInterface::JsGetUndefinedValue(&functionPosition));
        }

        if (Debugger::debugger != nullptr)
        {
            Debugger::debugger->DumpFunctionPosition(functionPosition);
        }
    }

    return JS_INVALID_REFERENCE;
}

JsValueRef WScriptJsrt::RequestAsyncBreakCallback(JsValueRef callee, bool isConstructCall, JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    if (Debugger::debugger != nullptr && !Debugger::debugger->IsDetached())
    {
        IfJsErrorFailLogAndRet(ChakraRTInterface::JsDiagRequestAsyncBreak(Debugger::GetRuntime()));
    }
    else
    {
        Helpers::LogError(_u("RequestAsyncBreak can only be called when debugger is attached"));
    }

    return JS_INVALID_REFERENCE;
}

JsValueRef WScriptJsrt::EmptyCallback(JsValueRef callee, bool isConstructCall, JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    return JS_INVALID_REFERENCE;
}

bool WScriptJsrt::CreateNamedFunction(const char* nameString, JsNativeFunction callback, JsValueRef* functionVar)
{
    JsValueRef nameVar;
    IfJsrtErrorFail(ChakraRTInterface::JsPointerToStringUtf8(nameString, strlen(nameString), &nameVar), false);
    IfJsrtErrorFail(ChakraRTInterface::JsCreateNamedFunction(nameVar, callback, nullptr, functionVar), false);
    return true;
}

bool WScriptJsrt::InstallObjectsOnObject(JsValueRef object, const char* name, JsNativeFunction nativeFunction)
{
    JsValueRef propertyValueRef;
    JsPropertyIdRef propertyId;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8(name, &propertyId), false);
    CreateNamedFunction(name, nativeFunction, &propertyValueRef);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(object, propertyId, propertyValueRef, true), false);
    return true;
}

#define SET_BINARY_PATH_ERROR_MESSAGE(path, msg) \
    str_len = (int) strlen(msg);                 \
    memcpy(path, msg, (size_t)str_len);          \
    path[str_len] = char(0)

void GetBinaryLocation(char *path, const unsigned size)
{
    AssertMsg(size >= 512 && path != nullptr, "Min path buffer size 512 and path can not be nullptr");
    AssertMsg(size < INT_MAX, "Isn't it too big for a path buffer?");
#ifdef _WIN32
    LPWSTR wpath = (WCHAR*)malloc(sizeof(WCHAR) * size);
    int str_len;
    if (!wpath)
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: GetModuleFileName has failed. OutOfMemory!");
        return;
    }
    str_len = GetModuleFileNameW(NULL, wpath, size - 1);
    if (str_len <= 0)
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: GetModuleFileName has failed.");
        free(wpath);
        return;
    }

    str_len = WideCharToMultiByte(CP_UTF8, 0, wpath, str_len, path, size, NULL, NULL);
    free(wpath);

    if (str_len <= 0)
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: GetModuleFileName (WideCharToMultiByte) has failed.");
        return;
    }

    if ((unsigned)str_len > size - 1)
    {
        str_len = (int) size - 1;
    }
    path[str_len] = char(0);
#elif defined(__APPLE__)
    uint32_t path_size = (uint32_t)size;
    char *tmp = nullptr;
    int str_len;
    if (_NSGetExecutablePath(path, &path_size))
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: _NSGetExecutablePath has failed.");
        return;
    }

    tmp = (char*)malloc(size);
    char *result = realpath(path, tmp);
    str_len = strlen(result);
    memcpy(path, result, str_len);
    free(tmp);
    path[str_len] = char(0);
#elif defined(__linux__)
    int str_len = readlink("/proc/self/exe", path, size - 1);
    if (str_len <= 0)
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: /proc/self/exe has failed.");
        return;
    }
    path[str_len] = char(0);
#else
#warning "Implement GetBinaryLocation for this platform"
#endif
}

bool WScriptJsrt::Initialize()
{
    HRESULT hr = S_OK;
    char CH_BINARY_LOCATION[2048];
#ifdef CHAKRA_STATIC_LIBRARY
    const char* LINK_TYPE = "static";
#else
    const char* LINK_TYPE = "shared";
#endif

    JsValueRef wscript;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateObject(&wscript), false);

    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Echo", EchoCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Quit", QuitCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "LoadScriptFile", LoadScriptFileCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "LoadScript", LoadScriptCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "LoadModule", LoadModuleCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "SetTimeout", SetTimeoutCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "ClearTimeout", ClearTimeoutCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Attach", AttachCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Detach", DetachCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "DumpFunctionPosition", DumpFunctionPositionCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "RequestAsyncBreak", RequestAsyncBreakCallback));

    // ToDo Remove
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Edit", EmptyCallback));

    // Platform
    JsValueRef platformObject;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateObject(&platformObject), false);
    JsPropertyIdRef platformProperty;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("Platform", &platformProperty), false);

    // Set CPU arch
    JsPropertyIdRef archProperty;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("ARCH", &archProperty), false);
    JsValueRef archValue;
    IfJsrtErrorFail(ChakraRTInterface::JsPointerToStringUtf8(CPU_ARCH_TEXT,strlen(CPU_ARCH_TEXT), &archValue), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, archProperty, archValue, true), false);

    // Set Link Type [static / shared]
    JsPropertyIdRef linkProperty;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("LINK_TYPE", &linkProperty), false);
    JsValueRef linkValue;
    IfJsrtErrorFail(ChakraRTInterface::JsPointerToStringUtf8(LINK_TYPE,strlen(LINK_TYPE), &linkValue), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, linkProperty, linkValue, true), false);

    // Set Binary Location
    JsValueRef binaryPathValue;
    GetBinaryLocation(CH_BINARY_LOCATION, sizeof(CH_BINARY_LOCATION));

    JsPropertyIdRef binaryPathProperty;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("BINARY_PATH",
                                                    &binaryPathProperty), false);

    IfJsrtErrorFail(ChakraRTInterface::JsPointerToStringUtf8(CH_BINARY_LOCATION,
                          strlen(CH_BINARY_LOCATION), &binaryPathValue), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, binaryPathProperty,
                                                  binaryPathValue, true), false);

    // Set destination OS
    JsPropertyIdRef osProperty;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("OS", &osProperty), false);
    JsValueRef osValue;
    IfJsrtErrorFail(ChakraRTInterface::JsPointerToStringUtf8(DEST_PLATFORM_TEXT, strlen(DEST_PLATFORM_TEXT), &osValue), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, osProperty, osValue, true), false);

    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, platformProperty, platformObject, true), false);

    JsValueRef argsObject;

    if (!CreateArgumentsObject(&argsObject))
    {
        return false;
    }

    JsPropertyIdRef argsName;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("Arguments", &argsName), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, argsName, argsObject, true), false);

    JsPropertyIdRef wscriptName;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("WScript", &wscriptName), false);

    JsValueRef global;
    IfJsrtErrorFail(ChakraRTInterface::JsGetGlobalObject(&global), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(global, wscriptName, wscript, true), false);

    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(global, "print", EchoCallback));

    JsValueRef console;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateObject(&console), false);
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(console, "log", EchoCallback));

    JsPropertyIdRef consoleName;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("console", &consoleName), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(global, consoleName, console, true), false);

Error:
    return hr == S_OK;
}

bool WScriptJsrt::PrintException(LPCSTR fileName, JsErrorCode jsErrorCode)
{
    LPCWSTR errorTypeString = ConvertErrorCodeToMessage(jsErrorCode);
    JsValueRef exception;
    ChakraRTInterface::JsGetAndClearException(&exception);
    if (exception != nullptr)
    {
        if (jsErrorCode == JsErrorCode::JsErrorScriptCompile || jsErrorCode == JsErrorCode::JsErrorScriptException)
        {
            AutoString errorMessage;
            size_t errorMessageLength = 0;

            JsValueRef errorString = JS_INVALID_REFERENCE;

            IfJsrtErrorFail(ChakraRTInterface::JsConvertValueToString(exception, &errorString), false);
            IfJsrtErrorFail(ChakraRTInterface::JsStringToPointerUtf8Copy(errorString, &errorMessage, &errorMessageLength), false);
            AutoWideString wideErrorMessage;
            NarrowStringToWideDynamic(*errorMessage, &wideErrorMessage);

            if (jsErrorCode == JsErrorCode::JsErrorScriptCompile)
            {
                JsPropertyIdRef linePropertyId = JS_INVALID_REFERENCE;
                JsValueRef lineProperty = JS_INVALID_REFERENCE;

                JsPropertyIdRef columnPropertyId = JS_INVALID_REFERENCE;
                JsValueRef columnProperty = JS_INVALID_REFERENCE;

                int line;
                int column;

                IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("line", &linePropertyId), false);
                IfJsrtErrorFail(ChakraRTInterface::JsGetProperty(exception, linePropertyId, &lineProperty), false);
                IfJsrtErrorFail(ChakraRTInterface::JsNumberToInt(lineProperty, &line), false);

                IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromNameUtf8("column", &columnPropertyId), false);
                IfJsrtErrorFail(ChakraRTInterface::JsGetProperty(exception, columnPropertyId, &columnProperty), false);
                IfJsrtErrorFail(ChakraRTInterface::JsNumberToInt(columnProperty, &column), false);

                CHAR shortFileName[_MAX_PATH];
                CHAR ext[_MAX_EXT];
                _splitpath_s(fileName, nullptr, 0, nullptr, 0, shortFileName, _countof(shortFileName), ext, _countof(ext));
                fwprintf(stderr, _u("%ls\n\tat code (%S%S:%d:%d)\n"), *wideErrorMessage, shortFileName, ext, (int)line + 1, (int)column + 1);
            }
            else
            {
                JsValueType propertyType = JsUndefined;
                JsPropertyIdRef stackPropertyId = JS_INVALID_REFERENCE;
                JsValueRef stackProperty = JS_INVALID_REFERENCE;
                AutoString errorStack;
                size_t errorStackLength = 0;

                JsErrorCode errorCode = ChakraRTInterface::JsGetPropertyIdFromNameUtf8("stack", &stackPropertyId);

                if (errorCode == JsErrorCode::JsNoError)
                {
                    errorCode = ChakraRTInterface::JsGetProperty(exception, stackPropertyId, &stackProperty);
                    if (errorCode == JsErrorCode::JsNoError)
                    {
                        errorCode = ChakraRTInterface::JsGetValueType(stackProperty, &propertyType);
                    }
                }

                if (errorCode != JsErrorCode::JsNoError || propertyType == JsUndefined)
                {
                    const char *fName = fileName != nullptr ? fileName : "(unknown)";
                    // do not mix char/wchar. print them separately
                    fprintf(stderr, "thrown at %s:\n^\n", fName);
                    fwprintf(stderr, _u("%ls\n"), *wideErrorMessage);
                }
                else
                {
                    IfJsrtErrorFail(ChakraRTInterface::JsStringToPointerUtf8Copy(stackProperty, &errorStack, &errorStackLength), false);
                    AutoWideString wideErrorStack;
                    NarrowStringToWideDynamic(*errorStack, &wideErrorStack);
                    fwprintf(stderr, _u("%ls\n"), *wideErrorStack);
                }
            }
        }
        else
        {
            fwprintf(stderr, _u("Error : %ls\n"), errorTypeString);
        }
        return true;
    }
    else
    {
        fwprintf(stderr, _u("Error : %ls\n"), errorTypeString);
    }
    return false;
}

void WScriptJsrt::AddMessageQueue(MessageQueue *_messageQueue)
{
    Assert(messageQueue == nullptr);

    messageQueue = _messageQueue;
}

WScriptJsrt::CallbackMessage::CallbackMessage(unsigned int time, JsValueRef function) : MessageBase(time), m_function(function)
{
    JsErrorCode error = ChakraRTInterface::JsAddRef(m_function, nullptr);
    if (error != JsNoError)
    {
        // Simply report a fatal error and exit because continuing from this point would result in inconsistent state
        // and FailFast telemetry would not be useful.
        wprintf(_u("FATAL ERROR: ChakraRTInterface::JsAddRef failed in WScriptJsrt::CallbackMessage::`ctor`. error=0x%x\n"), error);
        exit(1);
    }
}

WScriptJsrt::CallbackMessage::~CallbackMessage()
{
    bool hasException = false;
    ChakraRTInterface::JsHasException(&hasException);
    if (hasException)
    {
        WScriptJsrt::PrintException("", JsErrorScriptException);
    }
    JsErrorCode errorCode = ChakraRTInterface::JsRelease(m_function, nullptr);
    Assert(errorCode == JsNoError);
    m_function = JS_INVALID_REFERENCE;
}

HRESULT WScriptJsrt::CallbackMessage::Call(LPCSTR fileName)
{
    return CallFunction(fileName);
}

HRESULT WScriptJsrt::CallbackMessage::CallFunction(LPCSTR fileName)
{
    HRESULT hr = S_OK;

    JsValueRef global;
    JsValueRef result;
    JsValueRef stringValue;
    JsValueType type;
    JsErrorCode errorCode = JsNoError;

    IfJsrtErrorHR(ChakraRTInterface::JsGetGlobalObject(&global));
    IfJsrtErrorHR(ChakraRTInterface::JsGetValueType(m_function, &type));

    if (type == JsString)
    {
        AutoString script;
        size_t length = 0;

        IfJsrtErrorHR(ChakraRTInterface::JsConvertValueToString(m_function, &stringValue));
        IfJsrtErrorHR(ChakraRTInterface::JsStringToPointerUtf8Copy(stringValue, &script, &length));

        // Run the code
        errorCode = ChakraRTInterface::JsRunScriptUtf8(*script, JS_SOURCE_CONTEXT_NONE, "" /*sourceUrl*/, nullptr /*no result needed*/);
    }
    else
    {
        errorCode = ChakraRTInterface::JsCallFunction(m_function, &global, 1, &result);
    }

    if (errorCode != JsNoError)
    {
        hr = E_FAIL;
        PrintException(fileName, errorCode);
    }

Error:
    return hr;
}

WScriptJsrt::ModuleMessage::ModuleMessage(JsModuleRecord module, JsValueRef specifier)
    : MessageBase(0), moduleRecord(module), specifier(specifier)
{
    ChakraRTInterface::JsAddRef(module, nullptr);
    if (specifier != nullptr)
    {
        // nullptr specifier means a Promise to execute; non-nullptr means a "fetch" operation.
        ChakraRTInterface::JsAddRef(specifier, nullptr);
    }
}

WScriptJsrt::ModuleMessage::~ModuleMessage()
{
    ChakraRTInterface::JsRelease(moduleRecord, nullptr);
    if (specifier != nullptr)
    {
        ChakraRTInterface::JsRelease(specifier, nullptr);
    }
}

HRESULT WScriptJsrt::ModuleMessage::Call(LPCSTR fileName)
{
    JsErrorCode errorCode;
    JsValueRef result = JS_INVALID_REFERENCE;
    HRESULT hr;
    if (specifier == nullptr)
    {
        errorCode = ChakraRTInterface::JsModuleEvaluation(moduleRecord, &result);
        if (errorCode != JsNoError)
        {
            PrintException(fileName, errorCode);
        }
    }
    else
    {
        LPCSTR fileContent = nullptr;
        char* specifierStr = nullptr;
        size_t length;
        errorCode = ChakraRTInterface::JsStringToPointerUtf8Copy(specifier, &specifierStr, &length);
        if (errorCode == JsNoError)
        {
            hr = Helpers::LoadScriptFromFile(specifierStr, fileContent);
            if (FAILED(hr))
            {
                fprintf(stderr, "Couldn't load file.\n");
            }
            else
            {
                LoadScript(nullptr, specifierStr, fileContent, "module", true);
            }
        }
    }
    return errorCode;
}

// Callback from chakracore to fetch dependent module. In the test harness, we are not doing any translation, just treat the specifier as fileName.
// While this call will come back directly from ParseModuleSource, the additional task are treated as Promise that will be executed later.
JsErrorCode WScriptJsrt::FetchImportedModule(_In_ JsModuleRecord referencingModule, _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord)
{
    JsModuleRecord moduleRecord = JS_INVALID_REFERENCE;
    AutoString specifierStr;
    size_t length;

    JsErrorCode errorCode = ChakraRTInterface::JsStringToPointerUtf8Copy(specifier, &specifierStr, &length);
    if (errorCode != JsNoError)
    {
        return errorCode;
    }
    auto moduleEntry = moduleRecordMap.find(std::string(*specifierStr));
    if (moduleEntry != moduleRecordMap.end())
    {
        *dependentModuleRecord = moduleEntry->second;
        return JsNoError;
    }

    if (errorCode == JsNoError)
    {
        errorCode = ChakraRTInterface::JsInitializeModuleRecord(referencingModule, specifier, &moduleRecord);
    }
    if (errorCode == JsNoError)
    {
        InitializeModuleInfo(specifier, moduleRecord);
        moduleRecordMap[std::string(*specifierStr)] = moduleRecord;
        ModuleMessage* moduleMessage =
            WScriptJsrt::ModuleMessage::Create(referencingModule, specifier);
        if (moduleMessage == nullptr)
        {
            return JsErrorOutOfMemory;
        }
        WScriptJsrt::PushMessage(moduleMessage);
        *dependentModuleRecord = moduleRecord;
    }
    return errorCode;
}

// Callback from chakraCore when the module resolution is finished, either successfuly or unsuccessfully.
JsErrorCode WScriptJsrt::NotifyModuleReadyCallback(_In_opt_ JsModuleRecord referencingModule, _In_opt_ JsValueRef exceptionVar)
{
    if (exceptionVar != nullptr)
    {
        ChakraRTInterface::JsSetException(exceptionVar);
        JsValueRef specifier = JS_INVALID_REFERENCE;
        ChakraRTInterface::JsGetModuleHostInfo(referencingModule, JsModuleHostInfo_HostDefined, &specifier);
        AutoString fileName;
        size_t length;
        if (specifier != JS_INVALID_REFERENCE)
        {
            ChakraRTInterface::JsStringToPointerUtf8Copy(specifier, &fileName, &length);
        }
        PrintException(*fileName, JsErrorScriptException);
    }
    else
    {
        WScriptJsrt::ModuleMessage* moduleMessage =
            WScriptJsrt::ModuleMessage::Create(referencingModule, nullptr);
        if (moduleMessage == nullptr)
        {
            return JsErrorOutOfMemory;
        }
        WScriptJsrt::PushMessage(moduleMessage);
    }
    return JsNoError;
}
