//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

MessageQueue* WScriptJsrt::messageQueue = nullptr;
DWORD_PTR WScriptJsrt::sourceContext = 0;

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

        IfJsrtErrorFail(ChakraRTInterface::JsPointerToString(argv[i], wcslen(argv[i]), &value), false);
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
            LPCWSTR str = nullptr;
            size_t length;
            error = ChakraRTInterface::JsStringToPointer(strValue, &str, &length);
            if (error == JsNoError)
            {
                if (i > 1)
                {
                    wprintf(_u(" "));
                }
                wprintf(_u("%ls"), str);
            }
        }

        if (error == JsErrorScriptException)
        {
            return nullptr;
        }
    }

    wprintf(_u("\n"));

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

JsValueRef __stdcall WScriptJsrt::LoadModuleFileCallback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount, void *callbackState)
{
    return LoadScriptFileHelper(callee, arguments, argumentCount, true);
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
        const wchar_t *fileContent;
        const wchar_t *fileName;
        const wchar_t *scriptInjectType = _u("self");
        size_t fileNameLength;
        size_t scriptInjectTypeLength;
        char *fileNameNarrow;

        IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointer(arguments[1], &fileName, &fileNameLength));

        if (argumentCount > 2)
        {
            IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointer(arguments[2], &scriptInjectType, &scriptInjectTypeLength));
        }

        if (errorCode == JsNoError)
        {
            IfFailGo(Helpers::WideStringToNarrowDynamic(fileName, &fileNameNarrow));
            hr = Helpers::LoadScriptFromFile(fileNameNarrow, fileContent);
            if (FAILED(hr))
            {
                fwprintf(stderr, _u("Couldn't load file.\n"));
            }
            else
            {
                returnValue = LoadScript(callee, fileNameNarrow, fileContent, scriptInjectType, isSourceModule);
                free(fileNameNarrow);
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

        ChakraRTInterface::JsPointerToString(errorMessage, wcslen(errorMessage), &errorMessageString);
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
        const wchar_t *fileContent;
        char *fileName = (char*) "script.js";
        const wchar_t *scriptInjectType = _u("self");
        size_t fileContentLength;
        size_t scriptInjectTypeLength;
        bool freeFileName = false;

        IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointer(arguments[1], &fileContent, &fileContentLength));

        if (argumentCount > 2)
        {
            IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointer(arguments[2], &scriptInjectType, &scriptInjectTypeLength));

            if (argumentCount > 3)
            {
                size_t fileNameWideLength = 0;
                const wchar_t* fileNameWide = nullptr;
                IfJsrtErrorSetGo(ChakraRTInterface::JsStringToPointer(arguments[3], &fileNameWide, &fileNameWideLength));
                IfFailGo(Helpers::WideStringToNarrowDynamic(fileNameWide, &fileName));
                freeFileName = true;
            }
        }
        
        returnValue = LoadScript(callee, fileName, fileContent, scriptInjectType, isSourceModule);

        if (freeFileName)
        {
            free(fileName);
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

        ChakraRTInterface::JsPointerToString(errorMessage, wcslen(errorMessage), &errorMessageString);
        ChakraRTInterface::JsCreateError(errorMessageString, &errorObject);
        ChakraRTInterface::JsSetException(errorObject);
    }

    return returnValue;
}

JsValueRef WScriptJsrt::LoadScript(JsValueRef callee, LPCSTR fileName, LPCWSTR fileContent, LPCWSTR scriptInjectType, bool isSourceModule)
{
    HRESULT hr = E_FAIL;
    JsErrorCode errorCode = JsNoError;
    LPCWSTR errorMessage = _u("Internal error.");
    size_t errorMessageLength = wcslen(errorMessage);
    JsValueRef returnValue = JS_INVALID_REFERENCE;
    JsErrorCode innerErrorCode = JsNoError;
    JsContextRef currentContext = JS_INVALID_REFERENCE;
    JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
    
    wchar_t* fullPath = nullptr;
    char fullPathNarrow[_MAX_PATH];
    size_t len = 0;
    
    IfJsrtErrorSetGo(ChakraRTInterface::JsGetCurrentContext(&currentContext));
    IfJsrtErrorSetGo(ChakraRTInterface::JsGetRuntime(currentContext, &runtime));

    if (_fullpath(fullPathNarrow, fileName, _MAX_PATH) == nullptr)
    {
        IfFailGo(E_FAIL);
    }
    // canonicalize that path name to lower case for the profile storage
    // REVIEW: This doesn't work for UTF8...
    len = strlen(fullPathNarrow);
    for (size_t i = 0; i < len; i++)
    {
        fullPathNarrow[i] = (char) tolower(fullPathNarrow[i]);
    }

    // TODO: Remove when we have utf8 versions of the Jsrt APIs
    Helpers::NarrowStringToWideDynamic(fullPathNarrow, &fullPath);
    if (wcscmp(scriptInjectType, _u("self")) == 0)
    {
        JsContextRef calleeContext;
        IfJsrtErrorSetGo(ChakraRTInterface::JsGetContextOfObject(callee, &calleeContext));

        IfJsrtErrorSetGo(ChakraRTInterface::JsSetCurrentContext(calleeContext));

        if (isSourceModule)
        {
            errorCode = ChakraRTInterface::JsRunModule(fileContent, GetNextSourceContext(), fullPath, &returnValue);
        }
        else
        {
            errorCode = ChakraRTInterface::JsRunScript(fileContent, GetNextSourceContext(), fullPath, &returnValue);
        }

        if (errorCode == JsNoError)
        {
            errorCode = ChakraRTInterface::JsGetGlobalObject(&returnValue);
        }

        IfJsrtErrorSetGo(ChakraRTInterface::JsSetCurrentContext(currentContext));
    }
    else if (wcscmp(scriptInjectType, _u("samethread")) == 0)
    {
        JsValueRef newContext = JS_INVALID_REFERENCE;

        // Create a new context and set it as the current context
        IfJsrtErrorSetGo(ChakraRTInterface::JsCreateContext(runtime, &newContext));
        IfJsrtErrorSetGo(ChakraRTInterface::JsSetCurrentContext(newContext));

        // Initialize the host objects
        Initialize();


        if (isSourceModule)
        {
            errorCode = ChakraRTInterface::JsRunModule(fileContent, GetNextSourceContext(), fullPath, &returnValue);
        }
        else
        {
            errorCode = ChakraRTInterface::JsRunScript(fileContent, GetNextSourceContext(), fullPath, &returnValue);
        }

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
        errorMessageLength = wcslen(errorMessage);
        innerErrorCode = ChakraRTInterface::JsPointerToString(errorMessage, errorMessageLength, &messageProperty);
        if (innerErrorCode == JsNoError)
        {
            innerErrorCode = ChakraRTInterface::JsCreateError(messageProperty, &error);
            if (innerErrorCode == JsNoError)
            {
                innerErrorCode = ChakraRTInterface::JsSetException(error);
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

    JsErrorCode errorCode = ChakraRTInterface::JsPointerToString(errorMessage, wcslen(errorMessage), &errorMessageString);

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

    JsErrorCode errorCode = ChakraRTInterface::JsPointerToString(errorMessage, wcslen(errorMessage), &errorMessageString);

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

bool WScriptJsrt::CreateNamedFunction(const wchar_t* nameString, JsNativeFunction callback, JsValueRef* functionVar)
{
    JsValueRef nameVar;
    IfJsrtErrorFail(ChakraRTInterface::JsPointerToString(nameString, wcslen(nameString), &nameVar), false);
    IfJsrtErrorFail(ChakraRTInterface::JsCreateNamedFunction(nameVar, callback, nullptr, functionVar), false);
    return true;
}

bool WScriptJsrt::Initialize()
{
    JsValueRef wscript;
    IfJsrtErrorFail(ChakraRTInterface::JsCreateObject(&wscript), false);

    JsValueRef echo;
    JsPropertyIdRef echoPropertyId;
    const wchar_t* echoString = _u("Echo");
    CreateNamedFunction(echoString, EchoCallback, &echo);
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(echoString, &echoPropertyId), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, echoPropertyId, echo, true), false);

    JsValueRef argsObject;

    if (!CreateArgumentsObject(&argsObject))
    {
        return false;
    }

    JsPropertyIdRef argsName;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(_u("Arguments"), &argsName), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, argsName, argsObject, true), false);

    JsValueRef quit;
    const wchar_t* quitString = _u("Quit");
    JsPropertyIdRef quitPropertyId;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(quitString, &quitPropertyId), false);
    CreateNamedFunction(quitString, QuitCallback, &quit);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, quitPropertyId, quit, true), false);

    JsValueRef loadScriptFile;
    const wchar_t* loadScriptFileString = _u("LoadScriptFile");
    JsPropertyIdRef loadScriptFilePropertyId;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(loadScriptFileString, &loadScriptFilePropertyId), false);
    CreateNamedFunction(loadScriptFileString, LoadScriptFileCallback, &loadScriptFile);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, loadScriptFilePropertyId, loadScriptFile, true), false);

    JsValueRef loadModuleFile;
    const wchar_t* loadModuleFileString = _u("LoadModuleFile");
    JsPropertyIdRef loadModuleFilePropertyId;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(loadModuleFileString, &loadModuleFilePropertyId), false);
    CreateNamedFunction(loadModuleFileString, LoadModuleFileCallback, &loadModuleFile);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, loadModuleFilePropertyId, loadModuleFile, true), false);

    JsValueRef loadScript;
    JsPropertyIdRef loadScriptName;
    const wchar_t* loadScriptString = _u("LoadScript");
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(loadScriptString, &loadScriptName), false);
    CreateNamedFunction(loadScriptString, LoadScriptCallback, &loadScript);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, loadScriptName, loadScript, true), false);

    JsValueRef loadModule;
    JsPropertyIdRef loadModuleName;
    const wchar_t* loadModuleString = _u("LoadModule");
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(loadModuleString, &loadModuleName), false);
    CreateNamedFunction(loadModuleString, LoadModuleCallback, &loadModule);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, loadModuleName, loadModule, true), false);

    JsValueRef setTimeout;
    JsPropertyIdRef setTimeoutName;
    const wchar_t* setTimeoutString = _u("SetTimeout");
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(setTimeoutString, &setTimeoutName), false);
    CreateNamedFunction(setTimeoutString, SetTimeoutCallback, &setTimeout);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, setTimeoutName, setTimeout, true), false);

    JsValueRef clearTimeout;
    JsPropertyIdRef clearTimeoutName;
    const wchar_t* clearTimeoutString = _u("ClearTimeout");
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(clearTimeoutString, &clearTimeoutName), false);
    CreateNamedFunction(clearTimeoutString, ClearTimeoutCallback, &clearTimeout);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(wscript, clearTimeoutName, clearTimeout, true), false);

    JsPropertyIdRef wscriptName;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(_u("WScript"), &wscriptName), false);
    JsValueRef global;
    IfJsrtErrorFail(ChakraRTInterface::JsGetGlobalObject(&global), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(global, wscriptName, wscript, true), false);

    JsPropertyIdRef printName;
    IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(_u("print"), &printName), false);
    IfJsrtErrorFail(ChakraRTInterface::JsSetProperty(global, printName, echo, true), false);

    return true;
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
            LPCWSTR errorMessage = nullptr;
            size_t errorMessageLength = 0;
            
            JsValueRef errorString = JS_INVALID_REFERENCE;

            IfJsrtErrorFail(ChakraRTInterface::JsConvertValueToString(exception, &errorString), false);
            IfJsrtErrorFail(ChakraRTInterface::JsStringToPointer(errorString, &errorMessage, &errorMessageLength), false);
            
            if (jsErrorCode == JsErrorCode::JsErrorScriptCompile)
            {
                JsPropertyIdRef linePropertyId = JS_INVALID_REFERENCE;
                JsValueRef lineProperty = JS_INVALID_REFERENCE;

                JsPropertyIdRef columnPropertyId = JS_INVALID_REFERENCE;
                JsValueRef columnProperty = JS_INVALID_REFERENCE;
                
                int line;
                int column;
                
                IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(_u("line"), &linePropertyId), false);
                IfJsrtErrorFail(ChakraRTInterface::JsGetProperty(exception, linePropertyId, &lineProperty), false);
                IfJsrtErrorFail(ChakraRTInterface::JsNumberToInt(lineProperty, &line), false);

                IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(_u("column"), &columnPropertyId), false);
                IfJsrtErrorFail(ChakraRTInterface::JsGetProperty(exception, columnPropertyId, &columnProperty), false);
                IfJsrtErrorFail(ChakraRTInterface::JsNumberToInt(columnProperty, &column), false);

                CHAR shortFileName[_MAX_PATH];
                CHAR ext[_MAX_EXT];
                _splitpath_s(fileName, nullptr, 0, nullptr, 0, shortFileName, _countof(shortFileName), ext, _countof(ext));
                fwprintf(stderr, _u("%ls\n\tat code (%S%S:%d:%d)\n"), errorMessage, shortFileName, ext, (int)line + 1, (int)column + 1);
            }
            else
            {
                JsValueType propertyType = JsUndefined;
                JsPropertyIdRef stackPropertyId = JS_INVALID_REFERENCE;
                JsValueRef stackProperty = JS_INVALID_REFERENCE;
                LPCWSTR errorStack = nullptr;
                size_t errorStackLength = 0;

                IfJsrtErrorFail(ChakraRTInterface::JsGetPropertyIdFromName(_u("stack"), &stackPropertyId), false);
                IfJsrtErrorFail(ChakraRTInterface::JsGetProperty(exception, stackPropertyId, &stackProperty), false);

                IfJsrtErrorFail(ChakraRTInterface::JsGetValueType(stackProperty, &propertyType), false);

                if (propertyType == JsUndefined)
                {
                    fwprintf(stderr, _u("%ls\n"), errorMessage);
                }
                else
                {
                    IfJsrtErrorFail(ChakraRTInterface::JsStringToPointer(stackProperty, &errorStack, &errorStackLength), false);
                    fwprintf(stderr, _u("%ls\n"), errorStack);
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
    ChakraRTInterface::JsRelease(m_function, nullptr);
    m_function = JS_INVALID_REFERENCE;
}

HRESULT WScriptJsrt::CallbackMessage::Call(LPCSTR fileName)
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
        LPCWSTR script = nullptr;
        size_t length = 0;

        IfJsrtErrorHR(ChakraRTInterface::JsConvertValueToString(m_function, &stringValue));
        IfJsrtErrorHR(ChakraRTInterface::JsStringToPointer(stringValue, &script, &length));

        // Run the code
        errorCode = ChakraRTInterface::JsRunScript(script, JS_SOURCE_CONTEXT_NONE, _u("") /*sourceUrl*/, nullptr /*no result needed*/);
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
