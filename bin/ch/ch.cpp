//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "Core/AtomLockGuids.h"

unsigned int MessageBase::s_messageCount = 0;
Debugger* Debugger::debugger = nullptr;

#ifdef _WIN32
LPCWSTR hostName = _u("ch.exe");
#else
LPCWSTR hostName = _u("ch");
#endif

JsRuntimeHandle chRuntime = JS_INVALID_RUNTIME_HANDLE;

BOOL doTTRecord = false;
BOOL doTTDebug = false;
byte ttUri[MAX_PATH * sizeof(wchar_t)];
size_t ttUriByteLength = 0;
UINT32 snapInterval = MAXUINT32;
UINT32 snapHistoryLength = MAXUINT32;
UINT32 startEventCount = 1;

extern "C"
HRESULT __stdcall OnChakraCoreLoadedEntry(TestHooks& testHooks)
{
    return ChakraRTInterface::OnChakraCoreLoaded(testHooks);
}

JsRuntimeAttributes jsrtAttributes = JsRuntimeAttributeAllowScriptInterrupt;

int HostExceptionFilter(int exceptionCode, _EXCEPTION_POINTERS *ep)
{
    ChakraRTInterface::NotifyUnhandledException(ep);

    bool crashOnException = false;
    ChakraRTInterface::GetCrashOnExceptionFlag(&crashOnException);

    if (exceptionCode == EXCEPTION_BREAKPOINT || (crashOnException && exceptionCode != 0xE06D7363))
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    fwprintf(stderr, _u("FATAL ERROR: %ls failed due to exception code %x\n"), hostName, exceptionCode);

    _flushall();

    // Exception happened, so we probably didn't clean up properly,
    // Don't exit normally, just terminate
    TerminateProcess(::GetCurrentProcess(), exceptionCode);

    return EXCEPTION_CONTINUE_SEARCH;
}

void __stdcall PrintUsageFormat()
{
    wprintf(_u("\nUsage: %s [flaglist] <source file>\n"), hostName);
}

void __stdcall PrintUsage()
{
#ifndef DEBUG
    wprintf(_u("\nUsage: %s <source file> %s"), hostName,
            _u("\n[flaglist] is not supported for Release mode\n"));
#else
    PrintUsageFormat();
    wprintf(_u("Try '%s -?' for help\n"), hostName);
#endif
}

// On success the param byteCodeBuffer will be allocated in the function.
// The caller of this function should de-allocate the memory.
HRESULT GetSerializedBuffer(LPCSTR fileContents, __out BYTE **byteCodeBuffer, __out DWORD *byteCodeBufferSize)
{
    HRESULT hr = S_OK;
    *byteCodeBuffer = nullptr;
    *byteCodeBufferSize = 0;
    BYTE *bcBuffer = nullptr;

    unsigned int bcBufferSize = 0;
    unsigned int newBcBufferSize = 0;
    IfJsErrorFailLog(ChakraRTInterface::JsSerializeScriptUtf8(fileContents, bcBuffer, &bcBufferSize));
    // Above call will return the size of the buffer only, once succeed we need to allocate memory of that much and call it again.
    if (bcBufferSize == 0)
    {
        AssertMsg(false, "bufferSize should not be zero");
        IfFailGo(E_FAIL);
    }
    bcBuffer = new BYTE[bcBufferSize];
    newBcBufferSize = bcBufferSize;
    IfJsErrorFailLog(ChakraRTInterface::JsSerializeScriptUtf8(fileContents, bcBuffer, &newBcBufferSize));
    Assert(bcBufferSize == newBcBufferSize);

Error:
    if (hr != S_OK)
    {
        // In the failure release the buffer
        if (bcBuffer != nullptr)
        {
            delete[] bcBuffer;
        }
    }
    else
    {
        *byteCodeBuffer = bcBuffer;
        *byteCodeBufferSize = bcBufferSize;
    }

    return hr;
}

HRESULT CreateLibraryByteCodeHeader(LPCSTR contentsRaw, DWORD lengthBytes, LPCWSTR bcFullPath, LPCSTR libraryNameNarrow)
{
    HANDLE bcFileHandle = nullptr;
    BYTE *bcBuffer = nullptr;
    DWORD bcBufferSize = 0;
    HRESULT hr = GetSerializedBuffer(contentsRaw, &bcBuffer, &bcBufferSize);

    if (FAILED(hr)) return hr;

    bcFileHandle = CreateFile(bcFullPath, GENERIC_WRITE, FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (bcFileHandle == INVALID_HANDLE_VALUE)
    {
        return E_FAIL;
    }

    DWORD written;

    // For validating the header file against the library file
    auto outputStr =
        "//-------------------------------------------------------------------------------------------------------\r\n"
        "// Copyright (C) Microsoft. All rights reserved.\r\n"
        "// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.\r\n"
        "//-------------------------------------------------------------------------------------------------------\r\n"
        "#if 0\r\n";
    IfFalseGo(WriteFile(bcFileHandle, outputStr, (DWORD)strlen(outputStr), &written, nullptr));
    IfFalseGo(WriteFile(bcFileHandle, contentsRaw, lengthBytes, &written, nullptr));
    if (lengthBytes < 2 || contentsRaw[lengthBytes - 2] != '\r' || contentsRaw[lengthBytes - 1] != '\n')
    {
        outputStr = "\r\n#endif\r\n";
    }
    else
    {
        outputStr = "#endif\r\n";
    }
    IfFalseGo(WriteFile(bcFileHandle, outputStr, (DWORD)strlen(outputStr), &written, nullptr));

    // Write out the bytecode
    outputStr = "namespace Js\r\n{\r\n    const char Library_Bytecode_";
    IfFalseGo(WriteFile(bcFileHandle, outputStr, (DWORD)strlen(outputStr), &written, nullptr));
    IfFalseGo(WriteFile(bcFileHandle, libraryNameNarrow, (DWORD)strlen(libraryNameNarrow), &written, nullptr));
    outputStr = "[] = {\r\n/* 00000000 */";
    IfFalseGo(WriteFile(bcFileHandle, outputStr, (DWORD)strlen(outputStr), &written, nullptr));

    for (unsigned int i = 0; i < bcBufferSize; i++)
    {
        char scratch[6];
        auto scratchLen = sizeof(scratch);
        int num = _snprintf_s(scratch, scratchLen, _countof(scratch), " 0x%02X", bcBuffer[i]);
        Assert(num == 5);
        IfFalseGo(WriteFile(bcFileHandle, scratch, (DWORD)(scratchLen - 1), &written, nullptr));

        // Add a comma and a space if this is not the last item
        if (i < bcBufferSize - 1)
        {
            char commaSpace[2];
            _snprintf_s(commaSpace, sizeof(commaSpace), _countof(commaSpace), ",");  // close quote, new line, offset and open quote
            IfFalseGo(WriteFile(bcFileHandle, commaSpace, (DWORD)strlen(commaSpace), &written, nullptr));
        }

        // Add a line break every 16 scratches, primarily so the compiler doesn't complain about the string being too long.
        // Also, won't add for the last scratch
        if (i % 16 == 15 && i < bcBufferSize - 1)
        {
            char offset[17];
            _snprintf_s(offset, sizeof(offset), _countof(offset), "\r\n/* %08X */", i + 1);  // close quote, new line, offset and open quote
            IfFalseGo(WriteFile(bcFileHandle, offset, (DWORD)strlen(offset), &written, nullptr));
        }
    }
    outputStr = "};\r\n\r\n";
    IfFalseGo(WriteFile(bcFileHandle, outputStr, (DWORD)strlen(outputStr), &written, nullptr));

    outputStr = "}\r\n";
    IfFalseGo(WriteFile(bcFileHandle, outputStr, (DWORD)strlen(outputStr), &written, nullptr));

Error:
    if (bcFileHandle != nullptr)
    {
        CloseHandle(bcFileHandle);
    }
    if (bcBuffer != nullptr)
    {
        delete[] bcBuffer;
    }

    return hr;
}

static void CALLBACK PromiseContinuationCallback(JsValueRef task, void *callbackState)
{
    Assert(task != JS_INVALID_REFERENCE);
    Assert(callbackState != JS_INVALID_REFERENCE);
    MessageQueue * messageQueue = (MessageQueue *)callbackState;

    WScriptJsrt::CallbackMessage *msg = new WScriptJsrt::CallbackMessage(0, task);
    messageQueue->InsertSorted(msg);
}

static bool CHAKRA_CALLBACK DummyJsSerializedScriptLoadUtf8Source(_In_ JsSourceContext sourceContext, _Outptr_result_z_ const char** scriptBuffer)
{
    // sourceContext is source ptr, see RunScript below
    *scriptBuffer = reinterpret_cast<const char*>(sourceContext);
    return true;
}

static void CHAKRA_CALLBACK DummyJsSerializedScriptUnload(_In_ JsSourceContext sourceContext)
{
    // sourceContext is source ptr, see RunScript below
    // source memory was originally allocated with malloc() in
    // Helpers::LoadScriptFromFile. No longer needed, free() it.
    free(reinterpret_cast<void*>(sourceContext));
}

HRESULT RunScript(const char* fileName, LPCSTR fileContents, BYTE *bcBuffer, char *fullPath)
{
    HRESULT hr = S_OK;
    MessageQueue * messageQueue = new MessageQueue();
    WScriptJsrt::AddMessageQueue(messageQueue);

    IfJsErrorFailLog(ChakraRTInterface::JsSetPromiseContinuationCallback(PromiseContinuationCallback, (void*)messageQueue));

    if(strlen(fileName) >= 14 && strcmp(fileName + strlen(fileName) - 14, "ttdSentinal.js") == 0)
    {
#if !ENABLE_TTD
        wprintf(_u("Sential js file is only ok when in TTDebug mode!!!\n"));
        return E_FAIL;
#else
        if(!doTTDebug)
        {
            wprintf(_u("Sential js file is only ok when in TTDebug mode!!!\n"));
            return E_FAIL;
        }

        ChakraRTInterface::JsTTDStartTimeTravelDebugging();

        try
        {
            JsTTDMoveMode moveMode = (JsTTDMoveMode)(JsTTDMoveMode::JsTTDMoveKthEvent | ((int64) startEventCount) << 32);
            INT64 snapEventTime = -1;
            INT64 nextEventTime = -2;

            while(true)
            {
                bool needFreshCtxs = false;
                JsErrorCode error = ChakraRTInterface::JsTTDGetSnapTimeTopLevelEventMove(chRuntime, moveMode, &nextEventTime, &needFreshCtxs, &snapEventTime, nullptr);

                if(error != JsNoError)
                {
                    if(error == JsErrorCategoryUsage)
                    {
                        wprintf(_u("Start time not in log range.\n"));
                    }

                    return error;
                }

                IfFailedReturn(ChakraRTInterface::JsTTDPrepContextsForTopLevelEventMove(chRuntime, needFreshCtxs));
                IfFailedReturn(ChakraRTInterface::JsTTDMoveToTopLevelEvent(moveMode, snapEventTime, nextEventTime));

                JsErrorCode res = ChakraRTInterface::JsTTDReplayExecution(&moveMode, &nextEventTime);

                //handle any uncaught exception by immediately time-traveling to the throwing line in the debugger -- in replay just report and exit
                if(res == JsErrorCategoryScript)
                {
                    wprintf(_u("An unhandled script exception occoured!!!\n"));

                    ExitProcess(0);
                }

                if(nextEventTime == -1)
                {
                    wprintf(_u("\nReached end of Execution -- Exiting.\n"));
                    break;
                }
            }
        }
        catch(...)
        {
            wprintf(_u("Terminal exception in Replay -- exiting.\n"));
            ExitProcess(0);
        }
#endif
    }
    else
    {
        Assert(fileContents != nullptr || bcBuffer != nullptr);

        JsErrorCode runScript;
        if(bcBuffer != nullptr)
        {
            runScript = ChakraRTInterface::JsRunSerializedScriptUtf8(
                DummyJsSerializedScriptLoadUtf8Source, DummyJsSerializedScriptUnload,
                bcBuffer,
                reinterpret_cast<JsSourceContext>(fileContents),
                // Use source ptr as sourceContext
                fullPath, nullptr /*result*/);
        }
        else
        {
#if ENABLE_TTD
            if(doTTRecord)
            {
                ChakraRTInterface::JsTTDStartTimeTravelRecording();
            }

            runScript = ChakraRTInterface::JsRunScriptUtf8(fileContents, WScriptJsrt::GetNextSourceContext(), fullPath, nullptr /*result*/);
            if (runScript == JsErrorCategoryUsage)
            {
                wprintf(_u("FATAL ERROR: Core was compiled without ENABLE_TTD is defined. CH is trying to use TTD interface\n"));
                abort();
            }
#else
            runScript = ChakraRTInterface::JsRunScriptUtf8(fileContents, WScriptJsrt::GetNextSourceContext(), fullPath, nullptr /*result*/);
#endif
        }

        //Do a yield after the main script body executes
        ChakraRTInterface::JsTTDNotifyYield();

        if(runScript != JsNoError)
        {
            WScriptJsrt::PrintException(fileName, runScript);
        }
        else
        {
            // Repeatedly flush the message queue until it's empty. It is necessary to loop on this
            // because setTimeout can add scripts to execute.
            do
            {
                IfFailGo(messageQueue->ProcessAll(fileName));
            } while(!messageQueue->IsEmpty());
        }
    }

Error:
#if ENABLE_TTD
    if(doTTRecord)
    {
        ChakraRTInterface::JsTTDStopTimeTravelRecording();
    }
#endif

    if (messageQueue != nullptr)
    {
        messageQueue->RemoveAll();
        // clean up possible pinned exception object on exit to avoid potential leak
        bool hasException;
        if (ChakraRTInterface::JsHasException(&hasException) == JsNoError && hasException)
        {
            JsValueRef exception = JS_INVALID_REFERENCE;
            ChakraRTInterface::JsGetAndClearException(&exception);
        }
        delete messageQueue;
    }
    return hr;
}

static HRESULT CreateRuntime(JsRuntimeHandle *runtime)
{
    HRESULT hr = E_FAIL;
    IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(jsrtAttributes, nullptr, runtime));

#ifndef _WIN32
    // On Posix, malloc may not return NULL even if there is no
    // memory left. However, kernel will send SIGKILL to process
    // in case we use that `not actually available` memory address.
    // (See posix man malloc and OOM)

    size_t memoryLimit;
    if (PlatformAgnostic::SystemInfo::GetTotalRam(&memoryLimit))
    {
        IfJsErrorFailLog(ChakraRTInterface::JsSetRuntimeMemoryLimit(*runtime, memoryLimit));
    }
#endif
    hr = S_OK;
Error:
    return hr;
}

HRESULT CreateAndRunSerializedScript(const char* fileName, LPCSTR fileContents, char *fullPath)
{
    HRESULT hr = S_OK;
    JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
    JsContextRef context = JS_INVALID_REFERENCE, current = JS_INVALID_REFERENCE;
    BYTE *bcBuffer = nullptr;
    DWORD bcBufferSize = 0;
    WScriptJsrt::ContextData* contextData = nullptr;
    char fullPathLower[_MAX_PATH];
    size_t len;
    IfFailGo(GetSerializedBuffer(fileContents, &bcBuffer, &bcBufferSize));

    // Bytecode buffer is created in one runtime and will be executed on different runtime.

    IfFailGo(CreateRuntime(&runtime));
    chRuntime = runtime;

    IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
    IfJsErrorFailLog(ChakraRTInterface::JsGetCurrentContext(&current));
    IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));

    // canonicalize that path name to lower case for the profile storage
    // REVIEW: Assuming no utf8 characters here
    len = strlen(fullPath) + 1;
    for (size_t i = 0; i < len; i++)
    {
        fullPathLower[i] = (char)tolower(fullPath[i]);
    }
    contextData = new WScriptJsrt::ContextData(fullPath);
    IfJsErrorFailLog(ChakraRTInterface::JsSetContextData(context, (void*)contextData));

    // Initialized the WScript object on the new context
    if (!WScriptJsrt::Initialize())
    {
        IfFailGo(E_FAIL);
    }

    IfFailGo(RunScript(fileName, fileContents, bcBuffer, fullPathLower));

Error:
    if (bcBuffer != nullptr)
    {
        delete[] bcBuffer;
    }

    if (current != JS_INVALID_REFERENCE)
    {
        ChakraRTInterface::JsSetCurrentContext(current);
    }

    if (runtime != JS_INVALID_RUNTIME_HANDLE)
    {
        ChakraRTInterface::JsDisposeRuntime(runtime);
    }
    return hr;
}

HRESULT ExecuteTest(const char* fileName)
{
    HRESULT hr = S_OK;
    LPCSTR fileContents = nullptr;
    JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
    UINT lengthBytes = 0;

    if(strlen(fileName) >= 14 && strcmp(fileName + strlen(fileName) - 14, "ttdSentinal.js") == 0)
    {
#if !ENABLE_TTD
        wprintf(_u("Sentinel js file is only ok when in TTDebug mode!!!\n"));
        return E_FAIL;
#else
        if(!doTTDebug)
        {
            wprintf(_u("Sentinel js file is only ok when in TTDebug mode!!!\n"));
            return E_FAIL;
        }

        jsrtAttributes = static_cast<JsRuntimeAttributes>(jsrtAttributes | JsRuntimeAttributeEnableExperimentalFeatures);

        IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateDebugRuntime(jsrtAttributes, ttUri, ttUriByteLength, nullptr, &runtime));
        chRuntime = runtime;

        ChakraRTInterface::JsTTDSetIOCallbacks(runtime, &Helpers::TTInitializeForWriteLogStreamCallback, &Helpers::TTCreateStreamCallback, &Helpers::TTReadBytesFromStreamCallback, &Helpers::TTWriteBytesToStreamCallback, &Helpers::TTFlushAndCloseStreamCallback);

        JsContextRef context = JS_INVALID_REFERENCE;
        IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
        IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));

        IfFailGo(RunScript(fileName, fileContents, nullptr, nullptr));
#endif
    }
    else
    {
        LPCOLESTR contentsRaw = nullptr;

        char fullPathSrc[_MAX_PATH];
        char fullPath[_MAX_PATH];
        size_t len = 0;

        hr = Helpers::LoadScriptFromFile(fileName, fileContents, &lengthBytes);
        contentsRaw; lengthBytes; // Unused for now.

        IfFailGo(hr);
        if (HostConfigFlags::flags.GenerateLibraryByteCodeHeaderIsEnabled)
        {
            jsrtAttributes = (JsRuntimeAttributes)(jsrtAttributes | JsRuntimeAttributeSerializeLibraryByteCode);
        }

        JsContextRef context = JS_INVALID_REFERENCE;
#if ENABLE_TTD
        if (doTTRecord)
        {
            //Ensure we run with experimental features (as that is what Node does right now).
            jsrtAttributes = static_cast<JsRuntimeAttributes>(jsrtAttributes | JsRuntimeAttributeEnableExperimentalFeatures);

            IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateRecordRuntime(jsrtAttributes, ttUri, ttUriByteLength, snapInterval, snapHistoryLength, nullptr, &runtime));
            chRuntime = runtime;

            ChakraRTInterface::JsTTDSetIOCallbacks(runtime, &Helpers::TTInitializeForWriteLogStreamCallback, &Helpers::TTCreateStreamCallback, &Helpers::TTReadBytesFromStreamCallback, &Helpers::TTWriteBytesToStreamCallback, &Helpers::TTFlushAndCloseStreamCallback);

            IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateContext(runtime, &context));
            IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));
        }
        else
        {
            AssertMsg(!doTTDebug, "Should be handled in the else case above!!!");

            IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(jsrtAttributes, nullptr, &runtime));
            chRuntime = runtime;

            if (HostConfigFlags::flags.DebugLaunch)
            {
                Debugger* debugger = Debugger::GetDebugger(runtime);
                debugger->StartDebugging(runtime);
            }

            IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
            IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));
        }
#else
        IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(jsrtAttributes, nullptr, &runtime));
        chRuntime = runtime;

        if (HostConfigFlags::flags.DebugLaunch)
        {
            Debugger* debugger = Debugger::GetDebugger(runtime);
            debugger->StartDebugging(runtime);
        }

        IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
        IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));
#endif

#ifdef DEBUG
        ChakraRTInterface::SetCheckOpHelpersFlag(true);
#endif

        if (!WScriptJsrt::Initialize())
        {
            IfFailGo(E_FAIL);
        }

        if (_fullpath(fullPathSrc, fileName, _MAX_PATH) == nullptr)
        {
            IfFailGo(E_FAIL);
        }

        // canonicalize that path name to lower case for the profile storage
        // REVIEW: Assuming no utf8 characters here
        len = strlen(fullPathSrc) + 1;
        for (size_t i = 0; i < len; i++)
        {
            fullPath[i] = (char)tolower(fullPathSrc[i]);
        }

        WScriptJsrt::ContextData* contextData = new WScriptJsrt::ContextData(fullPathSrc);
        IfJsErrorFailLog(ChakraRTInterface::JsSetContextData(context, (void*)contextData));

        if (HostConfigFlags::flags.GenerateLibraryByteCodeHeaderIsEnabled)
        {
            if (HostConfigFlags::flags.GenerateLibraryByteCodeHeader != nullptr && *HostConfigFlags::flags.GenerateLibraryByteCodeHeader != _u('\0'))
            {
                CHAR libraryName[_MAX_PATH];
                CHAR ext[_MAX_EXT];
                _splitpath_s(fullPath, NULL, 0, NULL, 0, libraryName, _countof(libraryName), ext, _countof(ext));

                IfFailGo(CreateLibraryByteCodeHeader(fileContents, lengthBytes, HostConfigFlags::flags.GenerateLibraryByteCodeHeader, libraryName));
            }
            else
            {
                fwprintf(stderr, _u("FATAL ERROR: -GenerateLibraryByteCodeHeader must provide the file name, i.e., -GenerateLibraryByteCodeHeader:<bytecode file name>, exiting\n"));
                IfFailGo(E_FAIL);
            }
        }
        else if (HostConfigFlags::flags.SerializedIsEnabled)
        {
            CreateAndRunSerializedScript(fileName, fileContents, fullPathSrc);
        }
        else
        {
            IfFailGo(RunScript(fileName, fileContents, nullptr, fullPath));
        }
    }
Error:
    if (Debugger::debugger != nullptr)
    {
        Debugger::debugger->CompareOrWriteBaselineFile(fileName);
        Debugger::CloseDebugger();
    }

    ChakraRTInterface::JsSetCurrentContext(nullptr);

    if (runtime != JS_INVALID_RUNTIME_HANDLE)
    {
        ChakraRTInterface::JsDisposeRuntime(runtime);
    }

    _flushall();

    return hr;
}

HRESULT ExecuteTestWithMemoryCheck(char* fileName)
{
    HRESULT hr = E_FAIL;
#ifdef CHECK_MEMORY_LEAK
    // Always check memory leak, unless user specified the flag already
    if (!ChakraRTInterface::IsEnabledCheckMemoryFlag())
    {
        ChakraRTInterface::SetCheckMemoryLeakFlag(true);
    }

    // Disable the output in case an unhandled exception happens
    // We will re-enable it if there is no unhandled exceptions
    ChakraRTInterface::SetEnableCheckMemoryLeakOutput(false);
#endif

#ifdef _WIN32
    __try
    {
        hr = ExecuteTest(fileName);
    }
    __except (HostExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
    {
        Assert(false);
    }
#else
    // REVIEW: Do we need a SEH handler here?
    hr = ExecuteTest(fileName);
    if (FAILED(hr)) exit(0);
#endif // _WIN32

    _flushall();
#ifdef CHECK_MEMORY_LEAK
    ChakraRTInterface::SetEnableCheckMemoryLeakOutput(true);
#endif
    return hr;
}


unsigned int WINAPI StaticThreadProc(void *lpParam)
{
    ChakraRTInterface::ArgInfo* argInfo = static_cast<ChakraRTInterface::ArgInfo* >(lpParam);
    return ExecuteTestWithMemoryCheck(argInfo->filename);
}

#ifndef _WIN32
static char16** argv = nullptr;
int main(int argc, char** c_argv)
{
    PAL_InitializeChakraCore(argc, c_argv);
    argv = new char16*[argc];
    for (int i = 0; i < argc; i++)
    {
        NarrowStringToWideDynamic(c_argv[i], &argv[i]);
    }
#else
#define PAL_Shutdown()
int _cdecl wmain(int argc, __in_ecount(argc) LPWSTR argv[])
{
#endif

    if (argc < 2)
    {
        PrintUsage();
        PAL_Shutdown();
        return EXIT_FAILURE;
    }

    int cpos = 0;
    for(int i = 0; i < argc; ++i)
    {
        if(wcsstr(argv[i], _u("-TTRecord=")) == argv[i])
        {
            doTTRecord = true;
            wchar* ruri = argv[i] + wcslen(_u("-TTRecord="));
            Helpers::GetTTDDirectory(ruri, &ttUriByteLength, ttUri);
        }
        else if(wcsstr(argv[i], _u("-TTDebug=")) == argv[i])
        {
            doTTDebug = true;
            wchar* ruri = argv[i] + wcslen(_u("-TTDebug="));
            Helpers::GetTTDDirectory(ruri, &ttUriByteLength, ttUri);
        }
        else if(wcsstr(argv[i], _u("-TTSnapInterval=")) == argv[i])
        {
            LPCWSTR intervalStr = argv[i] + wcslen(_u("-TTSnapInterval="));
            snapInterval = (UINT32)_wtoi(intervalStr);
        }
        else if(wcsstr(argv[i], _u("-TTHistoryLength=")) == argv[i])
        {
            LPCWSTR historyStr = argv[i] + wcslen(_u("-TTHistoryLength="));
            snapHistoryLength = (UINT32)_wtoi(historyStr);
        }
        else if(wcsstr(argv[i], _u("-TTDStartEvent=")) == argv[i])
        {
            LPCWSTR startEventStr = argv[i] + wcslen(_u("-TTDStartEvent="));
            startEventCount = (UINT32)_wtoi(startEventStr);
        }
        else
        {
            argv[cpos] = argv[i];
            cpos++;
        }
    }
    argc = cpos;

    if(doTTRecord & doTTDebug)
    {
        fwprintf(stderr, _u("Cannot run in record and debug at same time!!!"));
        ExitProcess(0);
    }

    HostConfigFlags::pfnPrintUsage = PrintUsageFormat;

    // The following code is present to make sure we don't load
    // jscript9.dll etc with ch. Since that isn't a concern on non-Windows
    // builds, it's safe to conditionally compile it out.
#ifdef _WIN32
    ATOM lock = ::AddAtom(szChakraCoreLock);
    AssertMsg(lock, "failed to lock chakracore.dll");
#endif // _WIN32

    HostConfigFlags::HandleArgsFlag(argc, argv);

    ChakraRTInterface::ArgInfo argInfo = { argc, argv, PrintUsage, nullptr };
    HINSTANCE chakraLibrary = nullptr;
    bool success = ChakraRTInterface::LoadChakraDll(&argInfo, &chakraLibrary);

#if defined(CHAKRA_STATIC_LIBRARY) && !defined(NDEBUG)
    // handle command line flags
    OnChakraCoreLoaded();
#endif

    if (argInfo.filename == nullptr)
    {
        WideStringToNarrowDynamic(argv[1], &argInfo.filename);
    }

    if (success)
    {
#ifdef _WIN32
        HANDLE threadHandle;
        threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, &StaticThreadProc, &argInfo, STACK_SIZE_PARAM_IS_A_RESERVATION, 0));

        if (threadHandle != nullptr)
        {
            DWORD waitResult = WaitForSingleObject(threadHandle, INFINITE);
            Assert(waitResult == WAIT_OBJECT_0);
            CloseHandle(threadHandle);
        }
        else
        {
            fwprintf(stderr, _u("FATAL ERROR: failed to create worker thread error code %d, exiting\n"), errno);
            AssertMsg(false, "failed to create worker thread");
        }
#else
        // On linux, execute on the same thread
        ExecuteTestWithMemoryCheck(argInfo.filename);
#endif
        ChakraRTInterface::UnloadChakraDll(chakraLibrary);
    }

    PAL_Shutdown();
    return 0;
}
