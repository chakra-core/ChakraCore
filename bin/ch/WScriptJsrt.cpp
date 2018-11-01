//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include <vector>

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
#ifdef __IOS__
#define DEST_PLATFORM_TEXT "ios"
#else // ! iOS
#define DEST_PLATFORM_TEXT "darwin"
#endif // iOS ?
#elif defined(__ANDROID__)
#define DEST_PLATFORM_TEXT "android"
#elif defined(__linux__)
#define DEST_PLATFORM_TEXT "posix"
#elif defined(__FreeBSD__) || defined(__unix__)
#define DEST_PLATFORM_TEXT "bsd"
#endif // FreeBSD or unix ?
#endif // _WIN32 ?

#pragma prefast(disable:26444, "This warning unfortunately raises false positives when auto is used for declaring the type of an iterator in a loop.")
#ifdef HAS_ICU
#define INTL_LIBRARY_TEXT "icu"
#elif defined(_WIN32)
#define INTL_LIBRARY_TEXT "winglob"
#else
#define INTL_LIBRARY_TEXT ""
#endif

MessageQueue* WScriptJsrt::messageQueue = nullptr;
std::map<std::string, JsModuleRecord>  WScriptJsrt::moduleRecordMap;
std::map<JsModuleRecord, std::string>  WScriptJsrt::moduleDirMap;
std::map<JsModuleRecord, ModuleState>  WScriptJsrt::moduleErrMap;
std::map<DWORD_PTR, std::string> WScriptJsrt::scriptDirMap;
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
            errorCode = ChakraRTInterface::JsCreateString(outOfMemoryString,        \
                strlen(outOfMemoryString), &errorMessageString);                    \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            errorCode = ChakraRTInterface::JsCreateString(errorMessageNarrow,       \
                strlen(errorMessageNarrow), &errorMessageString);                   \
            free(errorMessageNarrow);                                               \
        }                                                                           \
    }                                                                               \
    while(0)

DWORD_PTR WScriptJsrt::GetNextSourceContext()
{
    return sourceContext++;
}

void WScriptJsrt::RegisterScriptDir(DWORD_PTR sourceContext, LPCSTR fullDirNarrow)
{
    GetDir(fullDirNarrow, &scriptDirMap[sourceContext]);
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
        JsErrorCode errCode  = ChakraRTInterface::JsCreateString(
            argNarrow,
            strlen(argNarrow), &value);
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
            AutoString str(strValue);
            if (str.GetError() == JsNoError)
            {
                if (i > 1)
                {
                    wprintf(_u(" "));
                }
                charcount_t len;
                LPWSTR ws = str.GetWideString(&len);
                LPWSTR wsNoNull = new WCHAR[((size_t)len) + 1];
                charcount_t newIndex = 0;
                for (charcount_t j = 0; j < len; j++)
                {
                    if (ws[j] != _u('\0'))
                    {
                        wsNoNull[newIndex++] = ws[j];
                    }
                }
                wsNoNull[newIndex] = _u('\0');
                wprintf(_u("%s"), wsNoNull);
                delete[] wsNoNull;
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

// needed because of calling convention differences between _stdcall and _cdecl
void CHAKRA_CALLBACK WScriptJsrt::FinalizeFree(void* addr)
{
    free(addr);
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
        AutoString fileName(arguments[1]);
        IfJsrtErrorSetGo(fileName.GetError());

        AutoString scriptInjectType;
        if (argumentCount > 2)
        {
            IfJsrtErrorSetGo(scriptInjectType.Initialize(arguments[2]));
        }

        if (errorCode == JsNoError)
        {
            hr = Helpers::LoadScriptFromFile(*fileName, fileContent);
            if (FAILED(hr))
            {
                fprintf(stderr, "Couldn't load file '%s'\n", fileName.GetString());
                IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));
                return returnValue;
            }

            returnValue = LoadScript(callee, *fileName, fileContent, *scriptInjectType ? *scriptInjectType : "self", isSourceModule, WScriptJsrt::FinalizeFree, true);
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

JsValueRef __stdcall WScriptJsrt::GetModuleNamespace(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    JsErrorCode errorCode = JsNoError;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    LPCWSTR errorMessage = _u("");
    char fullPath[_MAX_PATH];

    if (argumentCount < 2)
    {
        errorCode = JsErrorInvalidArgument;
        errorMessage = _u("Need an argument for WScript.GetModuleNamespace");
    }
    else
    {
        AutoString specifierStr(arguments[1]);
        errorCode = specifierStr.GetError();

        if (errorCode == JsNoError)
        {
            if (_fullpath(fullPath, specifierStr.GetString(), _MAX_PATH) == nullptr)
            {
                errorCode = JsErrorInvalidArgument;
            }
            else
            {
                auto moduleEntry = moduleRecordMap.find(fullPath);
                if (moduleEntry == moduleRecordMap.end())
                {
                    errorCode = JsErrorInvalidArgument;
                    errorMessage = _u("Need to supply a path for an already loaded module for WScript.GetModuleNamespace");
                }
                else
                {
                    errorCode = ChakraRTInterface::JsGetModuleNamespace(moduleEntry->second, &returnValue);
                    if (errorCode == JsErrorModuleNotEvaluated)
                    {
                        errorMessage = _u("GetModuleNamespace called with un-evaluated module");
                    }
                }
            }
        }
    }

    if (errorCode != JsNoError)
    {
        JsValueRef errorObject;
        JsValueRef errorMessageString;

        if (wcscmp(errorMessage, _u("")) == 0)
        {
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
        char *fileNameNarrow = nullptr;
        AutoString fileName;
        AutoString scriptInjectType;
        char fileNameBuffer[MAX_PATH];
        bool isFile = true;

        IfJsrtErrorSetGo(fileContent.Initialize(arguments[1]));
        // ExternalArrayBuffer Finalize will clean this up
        // but only if we actually register a finalizecallback for this
        fileContent.MakePersistent();

        if (argumentCount > 2)
        {
            IfJsrtErrorSetGo(scriptInjectType.Initialize(arguments[2]));

            if (argumentCount > 3)
            {
                IfJsrtErrorSetGo(fileName.Initialize(arguments[3]));
                fileNameNarrow = *fileName;
            }
        }

        if (!fileNameNarrow)
        {
            isFile = false;
            if (isSourceModule)
            {
                sprintf_s(fileNameBuffer, MAX_PATH, "moduleScript%i.js", (int)sourceContext);
                fileNameNarrow = fileNameBuffer;
            }
        }

        if (*fileContent)
        {
            // TODO: This is CESU-8. How to tell the engine?
            // TODO: How to handle this source (script) life time?
            returnValue = LoadScript(callee, fileNameNarrow, *fileContent, *scriptInjectType ? *scriptInjectType : "self", isSourceModule, WScriptJsrt::FinalizeFree, isFile);
        }
    }

Error:
    if (errorCode != JsNoError)
    {
        // check and clear exception if any
        bool hasException;
        if (ChakraRTInterface::JsHasException(&hasException) == JsNoError && hasException)
        {
            JsValueRef unusedException = JS_INVALID_REFERENCE;
            ChakraRTInterface::JsGetAndClearException(&unusedException);
            unusedException;
        }

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
        errorCode = ChakraRTInterface::JsSetModuleHostInfo(moduleRecord, JsModuleHostInfo_FetchImportedModuleFromScriptCallback, (void*)WScriptJsrt::FetchImportedModuleFromScript);

        if (errorCode == JsNoError)
        {
            errorCode = ChakraRTInterface::JsSetModuleHostInfo(moduleRecord, JsModuleHostInfo_NotifyModuleReadyCallback, (void*)WScriptJsrt::NotifyModuleReadyCallback);

            if (errorCode == JsNoError)
            {
                errorCode = ChakraRTInterface::JsSetModuleHostInfo(moduleRecord, JsModuleHostInfo_HostDefined, specifier);
            }
        }
    }

    IfJsrtErrorFailLogAndRetErrorCode(errorCode);
    return JsNoError;
}

void WScriptJsrt::GetDir(LPCSTR fullPathNarrow, std::string *fullDirNarrow)
{
    char fileDrive[_MAX_DRIVE];
    char fileDir[_MAX_DIR];

    std::string result;
    if (_splitpath_s(fullPathNarrow, fileDrive, _countof(fileDrive), fileDir, _countof(fileDir), nullptr, 0, nullptr, 0) == 0)
    {
        result += fileDrive;
        result += fileDir;
    }

    *fullDirNarrow = result;
}

JsErrorCode WScriptJsrt::ModuleEntryPoint(LPCSTR fileName, LPCSTR fileContent, LPCSTR fullName)
{
    return LoadModuleFromString(fileName, fileContent, fullName, true);
}

JsErrorCode WScriptJsrt::LoadModuleFromString(LPCSTR fileName, LPCSTR fileContent, LPCSTR fullName, bool isFile)
{
    DWORD_PTR dwSourceCookie = WScriptJsrt::GetNextSourceContext();
    JsModuleRecord requestModule = JS_INVALID_REFERENCE;
    LPCSTR moduleRecordKey = fullName ? fullName : fileName;
    auto moduleRecordEntry = moduleRecordMap.find(std::string(moduleRecordKey));
    JsErrorCode errorCode = JsNoError;

    // we need to create a new moduleRecord if the specifier (fileName) is not found;
    // otherwise we'll use the old one.
    if (moduleRecordEntry == moduleRecordMap.end())
    {
        JsValueRef specifier;
        errorCode = ChakraRTInterface::JsCreateString(
            fileName, strlen(fileName), &specifier);
        if (errorCode == JsNoError)
        {
            errorCode = ChakraRTInterface::JsInitializeModuleRecord(
                nullptr, specifier, &requestModule);
        }
        if (errorCode == JsNoError)
        {
            errorCode = InitializeModuleInfo(specifier, requestModule);
        }
        if (errorCode == JsNoError)
        {
            if (fullName)
            {
                GetDir(fullName, &moduleDirMap[requestModule]);
            }

            moduleRecordMap[std::string(moduleRecordKey)] = requestModule;
            moduleErrMap[requestModule] = RootModule;
        }
    }
    else
    {
        requestModule = moduleRecordEntry->second;
    }
    IfJsrtErrorFailLogAndRetErrorCode(errorCode);
    JsValueRef errorObject = JS_INVALID_REFERENCE;

    // ParseModuleSource is sync, while additional fetch & evaluation are async.
    unsigned int fileContentLength = (fileContent == nullptr) ? 0 : (unsigned int)strlen(fileContent);
 
    if (isFile && fullName)
    {
        JsValueRef moduleUrl;
        ChakraRTInterface::JsCreateString(fullName, strlen(fullName), &moduleUrl);
        errorCode = ChakraRTInterface::JsSetModuleHostInfo(requestModule, JsModuleHostInfo_Url, moduleUrl);
        IfJsrtErrorFail(errorCode, errorCode);
    }
 
    errorCode = ChakraRTInterface::JsParseModuleSource(requestModule, dwSourceCookie, (LPBYTE)fileContent,
        fileContentLength, JsParseModuleSourceFlags_DataIsUTF8, &errorObject);
    if ((errorCode != JsNoError) && errorObject != JS_INVALID_REFERENCE && fileContent != nullptr && !HostConfigFlags::flags.IgnoreScriptErrorCode && moduleErrMap[requestModule] == RootModule)
    {
        ChakraRTInterface::JsSetException(errorObject);
        moduleErrMap[requestModule] = ErroredModule;
        return errorCode;
    }
    return JsNoError;
}


JsValueRef WScriptJsrt::LoadScript(JsValueRef callee, LPCSTR fileName,
    LPCSTR fileContent, LPCSTR scriptInjectType, bool isSourceModule, JsFinalizeCallback finalizeCallback, bool isFile)
{
    HRESULT hr = E_FAIL;
    JsErrorCode errorCode = JsNoError;
    LPCWSTR errorMessage = _u("Internal error.");
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode innerErrorCode = JsNoError;
    JsContextRef currentContext = JS_INVALID_REFERENCE;
    JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
    void *callbackArg = (finalizeCallback != nullptr ? (void*)fileContent : nullptr);

    char fullPath[_MAX_PATH];

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetCurrentContext(&currentContext));
    IfJsrtErrorSetGo(ChakraRTInterface::JsGetRuntime(currentContext, &runtime));

    if (fileName == nullptr)
    {
        fileName = "script.js";
    }

    if (_fullpath(fullPath, fileName, _MAX_PATH) == nullptr)
    {
        goto Error;
    }

    // this is called with LoadModuleCallback method as well where caller pass in a string that should be
    // treated as a module source text instead of opening a new file.
    if (isSourceModule || (strcmp(scriptInjectType, "module") == 0))
    {
        errorCode = LoadModuleFromString(fileName, fileContent, fullPath, isFile);
    }
    else if (strcmp(scriptInjectType, "self") == 0)
    {
        JsContextRef calleeContext;
        IfJsrtErrorSetGo(ChakraRTInterface::JsGetContextOfObject(callee, &calleeContext));

        IfJsrtErrorSetGo(ChakraRTInterface::JsSetCurrentContext(calleeContext));

        JsValueRef scriptSource;
        IfJsrtErrorSetGo(ChakraRTInterface::JsCreateExternalArrayBuffer((void*)fileContent,
            (unsigned int)strlen(fileContent), finalizeCallback, callbackArg, &scriptSource));
        JsValueRef fname;
        IfJsrtErrorSetGo(ChakraRTInterface::JsCreateString(fullPath,
            strlen(fullPath), &fname));
        JsSourceContext sourceContext = GetNextSourceContext();
        RegisterScriptDir(sourceContext, fullPath);

        if (HostConfigFlags::flags.UseParserStateCacheIsEnabled)
        {
            JsValueRef parserState;
            IfJsrtErrorSetGo(ChakraRTInterface::JsSerializeParserState(scriptSource, &parserState, JsParseScriptAttributeNone));
            errorCode = ChakraRTInterface::JsRunScriptWithParserState(scriptSource, sourceContext, fname, JsParseScriptAttributeNone, parserState, &returnValue);
        }
        else
        {
            errorCode = ChakraRTInterface::JsRun(scriptSource, sourceContext, fname, JsParseScriptAttributeNone, &returnValue);
        }

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

#if ENABLE_TTD
        //We need this here since this context is created in record
        IfJsrtErrorSetGo(ChakraRTInterface::JsSetObjectBeforeCollectCallback(newContext, nullptr, WScriptJsrt::JsContextBeforeCollectCallback));
#endif

        IfJsrtErrorSetGo(ChakraRTInterface::JsSetCurrentContext(newContext));

        IfJsErrorFailLog(ChakraRTInterface::JsSetPromiseContinuationCallback(PromiseContinuationCallback, (void*)messageQueue));

        // Initialize the host objects
        Initialize();

        JsValueRef scriptSource;
        IfJsrtErrorSetGo(ChakraRTInterface::JsCreateExternalArrayBuffer((void*)fileContent,
            (unsigned int)strlen(fileContent), finalizeCallback, callbackArg, &scriptSource));
        JsValueRef fname;
        IfJsrtErrorSetGo(ChakraRTInterface::JsCreateString(fullPath,
            strlen(fullPath), &fname));
        JsSourceContext sourceContext = GetNextSourceContext();
        RegisterScriptDir(sourceContext, fullPath);

        if (HostConfigFlags::flags.UseParserStateCacheIsEnabled)
        {
            JsValueRef parserState;
            IfJsrtErrorSetGo(ChakraRTInterface::JsSerializeParserState(scriptSource, &parserState, JsParseScriptAttributeNone));
            errorCode = ChakraRTInterface::JsRunScriptWithParserState(scriptSource, sourceContext, fname, JsParseScriptAttributeNone, parserState, &returnValue);
        }
        else
        {
            errorCode = ChakraRTInterface::JsRun(scriptSource, sourceContext, fname, JsParseScriptAttributeNone, &returnValue);
        }

        if (errorCode == JsNoError)
        {
            errorCode = ChakraRTInterface::JsGetGlobalObject(&returnValue);
        }

        // Set the context back to the old one
        ChakraRTInterface::JsSetCurrentContext(currentContext);
    }
    else if (strcmp(scriptInjectType, "crossthread") == 0)
    {
        auto& threadData = GetRuntimeThreadLocalData().threadData;
        if (threadData == nullptr)
        {
            threadData = new RuntimeThreadData();
        }

        RuntimeThreadData* child = new RuntimeThreadData();
        child->initialSource = fileContent;
        threadData->children.push_back(child);
        child->parent = threadData;

        // TODO: need to add a switch in case we don't need to wait for
        // child initial script completion
        ResetEvent(threadData->hevntInitialScriptCompleted);

        child->hThread = ::CreateThread(NULL, NULL, [](void* param) -> DWORD
        {
            return ((RuntimeThreadData*)param)->ThreadProc();
        }, (void*)child, NULL, NULL);

        WaitForSingleObject(threadData->hevntInitialScriptCompleted, INFINITE);
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
                bool hasException = false;
                errorCode = ChakraRTInterface::JsHasException(&hasException);
                if (errorCode == JsNoError && !hasException)
                {
                    errCode = ChakraRTInterface::JsSetException(error);
                }
                else if (errCode == JsNoError)
                {
                    errCode = JsErrorInExceptionState;
                }
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

JsValueRef WScriptJsrt::RequestAsyncBreakCallback(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
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

JsValueRef WScriptJsrt::EmptyCallback(JsValueRef callee, bool isConstructCall,
    JsValueRef * arguments, unsigned short argumentCount, void * callbackState)
{
    return JS_INVALID_REFERENCE;
}

bool WScriptJsrt::CreateNamedFunction(const char* nameString, JsNativeFunction callback,
    JsValueRef* functionVar)
{
    JsValueRef nameVar;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateString(
        nameString, strlen(nameString), &nameVar), false);
    IfJsrtErrorFail(ChakraRTInterface::JsCreateNamedFunction(nameVar, callback,
        nullptr, functionVar), false);
    return true;
}

bool WScriptJsrt::InstallObjectsOnObject(JsValueRef object, const char* name,
    JsNativeFunction nativeFunction)
{
    JsValueRef propertyValueRef;
    JsPropertyIdRef propertyId;
    IfJsrtErrorFail(CreatePropertyIdFromString(name, &propertyId), false);
    if (!CreateNamedFunction(name, nativeFunction, &propertyValueRef))
    {
        return false;
    }
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(object, propertyId,
        propertyValueRef, true), false);
    return true;
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
#ifdef HAS_ICU
    int icuVersion = PlatformAgnostic::ICUHelpers::GetICUMajorVersion();
#else
    int icuVersion = -1;
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
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "LoadBinaryFile", LoadBinaryFileCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "LoadTextFile", LoadTextFileCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Flag", FlagCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "RegisterModuleSource", RegisterModuleSourceCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "GetModuleNamespace", GetModuleNamespace));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "GetProxyProperties", GetProxyPropertiesCallback));
    
    // ToDo Remove
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Edit", EmptyCallback));

    // Platform
    JsValueRef platformObject;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateObject(&platformObject), false);
    JsPropertyIdRef platformProperty;
    IfJsrtErrorFail(CreatePropertyIdFromString("Platform", &platformProperty), false);

    // Set CPU arch
    JsPropertyIdRef archProperty;
    IfJsrtErrorFail(CreatePropertyIdFromString("ARCH", &archProperty), false);
    JsValueRef archValue;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateString(
        CPU_ARCH_TEXT, strlen(CPU_ARCH_TEXT), &archValue), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, archProperty,
        archValue, true), false);

    // Set Build Type
    JsPropertyIdRef buildProperty;
    IfJsrtErrorFail(CreatePropertyIdFromString("BUILD_TYPE", &buildProperty), false);
    JsValueRef buildValue;
#ifdef _DEBUG
#define BUILD_TYPE_STRING_CH "Debug" // (O0)
#elif defined(ENABLE_DEBUG_CONFIG_OPTIONS)
#define BUILD_TYPE_STRING_CH "Test" // (O3 with debug config options)
#else
#define BUILD_TYPE_STRING_CH "Release" // (O3)
#endif
    IfJsrtErrorFail(ChakraRTInterface::JsCreateString(
        BUILD_TYPE_STRING_CH, strlen(BUILD_TYPE_STRING_CH), &buildValue), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, buildProperty,
        buildValue, true), false);
#undef BUILD_TYPE_STRING_CH

    // Set Link Type [static / shared]
    JsPropertyIdRef linkProperty;
    IfJsrtErrorFail(CreatePropertyIdFromString("LINK_TYPE", &linkProperty), false);
    JsValueRef linkValue;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateString(
        LINK_TYPE, strlen(LINK_TYPE), &linkValue), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, linkProperty,
      linkValue, true), false);

    // Set Binary Location
    JsValueRef binaryPathValue;
    PlatformAgnostic::SystemInfo::GetBinaryLocation(CH_BINARY_LOCATION, sizeof(CH_BINARY_LOCATION));

    JsPropertyIdRef binaryPathProperty;
    IfJsrtErrorFail(CreatePropertyIdFromString("BINARY_PATH", &binaryPathProperty), false);

    IfJsrtErrorFail(ChakraRTInterface::JsCreateString(
        CH_BINARY_LOCATION,
        strlen(CH_BINARY_LOCATION), &binaryPathValue), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(
        platformObject, binaryPathProperty, binaryPathValue, true), false);

    // Set destination OS
    JsPropertyIdRef osProperty;
    IfJsrtErrorFail(CreatePropertyIdFromString("OS", &osProperty), false);
    JsValueRef osValue;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateString(
        DEST_PLATFORM_TEXT, strlen(DEST_PLATFORM_TEXT), &osValue), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, osProperty,
        osValue, true), false);

    // set Internationalization library
    JsPropertyIdRef intlLibraryProp;
    IfJsrtErrorFail(CreatePropertyIdFromString("INTL_LIBRARY", &intlLibraryProp), false);
    JsValueRef intlLibraryStr;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateString(INTL_LIBRARY_TEXT, strlen(INTL_LIBRARY_TEXT), &intlLibraryStr), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, intlLibraryProp, intlLibraryStr, true), false);
    JsPropertyIdRef icuVersionProp;
    IfJsrtErrorFail(CreatePropertyIdFromString("ICU_VERSION", &icuVersionProp), false);
    JsValueRef icuVersionNum;
    IfJsrtErrorFail(ChakraRTInterface::JsIntToNumber(icuVersion, &icuVersionNum), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(platformObject, icuVersionProp, icuVersionNum, true), false);

    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, platformProperty,
        platformObject, true), false);

    JsValueRef argsObject;

    if (!CreateArgumentsObject(&argsObject))
    {
        return false;
    }

    JsPropertyIdRef argsName;
    IfJsrtErrorFail(CreatePropertyIdFromString("Arguments", &argsName), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, argsName, argsObject, true), false);

    JsPropertyIdRef wscriptName;
    IfJsrtErrorFail(CreatePropertyIdFromString("WScript", &wscriptName), false);

    JsValueRef global;
    IfJsrtErrorFail(ChakraRTInterface::JsGetGlobalObject(&global), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(global, wscriptName, wscript, true), false);

    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(global, "print", EchoCallback));

    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(global, "read", LoadTextFileCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(global, "readbuffer", LoadBinaryFileCallback));
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(global, "readline", ReadLineStdinCallback));

    JsValueRef console;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateObject(&console), false);
    IfFalseGo(WScriptJsrt::InstallObjectsOnObject(console, "log", EchoCallback));

    JsPropertyIdRef consoleName;
    IfJsrtErrorFail(CreatePropertyIdFromString("console", &consoleName), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(global, consoleName, console, true), false);

    IfJsrtErrorFail(InitializeModuleCallbacks(), false);

    // When the command-line argument `-Test262` is set,
    // WScript will have the extra support API below and $262 will be
    // added to global scope
    if (HostConfigFlags::flags.Test262)
    {
        IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Broadcast", BroadcastCallback));
        IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "ReceiveBroadcast", ReceiveBroadcastCallback));
        IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Report", ReportCallback));
        IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "GetReport", GetReportCallback));
        IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Leaving", LeavingCallback));
        IfFalseGo(WScriptJsrt::InstallObjectsOnObject(wscript, "Sleep", SleepCallback));

        // $262
        const char Test262[] =
            #include "262.js"
        ;

        JsValueRef Test262ScriptRef;
        IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCreateString(Test262, strlen(Test262), &Test262ScriptRef));

        JsValueRef fname;
        IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsCreateString("262", strlen("262"), &fname));
        IfJsrtErrorFailLogAndRetFalse(ChakraRTInterface::JsRun(Test262ScriptRef, WScriptJsrt::GetNextSourceContext(), fname, JsParseScriptAttributeNone, nullptr));
    }

Error:
    return hr == S_OK;
}

JsErrorCode WScriptJsrt::InitializeModuleCallbacks()
{
    JsModuleRecord moduleRecord = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = ChakraRTInterface::JsInitializeModuleRecord(nullptr, nullptr, &moduleRecord);
    if (errorCode == JsNoError)
    {
        errorCode = InitializeModuleInfo(nullptr, moduleRecord);
    }

    return errorCode;
}

bool WScriptJsrt::Uninitialize()
{
    // moduleRecordMap is a global std::map, its destructor may access overridden
    // "operator delete" / global HeapAllocator::Instance. Clear it manually here
    // to avoid worrying about global destructor order.
    moduleRecordMap.clear();
    moduleDirMap.clear();
    moduleErrMap.clear();
    scriptDirMap.clear();

    auto& threadData = GetRuntimeThreadLocalData().threadData;
    if (threadData && !threadData->children.empty())
    {
        LONG count = (LONG)threadData->children.size();
        std::vector<HANDLE> childrenHandles;

        //Clang does not support "for each" yet
        for(auto i = threadData->children.begin(); i!= threadData->children.end(); i++)
        {
            auto child = *i;
            childrenHandles.push_back(child->hThread);
            SetEvent(child->hevntShutdown);
        }

        DWORD waitRet = WaitForMultipleObjects(count, &childrenHandles[0], TRUE, INFINITE);
        Assert(waitRet == WAIT_OBJECT_0);

        for (auto i = threadData->children.begin(); i != threadData->children.end(); i++)
        {
            delete *i;
        }

        threadData->children.clear();
    }

    return true;
}

#if ENABLE_TTD
void CALLBACK WScriptJsrt::JsContextBeforeCollectCallback(JsRef contextRef, void *data)
{
    ChakraRTInterface::JsTTDNotifyContextDestroy(contextRef);
}
#endif

FileNode * SourceMap::root = nullptr;
JsValueRef __stdcall WScriptJsrt::RegisterModuleSourceCallback(JsValueRef callee, bool isConstructCall,
    JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    if (argumentCount < 3)
    {
        IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));
    }
    else
    {
        AutoString fileName;
        AutoString data;
        IfJsrtErrorSetGo(fileName.Initialize(arguments[1]));
        IfJsrtErrorSetGo(data.Initialize(arguments[2]));

        SourceMap::Add(fileName, data);
    }

Error:
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::LoadTextFileCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;
    const char* fileContent = nullptr;

    if (argumentCount < 2)
    {
        IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));
    }
    else
    {
        AutoString fileName;

        IfJsrtErrorSetGo(fileName.Initialize(arguments[1]));

        if (errorCode == JsNoError)
        {
            UINT lengthBytes = 0;
            hr = Helpers::LoadScriptFromFile(*fileName, fileContent, &lengthBytes);

            if (FAILED(hr))
            {
                fprintf(stderr, "Couldn't load file '%s'\n", fileName.GetString());
                IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));
                return returnValue;
            }

            IfJsrtErrorSetGo(ChakraRTInterface::JsCreateString(
                fileContent, lengthBytes, &returnValue));
        }
    }

Error:
    if (fileContent)
    {
        free((void*)fileContent);
    }
    return returnValue;
}

int JsFgets(char* buf, int size, FILE* file)
{
    int n = size - 1;
    if (n < 0)
        return -1;

    bool crflag = false;
    int c, i = 0;

    for (i = 0; i < n && (c = getc(file)) != EOF; i++) {
        buf[i] = (char)c;
        if (c == '\n') {        // any \n ends a line
            i++;                // keep the \n; we know there is room for \0
            break;
        }
        if (crflag) {           // \r not followed by \n ends line at the \r
            ungetc(c, file);
            break;              // and overwrite c in buf with \0
        }
        crflag = (c == '\r');
    }

    buf[i] = '\0';
    return i;
}

JsValueRef __stdcall WScriptJsrt::ReadLineStdinCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    const int BUFSIZE = 256;
    FILE* from = stdin;
    int buflength = 0;
    int bufsize = BUFSIZE;
    char* buf = static_cast<char*>(malloc(bufsize));
    char* tmp;
    int gotlength = 0;

    if (!buf)
    {
        goto Error;
    }

    while ((gotlength = JsFgets(buf + buflength, bufsize - buflength, from)) > 0) 
    {
        buflength += gotlength;

        // are we done?
        if (buf[buflength - 2] == '\r' && buf[buflength - 1] == '\n') 
        {
            buf[buflength - 1] = '\0';
            buf[buflength - 2] = '\0';
            buflength -= 2;
            break;
        }
        else if (buf[buflength - 1] == '\n') 
        {
            buf[buflength - 1] = '\0';
            buflength -= 1;
            break;
        }
        else if (buflength < bufsize - 1) 
        {
            break;
        }

        // Else, grow our buffer for another pass.
        bufsize *= 2;
        if (bufsize > buflength) 
        {
            tmp = static_cast<char*>(realloc(buf, bufsize));
        }
        else 
        {
            goto Error;
        }

        if (!tmp) 
        {
            goto Error;
        }

        buf = tmp;
    }

    //Treat the empty string specially.
    if (buflength == 0) 
    {
        if (feof(from)) 
        {
            goto Error;
        }
        else 
        {
            JsValueRef emptyStringObject;
            IfJsrtErrorSetGo(ChakraRTInterface::JsCreateString(buf, buflength, &emptyStringObject));
            free(buf);
            return emptyStringObject;
        }
    }

   
    // Turn buf into a JSString. Note that buflength includes the trailing null character.
    JsValueRef stringObject;
    IfJsrtErrorSetGo(ChakraRTInterface::JsCreateString(buf, buflength, &stringObject));
    free(buf);
    return stringObject;

Error:
    if (buf)
    {
        free(buf);
    }
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::LoadBinaryFileCallback(JsValueRef callee,
    bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;
    bool isHeapAlloc = true;

    if (argumentCount < 2)
    {
        IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));
    }
    else
    {
        const char *fileContent;
        AutoString fileName;

        IfJsrtErrorSetGo(fileName.Initialize(arguments[1]));

        if (errorCode == JsNoError)
        {
            UINT lengthBytes = 0;

            hr = Helpers::LoadBinaryFile(*fileName, fileContent, lengthBytes);

            if (FAILED(hr))
            {
                fprintf(stderr, "Couldn't load file '%s'\n", fileName.GetString());
                IfJsrtErrorSetGoLabel(ChakraRTInterface::JsGetUndefinedValue(&returnValue), Error);
                return returnValue;
            }

            JsValueRef arrayBuffer;
            IfJsrtErrorSetGoLabel(ChakraRTInterface::JsCreateArrayBuffer(lengthBytes, &arrayBuffer), ErrorStillFree);
            BYTE* buffer;
            unsigned int bufferLength;
            IfJsrtErrorSetGoLabel(ChakraRTInterface::JsGetArrayBufferStorage(arrayBuffer, &buffer, &bufferLength), ErrorStillFree);
            if (bufferLength < lengthBytes)
            {
                fwprintf(stderr, _u("Array buffer size is insufficient to store the binary file.\n"));
            }
            else
            {
                if (memcpy_s(buffer, bufferLength, (BYTE*)fileContent, lengthBytes) == 0)
                {
                    returnValue = arrayBuffer;
                }
            }
ErrorStillFree:
            if (isHeapAlloc)
            {
                HeapFree(GetProcessHeap(), 0, (void*)fileContent);
            }
        }
    }

Error:
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::FlagCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));

#if ENABLE_DEBUG_CONFIG_OPTIONS
    if (argumentCount > 1)
    {
        AutoString cmd;
        IfJsrtErrorSetGo(cmd.Initialize(arguments[1]));
        char16* argv[] = { nullptr, cmd.GetWideString() };
        ChakraRTInterface::SetConfigFlags(2, argv, nullptr);
    }
#endif

Error:
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::BroadcastCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));

    if (argumentCount > 1)
    {
        auto& threadData = GetRuntimeThreadLocalData().threadData;
        if (threadData)
        {
            ChakraRTInterface::JsGetSharedArrayBufferContent(arguments[1], &threadData->sharedContent);

            LONG count = (LONG)threadData->children.size();
            threadData->hSemaphore = CreateSemaphore(NULL, 0, count, NULL);
            if (threadData->hSemaphore)
            {
                //Clang does not support "for each" yet
                for (auto i = threadData->children.begin(); i != threadData->children.end(); i++)
                {
                    auto child = *i;
                    SetEvent(child->hevntReceivedBroadcast);
                }

                WaitForSingleObject(threadData->hSemaphore, INFINITE);
                CloseHandle(threadData->hSemaphore);
                threadData->hSemaphore = INVALID_HANDLE_VALUE;
            }
            else
            {
                fwprintf(stderr, _u("Couldn't create semaphore.\n"));
                fflush(stderr);
            }

            ChakraRTInterface::JsReleaseSharedArrayBufferContentHandle(threadData->sharedContent);
        }
    }

Error:
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::ReceiveBroadcastCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));

    if (argumentCount > 1)
    {
        auto& threadData = GetRuntimeThreadLocalData().threadData;
        if (threadData)
        {
            if (threadData->receiveBroadcastCallbackFunc)
            {
                ChakraRTInterface::JsRelease(threadData->receiveBroadcastCallbackFunc, nullptr);
            }
            threadData->receiveBroadcastCallbackFunc = arguments[1];
            ChakraRTInterface::JsAddRef(threadData->receiveBroadcastCallbackFunc, nullptr);
        }
    }

Error:
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::ReportCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));

    if (argumentCount > 1)
    {
        JsValueRef stringRef;
        ChakraRTInterface::JsConvertValueToString(arguments[1], &stringRef);

        AutoString autoStr(stringRef);

        if (autoStr.GetError() == JsNoError)
        {
            std::string str(autoStr.GetString());
            auto& threadData = GetRuntimeThreadLocalData().threadData;

            if (threadData && threadData->parent)
            {
                EnterCriticalSection(&threadData->parent->csReportQ);
                threadData->parent->reportQ.push_back(str);
                LeaveCriticalSection(&threadData->parent->csReportQ);
            }
        }
    }

Error:
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::GetReportCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetNullValue(&returnValue));

    if (argumentCount > 0)
    {
        auto& threadData = GetRuntimeThreadLocalData().threadData;
        if (threadData)
        {
            EnterCriticalSection(&threadData->csReportQ);

            if (threadData->reportQ.size() > 0)
            {
                auto str = threadData->reportQ.front();
                threadData->reportQ.pop_front();
                ChakraRTInterface::JsCreateString(str.c_str(), str.size(), &returnValue);
            }
            LeaveCriticalSection(&threadData->csReportQ);
        }
    }

Error:
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::LeavingCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));

    if (argumentCount > 0)
    {
        auto& threadData = GetRuntimeThreadLocalData().threadData;
        if (threadData)
        {
            threadData->leaving = true;
        }
    }

Error:
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::SleepCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&returnValue));

    if (argumentCount > 1)
    {
        double timeout = 0.0;
        ChakraRTInterface::JsNumberToDouble(arguments[1], &timeout);
        Sleep((DWORD)timeout);
    }

Error:
    return returnValue;
}

JsValueRef __stdcall WScriptJsrt::GetProxyPropertiesCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    HRESULT hr = E_FAIL;
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsValueRef undefined = JS_INVALID_REFERENCE;
    JsErrorCode errorCode = JsNoError;

    IfJsrtErrorSetGo(ChakraRTInterface::JsGetUndefinedValue(&undefined));

    returnValue = undefined;

    if (argumentCount > 1)
    {
        bool isProxy = false;
        JsValueRef target;
        JsValueRef handler;
        IfJsrtErrorSetGo(ChakraRTInterface::JsGetProxyProperties(arguments[1], &isProxy, &target, &handler));

        if (isProxy)
        {
            JsPropertyIdRef targetProperty;
            JsPropertyIdRef handlerProperty;
            JsPropertyIdRef revokedProperty;
            
            IfJsrtErrorSetGo(CreatePropertyIdFromString("target", &targetProperty));
            IfJsrtErrorSetGo(CreatePropertyIdFromString("handler", &handlerProperty));
            IfJsrtErrorSetGo(CreatePropertyIdFromString("revoked", &revokedProperty));
            IfJsrtErrorSetGo(ChakraRTInterface::JsCreateObject(&returnValue));

            JsValueRef revoked = JS_INVALID_REFERENCE;

            if (target == JS_INVALID_REFERENCE)
            {
                IfJsrtErrorSetGo(ChakraRTInterface::JsGetTrueValue(&revoked));
                target = undefined;
                handler = undefined;
            }
            else
            {
                IfJsrtErrorSetGo(ChakraRTInterface::JsGetFalseValue(&revoked));
            }

            IfJsrtErrorSetGo(ChakraRTInterface::JsSetProperty(returnValue, handlerProperty, handler, true));
            IfJsrtErrorSetGo(ChakraRTInterface::JsSetProperty(returnValue, targetProperty, target, true));
            IfJsrtErrorSetGo(ChakraRTInterface::JsSetProperty(returnValue, revokedProperty, revoked, true));
        }
    }
Error:
    return returnValue;
}

bool WScriptJsrt::PrintException(LPCSTR fileName, JsErrorCode jsErrorCode)
{
    LPCWSTR errorTypeString = ConvertErrorCodeToMessage(jsErrorCode);
    JsValueRef exception;
    ChakraRTInterface::JsGetAndClearException(&exception);
    if (HostConfigFlags::flags.MuteHostErrorMsgIsEnabled)
    {
        return false;
    }

    if (exception != nullptr)
    {
        if (jsErrorCode == JsErrorCode::JsErrorScriptCompile || jsErrorCode == JsErrorCode::JsErrorScriptException)
        {
            AutoString errorMessage;

            IfJsrtErrorFail(errorMessage.Initialize(exception), false);

            if (jsErrorCode == JsErrorCode::JsErrorScriptCompile)
            {
                JsPropertyIdRef linePropertyId = JS_INVALID_REFERENCE;
                JsValueRef lineProperty = JS_INVALID_REFERENCE;

                JsPropertyIdRef columnPropertyId = JS_INVALID_REFERENCE;
                JsValueRef columnProperty = JS_INVALID_REFERENCE;

                int line;
                int column;
                
                IfJsrtErrorFail(CreatePropertyIdFromString("line", &linePropertyId), false);
                IfJsrtErrorFail(ChakraRTInterface::JsGetProperty(exception, linePropertyId, &lineProperty), false);
                IfJsrtErrorFail(ChakraRTInterface::JsNumberToInt(lineProperty, &line), false);

                IfJsrtErrorFail(CreatePropertyIdFromString("column", &columnPropertyId), false);
                IfJsrtErrorFail(ChakraRTInterface::JsGetProperty(exception, columnPropertyId, &columnProperty), false);
                IfJsrtErrorFail(ChakraRTInterface::JsNumberToInt(columnProperty, &column), false);

                CHAR shortFileName[_MAX_PATH];
                CHAR ext[_MAX_EXT];
                _splitpath_s(fileName, nullptr, 0, nullptr, 0, shortFileName, _countof(shortFileName), ext, _countof(ext));
                fwprintf(stderr, _u("%ls\n\tat code (%S%S:%d:%d)\n"),
                    errorMessage.GetWideString(), shortFileName, ext, (int)line + 1,
                    (int)column + 1);
            }
            else
            {
                JsValueType propertyType = JsUndefined;
                JsPropertyIdRef stackPropertyId = JS_INVALID_REFERENCE;
                JsValueRef stackProperty = JS_INVALID_REFERENCE;
                AutoString errorStack;

                JsErrorCode errorCode = CreatePropertyIdFromString("stack", &stackPropertyId);

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

                    CHAR shortFileName[_MAX_PATH];
                    CHAR ext[_MAX_EXT];
                    _splitpath_s(fName, nullptr, 0, nullptr, 0, shortFileName, _countof(shortFileName), ext, _countof(ext));

                    // do not mix char/wchar. print them separately
                    fprintf(stderr, "thrown at %s%s:\n^\n", shortFileName, ext);
                    fwprintf(stderr, _u("%ls\n"), errorMessage.GetWideString());
                }
                else
                {
                    IfJsrtErrorFail(errorStack.Initialize(stackProperty), false);
                    fwprintf(stderr, _u("%ls\n"), errorStack.GetWideString());
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
        IfJsrtErrorHR(ChakraRTInterface::JsConvertValueToString(m_function, &stringValue));

        JsValueRef fname;
        ChakraRTInterface::JsCreateString("", strlen(""), &fname);
        // Run the code
        errorCode = ChakraRTInterface::JsRun(stringValue, JS_SOURCE_CONTEXT_NONE,
          fname, JsParseScriptAttributeArrayBufferIsUtf16Encoded,
          nullptr /*no result needed*/);
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

WScriptJsrt::ModuleMessage::ModuleMessage(JsModuleRecord module, JsValueRef specifier, std::string* fullPathPtr)
    : MessageBase(0), moduleRecord(module), specifier(specifier)
{
    fullPath = nullptr;
    ChakraRTInterface::JsAddRef(module, nullptr);
    if (specifier != nullptr)
    {
        fullPath = new std::string (*fullPathPtr);
        // nullptr specifier means a Promise to execute; non-nullptr means a "fetch" operation.
        ChakraRTInterface::JsAddRef(specifier, nullptr);
    }
}

WScriptJsrt::ModuleMessage::~ModuleMessage()
{
    ChakraRTInterface::JsRelease(moduleRecord, nullptr);
    if (specifier != nullptr)
    {
        delete fullPath;
        ChakraRTInterface::JsRelease(specifier, nullptr);
    }
}

HRESULT WScriptJsrt::ModuleMessage::Call(LPCSTR fileName)
{
    JsErrorCode errorCode = JsNoError;
    JsValueRef result = JS_INVALID_REFERENCE;
    HRESULT hr;
    if (specifier == nullptr)
    {
        if (moduleErrMap[moduleRecord] != ErroredModule)
        {
            errorCode = ChakraRTInterface::JsModuleEvaluation(moduleRecord, &result);
            if (errorCode != JsNoError)
            {
                if (moduleErrMap[moduleRecord] == RootModule)
                {
                    PrintException(fileName, errorCode);
                }
                else
                {
                    bool hasException = false;
                    ChakraRTInterface::JsHasException(&hasException);
                    if (hasException)
                    {
                        JsValueRef exception;
                        ChakraRTInterface::JsGetAndClearException(&exception);
                        exception; //unusued
                    }
                }
            }
        }
    }
    else
    {
        LPCSTR fileContent = nullptr;
        AutoString specifierStr(specifier);
        errorCode = specifierStr.GetError();
        if (errorCode == JsNoError)
        {
            hr = Helpers::LoadScriptFromFile(*specifierStr, fileContent, nullptr, fullPath, true);

            if (FAILED(hr))
            {
                if (!HostConfigFlags::flags.MuteHostErrorMsgIsEnabled)
                {
                    auto actualModuleRecord = moduleRecordMap.find(*fullPath);
                    if (actualModuleRecord == moduleRecordMap.end() || moduleErrMap[actualModuleRecord->second] == RootModule)
                    {
                        fprintf(stderr, "Couldn't load file '%s'\n", specifierStr.GetString());
                    }
                }
                LoadScript(nullptr, fullPath == nullptr ? *specifierStr : fullPath->c_str(), nullptr, "module", true, WScriptJsrt::FinalizeFree, false);
                goto Error;
            }
            LoadScript(nullptr, fullPath == nullptr ? *specifierStr : fullPath->c_str(), fileContent, "module", true, WScriptJsrt::FinalizeFree, true);
        }
    }
Error:
    return errorCode;
}

JsErrorCode WScriptJsrt::FetchImportedModuleHelper(JsModuleRecord referencingModule,
    JsValueRef specifier, __out JsModuleRecord* dependentModuleRecord, LPCSTR refdir)
{
    JsModuleRecord moduleRecord = JS_INVALID_REFERENCE;
    AutoString specifierStr;
    *dependentModuleRecord = nullptr;

    if (specifierStr.Initialize(specifier) != JsNoError)
    {
        return specifierStr.GetError();
    }

    char fullPath[_MAX_PATH];
    std::string specifierFullPath = refdir ? refdir : "";
    specifierFullPath += *specifierStr;
    if (_fullpath(fullPath, specifierFullPath.c_str(), _MAX_PATH) == nullptr)
    {
        return JsErrorInvalidArgument;
    }

    auto moduleEntry = moduleRecordMap.find(std::string(fullPath));
    if (moduleEntry != moduleRecordMap.end())
    {
        *dependentModuleRecord = moduleEntry->second;
        return JsNoError;
    }

    JsErrorCode errorCode = ChakraRTInterface::JsInitializeModuleRecord(referencingModule, specifier, &moduleRecord);
    if (errorCode == JsNoError)
    {
        GetDir(fullPath, &moduleDirMap[moduleRecord]);
        InitializeModuleInfo(specifier, moduleRecord);
        std::string pathKey = std::string(fullPath);
        moduleRecordMap[pathKey] = moduleRecord;
        moduleErrMap[moduleRecord] = ImportedModule;
        ModuleMessage* moduleMessage = WScriptJsrt::ModuleMessage::Create(referencingModule, specifier, &pathKey);
        if (moduleMessage == nullptr)
        {
            return JsErrorOutOfMemory;
        }
        WScriptJsrt::PushMessage(moduleMessage);
        *dependentModuleRecord = moduleRecord;
    }
    return errorCode;
}

// Callback from chakracore to fetch dependent module. In the test harness,
// we are not doing any translation, just treat the specifier as fileName.
// While this call will come back directly from ParseModuleSource, the additional
// task are treated as Promise that will be executed later.
JsErrorCode WScriptJsrt::FetchImportedModule(_In_ JsModuleRecord referencingModule,
    _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord)
{
    auto moduleDirEntry = moduleDirMap.find(referencingModule);
    if (moduleDirEntry != moduleDirMap.end())
    {
        std::string dir = moduleDirEntry->second;
        return FetchImportedModuleHelper(referencingModule, specifier, dependentModuleRecord, dir.c_str());
    }

    return FetchImportedModuleHelper(referencingModule, specifier, dependentModuleRecord);
}

// Callback from chakracore to fetch module dynamically during runtime. In the test harness,
// we are not doing any translation, just treat the specifier as fileName.
// While this call will come back directly from runtime script or module code, the additional
// task can be scheduled asynchronously that executed later.
JsErrorCode WScriptJsrt::FetchImportedModuleFromScript(_In_ JsSourceContext dwReferencingSourceContext,
    _In_ JsValueRef specifier, _Outptr_result_maybenull_ JsModuleRecord* dependentModuleRecord)
{
    return FetchImportedModuleHelper(nullptr, specifier, dependentModuleRecord);
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
        if (specifier != JS_INVALID_REFERENCE)
        {
            fileName.Initialize(specifier);
        }

        if (HostConfigFlags::flags.TraceHostCallbackIsEnabled)
        {
            wprintf(_u("NotifyModuleReadyCallback(exception) %S\n"), fileName.GetString());
        }

        // No need to print - just consume the exception
        JsValueRef exception;
        ChakraRTInterface::JsGetAndClearException(&exception);
        exception; // unused
    }
    
    if (exceptionVar != nullptr || moduleErrMap[referencingModule] != ErroredModule)
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

void WScriptJsrt::PromiseContinuationCallback(JsValueRef task, void *callbackState)
{
    Assert(task != JS_INVALID_REFERENCE);
    Assert(callbackState != JS_INVALID_REFERENCE);
    MessageQueue * messageQueue = (MessageQueue *)callbackState;

    WScriptJsrt::CallbackMessage *msg = new WScriptJsrt::CallbackMessage(0, task);
    messageQueue->InsertSorted(msg);
}

void WScriptJsrt::PromiseRejectionTrackerCallback(JsValueRef promise, JsValueRef reason, bool handled, void *callbackState)
{
    Assert(promise != JS_INVALID_REFERENCE);
    Assert(reason != JS_INVALID_REFERENCE);

    if (!handled)
    {
        wprintf(_u("Uncaught promise rejection\n"));
    }
    else
    {
        wprintf(_u("Promise rejection handled\n"));
    }

    JsPropertyIdRef stackPropertyID; 
    JsErrorCode error = ChakraRTInterface::JsCreatePropertyId("stack", strlen("stack"), &stackPropertyID);
    if (error == JsNoError)
    {
        JsValueRef stack;
        error = ChakraRTInterface::JsGetProperty(reason, stackPropertyID, &stack);
        if (error == JsNoError)
        {
            JsValueRef stackStrValue;
            error = ChakraRTInterface::JsConvertValueToString(stack, &stackStrValue);
            if (error == JsNoError)
            {
                AutoString str(stackStrValue);
                wprintf(_u("%ls\n"), str.GetWideString());
            }
        }
    }
    
    if (error != JsNoError)
    {
        // weren't able to print stack, so just convert reason to a string
        JsValueRef strValue;
        error = ChakraRTInterface::JsConvertValueToString(reason, &strValue);
        if (error == JsNoError)
        {
            AutoString str(strValue);
            wprintf(_u("%ls\n"), str.GetWideString());
        }
    }

    fflush(stdout);
}

