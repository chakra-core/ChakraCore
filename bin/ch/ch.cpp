//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "Core/AtomLockGuids.h"
#include <CommonPal.h>
#ifdef _WIN32
#include <winver.h>
#include <process.h>
#endif

unsigned int MessageBase::s_messageCount = 0;
Debugger* Debugger::debugger = nullptr;

#ifdef _WIN32
LPCWSTR hostName = _u("ch.exe");
#else
LPCWSTR hostName = _u("ch");
#endif

JsRuntimeHandle chRuntime = JS_INVALID_RUNTIME_HANDLE;

BOOL doTTRecord = false;
BOOL doTTReplay = false;
const size_t ttUriBufferLength = MAX_PATH * 3;
char ttUri[ttUriBufferLength];
size_t ttUriLength = 0;
UINT32 snapInterval = MAXUINT32;
UINT32 snapHistoryLength = MAXUINT32;
LPCWSTR connectionUuidString = NULL;
UINT32 startEventCount = 1;

extern "C"
HRESULT __stdcall OnChakraCoreLoadedEntry(TestHooks& testHooks)
{
    return ChakraRTInterface::OnChakraCoreLoaded(testHooks);
}

JsRuntimeAttributes jsrtAttributes = JsRuntimeAttributeNone;

int HostExceptionFilter(int exceptionCode, _EXCEPTION_POINTERS *ep)
{
    ChakraRTInterface::NotifyUnhandledException(ep);

#if ENABLE_NATIVE_CODEGEN && _WIN32
    JITProcessManager::TerminateJITServer();
#endif
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
    wprintf(_u("\nUsage: %s [-v|-version] [-h|-help] [-?] [flaglist] <source file>\n"), hostName);
    wprintf(_u("\t-v|-version\t\tDisplays version info\n"));
    wprintf(_u("\t-h|-help\t\tDisplays this help message\n"));
    wprintf(_u("\t-?\t\t\tDisplays this help message with complete [flaglist] info\n"));
}

#if !defined(ENABLE_DEBUG_CONFIG_OPTIONS)
void __stdcall PrintReleaseUsage()
{
    wprintf(_u("\nUsage: %s [-v|-version] [-h|-help|-?] <source file> %s"), hostName,
        _u("\nNote: [flaglist] is not supported in Release builds; try a Debug or Test build to enable these flags.\n"));
    wprintf(_u("\t-v|-version\t\tDisplays version info\n"));
    wprintf(_u("\t-h|-help|-?\t\tDisplays this help message\n"));
}
#endif

void __stdcall PrintUsage()
{
#if !defined(ENABLE_DEBUG_CONFIG_OPTIONS)
    PrintReleaseUsage();
#else
    PrintUsageFormat();
#endif
}

void __stdcall PrintChVersion()
{
#if CHAKRA_CORE_VERSION_RELEASE
    wprintf(_u("%s version %d.%d.%d.0\n"),
#else
    wprintf(_u("%s version %d.%d.%d.0-beta\n"),
#endif
        hostName, CHAKRA_CORE_MAJOR_VERSION, CHAKRA_CORE_MINOR_VERSION, CHAKRA_CORE_PATCH_VERSION);
}

#ifdef _WIN32
void __stdcall PrintChakraCoreVersion()
{
    char filename[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];

    LPCSTR chakraDllName = GetChakraDllName();

    char modulename[_MAX_PATH];
    GetModuleFileNameA(NULL, modulename, _MAX_PATH);
    _splitpath_s(modulename, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
    _makepath_s(filename, drive, dir, chakraDllName, nullptr);

    UINT size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD verSize = GetFileVersionInfoSizeA(filename, NULL);

    if (verSize != NULL)
    {
        LPSTR verData = new char[verSize];

        if (GetFileVersionInfoA(filename, NULL, verSize, verData) &&
            VerQueryValue(verData, _u("\\"), (VOID FAR * FAR *)&lpBuffer, &size) &&
            (size != 0))
        {
            VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
            if (verInfo->dwSignature == VS_FFI_SIGNATURE)
            {
                // Doesn't matter if you are on 32 bit or 64 bit,
                // DWORD is always 32 bits, so first two revision numbers
                // come from dwFileVersionMS, last two come from dwFileVersionLS
                printf("%s version %d.%d.%d.%d\n",
                    chakraDllName,
                    (verInfo->dwFileVersionMS >> 16) & 0xffff,
                    (verInfo->dwFileVersionMS >> 0) & 0xffff,
                    (verInfo->dwFileVersionLS >> 16) & 0xffff,
                    (verInfo->dwFileVersionLS >> 0) & 0xffff);
            }
        }

        delete[] verData;
    }
}
#endif

void __stdcall PrintVersion()
{
    PrintChVersion();

#ifdef _WIN32
    PrintChakraCoreVersion();
#endif
}

// On success the param byteCodeBuffer will be allocated in the function.
HRESULT GetSerializedBuffer(LPCSTR fileContents, JsFinalizeCallback fileContentFinalizeCallback, JsValueRef *byteCodeBuffer)
{
    HRESULT hr = S_OK;

    JsValueRef scriptSource;
    IfJsErrorFailLog(ChakraRTInterface::JsCreateExternalArrayBuffer((void*)fileContents,
        (unsigned int)strlen(fileContents), fileContentFinalizeCallback, (void*)fileContents, &scriptSource));
    IfJsErrorFailLog(ChakraRTInterface::JsSerialize(scriptSource, byteCodeBuffer,
        JsParseScriptAttributeNone));

Error:
    return hr;
}

HRESULT CreateLibraryByteCodeHeader(LPCSTR contentsRaw, JsFinalizeCallback contentsRawFinalizeCallback, DWORD lengthBytes, LPCWSTR bcFullPath, LPCSTR libraryNameNarrow)
{
    HANDLE bcFileHandle = nullptr;
    JsValueRef bufferVal;
    BYTE *bcBuffer = nullptr;
    unsigned int bcBufferSize = 0;
    DWORD written;
    // For validating the header file against the library file
    auto outputStr =
        "//-------------------------------------------------------------------------------------------------------\r\n"
        "// Copyright (C) Microsoft. All rights reserved.\r\n"
        "// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.\r\n"
        "//-------------------------------------------------------------------------------------------------------\r\n"
        "#if 0\r\n";


    // We still need contentsRaw after this, so pass a null finalizeCallback into it
    HRESULT hr = GetSerializedBuffer(contentsRaw, nullptr, &bufferVal);

    IfFailedGoLabel((hr), ErrorRunFinalize);

    IfJsrtErrorHRLabel(ChakraRTInterface::JsGetArrayBufferStorage(bufferVal, &bcBuffer, &bcBufferSize), ErrorRunFinalize);

    bcFileHandle = CreateFile(bcFullPath, GENERIC_WRITE, FILE_SHARE_DELETE,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (bcFileHandle == INVALID_HANDLE_VALUE)
    {
        IfFailedGoLabel(E_FAIL, ErrorRunFinalize);
    }

    IfFalseGoLabel(WriteFile(bcFileHandle, outputStr, (DWORD)strlen(outputStr), &written, nullptr), ErrorRunFinalize);
    IfFalseGoLabel(WriteFile(bcFileHandle, contentsRaw, lengthBytes, &written, nullptr), ErrorRunFinalize);
    if (lengthBytes < 2 || contentsRaw[lengthBytes - 2] != '\r' || contentsRaw[lengthBytes - 1] != '\n')
    {
        outputStr = "\r\n#endif\r\n";
    }
    else
    {
        outputStr = "#endif\r\n";
    }
    
    // We no longer need contentsRaw, so call the finalizer for it if one was provided
    if(contentsRawFinalizeCallback != nullptr)
    {
        contentsRawFinalizeCallback((void*)contentsRaw);
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

    if(false)
    {
ErrorRunFinalize:
        if(contentsRawFinalizeCallback != nullptr)
        {
            contentsRawFinalizeCallback((void*)contentsRaw);
        }
    }
Error:
    if (bcFileHandle != nullptr)
    {
        CloseHandle(bcFileHandle);
    }

    return hr;
}

// Used becuase of what's likely a deficiency in the API
typedef struct {
    void* scriptBody;
    JsFinalizeCallback scriptBodyFinalizeCallback;
    bool freeingHandled;
} SerializedCallbackInfo;

static bool CHAKRA_CALLBACK DummyJsSerializedScriptLoadUtf8Source(
    JsSourceContext sourceContext,
    JsValueRef* scriptBuffer,
    JsParseScriptAttributes *parseAttributes)
{
    SerializedCallbackInfo* serializedCallbackInfo = reinterpret_cast<SerializedCallbackInfo*>(sourceContext);
    Assert(!serializedCallbackInfo->freeingHandled);
    serializedCallbackInfo->freeingHandled = true;
    size_t length = strlen(reinterpret_cast<const char*>(serializedCallbackInfo->scriptBody));

    // sourceContext is source ptr, see RunScript below
    if (ChakraRTInterface::JsCreateExternalArrayBuffer(serializedCallbackInfo->scriptBody, (unsigned int)length,
        serializedCallbackInfo->scriptBodyFinalizeCallback, serializedCallbackInfo->scriptBody, scriptBuffer) != JsNoError)
    {
        return false;
    }

    *parseAttributes = JsParseScriptAttributeNone;
    return true;
}

HRESULT RunScript(const char* fileName, LPCSTR fileContents, JsFinalizeCallback fileContentsFinalizeCallback, JsValueRef bufferValue, char *fullPath)
{
    HRESULT hr = S_OK;
    MessageQueue * messageQueue = new MessageQueue();
    WScriptJsrt::AddMessageQueue(messageQueue);

    IfJsErrorFailLogLabel(ChakraRTInterface::JsSetPromiseContinuationCallback(WScriptJsrt::PromiseContinuationCallback, (void*)messageQueue), ErrorRunFinalize);

    if(strlen(fileName) >= 14 && strcmp(fileName + strlen(fileName) - 14, "ttdSentinal.js") == 0)
    {
        if(fileContentsFinalizeCallback != nullptr)
        {
            fileContentsFinalizeCallback((void*)fileContents);
        }
#if !ENABLE_TTD
        wprintf(_u("Sential js file is only ok when in TTDebug mode!!!\n"));
        return E_FAIL;
#else
        if(!doTTReplay)
        {
            wprintf(_u("Sential js file is only ok when in TTReplay mode!!!\n"));
            return E_FAIL;
        }

        IfFailedReturn(ChakraRTInterface::JsTTDStart());

        try
        {
            JsTTDMoveMode moveMode = JsTTDMoveMode::JsTTDMoveKthEvent;
            int64_t snapEventTime = -1;
            int64_t nextEventTime = -2;

            while(true)
            {
                JsErrorCode error = ChakraRTInterface::JsTTDGetSnapTimeTopLevelEventMove(chRuntime, moveMode, startEventCount, &nextEventTime, &snapEventTime, nullptr);

                if(error != JsNoError)
                {
                    if(error == JsErrorCategoryUsage)
                    {
                        wprintf(_u("Start time not in log range.\n"));
                    }

                    return error;
                }

                IfFailedReturn(ChakraRTInterface::JsTTDMoveToTopLevelEvent(chRuntime, moveMode, snapEventTime, nextEventTime));

                JsErrorCode res = ChakraRTInterface::JsTTDReplayExecution(&moveMode, &nextEventTime);

                //handle any uncaught exception by immediately time-traveling to the throwing line in the debugger -- in replay just report and exit
                if(res == JsErrorCategoryScript)
                {
                    wprintf(_u("An unhandled script exception occurred!!!\n"));

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
        Assert(fileContents != nullptr || bufferValue != nullptr);

        JsErrorCode runScript;
        JsValueRef fname;
        IfJsErrorFailLogLabel(ChakraRTInterface::JsCreateString(fullPath,
            strlen(fullPath), &fname), ErrorRunFinalize);

        if(bufferValue != nullptr)
        {
            if(fileContents == nullptr)
            {
                // if we have no fileContents, no worry about freeing them, and the call is simple.
                runScript = ChakraRTInterface::JsRunSerialized(
                        bufferValue,
                        nullptr /*JsSerializedLoadScriptCallback*/,
                        0 /*SourceContext*/,
                        fname,
                        nullptr /*result*/
                        );
            }
            else // fileContents != nullptr
            {
                // Memory management is a little more complex here
                SerializedCallbackInfo serializedCallbackInfo;
                serializedCallbackInfo.scriptBody = (void*)fileContents;
                serializedCallbackInfo.scriptBodyFinalizeCallback = fileContentsFinalizeCallback;
                serializedCallbackInfo.freeingHandled = false;

                // Now we can run our script, with this serializedCallbackInfo as the sourcecontext
                runScript = ChakraRTInterface::JsRunSerialized(
                        bufferValue,
                        DummyJsSerializedScriptLoadUtf8Source,
                        reinterpret_cast<JsSourceContext>(&serializedCallbackInfo),
                        // Use source ptr as sourceContext
                        fname,
                        nullptr /*result*/);
                // Now that we're down here, we can free the fileContents if they weren't sent into
                // a GC-managed object.
                if(!serializedCallbackInfo.freeingHandled)
                {
                    if(fileContentsFinalizeCallback != nullptr)
                    {
                        fileContentsFinalizeCallback((void*)fileContents);
                    }
                }
            }
        }
        else // bufferValue == nullptr
        {
            JsValueRef scriptSource;
            IfJsErrorFailLog(ChakraRTInterface::JsCreateExternalArrayBuffer((void*)fileContents,
                (unsigned int)strlen(fileContents),
                fileContentsFinalizeCallback, (void*)fileContents, &scriptSource));
#if ENABLE_TTD
            if(doTTRecord)
            {
                JsPropertyIdRef ttProperty = nullptr;
                JsValueRef ttString = nullptr;
                JsValueRef global = nullptr;

                IfFailedReturn(ChakraRTInterface::JsCreatePropertyId("ttdLogURI", strlen("ttdLogURI"), &ttProperty));
                IfFailedReturn(ChakraRTInterface::JsCreateString(ttUri, ttUriLength, &ttString));
                IfFailedReturn(ChakraRTInterface::JsGetGlobalObject(&global));

                IfFailedReturn(ChakraRTInterface::JsSetProperty(global, ttProperty, ttString, false));

                IfFailedReturn(ChakraRTInterface::JsTTDStart());
            }

            auto sourceContext = WScriptJsrt::GetNextSourceContext();
            WScriptJsrt::RegisterScriptDir(sourceContext, fullPath);
            runScript = ChakraRTInterface::JsRun(scriptSource,
                sourceContext, fname,
                JsParseScriptAttributeNone, nullptr /*result*/);
            if (runScript == JsErrorCategoryUsage)
            {
                wprintf(_u("FATAL ERROR: Core was compiled without ENABLE_TTD is defined. CH is trying to use TTD interface\n"));
                abort();
            }
#else
            runScript = ChakraRTInterface::JsRun(scriptSource,
                WScriptJsrt::GetNextSourceContext(), fname,
                JsParseScriptAttributeNone,
                nullptr /*result*/);
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

    if(false)
    {
ErrorRunFinalize:
        if(fileContentsFinalizeCallback != nullptr)
        {
            fileContentsFinalizeCallback((void*)fileContents);
        }
    }
Error:
#if ENABLE_TTD
    if(doTTRecord)
    {
        IfFailedReturn(ChakraRTInterface::JsTTDStop());
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

    // We only call RunScript() once, safe to Uninitialize()
    WScriptJsrt::Uninitialize();

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

HRESULT CreateAndRunSerializedScript(const char* fileName, LPCSTR fileContents, JsFinalizeCallback fileContentsFinalizeCallback, char *fullPath)
{
    HRESULT hr = S_OK;
    JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
    JsContextRef context = JS_INVALID_REFERENCE, current = JS_INVALID_REFERENCE;
    JsValueRef bufferVal;

    // We don't want this to free fileContents when it completes, so the finalizeCallback is nullptr
    IfFailedGoLabel(GetSerializedBuffer(fileContents, nullptr, &bufferVal), ErrorRunFinalize);

    // Bytecode buffer is created in one runtime and will be executed on different runtime.

    IfFailedGoLabel(CreateRuntime(&runtime), ErrorRunFinalize);
    chRuntime = runtime;

    IfJsErrorFailLogLabel(ChakraRTInterface::JsCreateContext(runtime, &context), ErrorRunFinalize);
    IfJsErrorFailLogLabel(ChakraRTInterface::JsGetCurrentContext(&current), ErrorRunFinalize);
    IfJsErrorFailLogLabel(ChakraRTInterface::JsSetCurrentContext(context), ErrorRunFinalize);

    // Initialized the WScript object on the new context
    if (!WScriptJsrt::Initialize())
    {
        IfFailedGoLabel(E_FAIL, ErrorRunFinalize);
    }

    // This is our last call to use fileContents, so pass in the finalizeCallback
    IfFailGo(RunScript(fileName, fileContents, fileContentsFinalizeCallback, bufferVal, fullPath));

    if(false)
    {
ErrorRunFinalize:
        if(fileContentsFinalizeCallback != nullptr)
        {
            fileContentsFinalizeCallback((void*)fileContents);
        }
    }
Error:
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
        if(!doTTReplay)
        {
            wprintf(_u("Sentinel js file is only ok when in TTReplay mode!!!\n"));
            return E_FAIL;
        }

        jsrtAttributes = static_cast<JsRuntimeAttributes>(jsrtAttributes | JsRuntimeAttributeEnableExperimentalFeatures);

        IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateReplayRuntime(jsrtAttributes, ttUri, ttUriLength, Helpers::TTCreateStreamCallback, Helpers::TTReadBytesFromStreamCallback, Helpers::TTFlushAndCloseStreamCallback, nullptr, &runtime));
        chRuntime = runtime;

        JsContextRef context = JS_INVALID_REFERENCE;
        IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateContext(runtime, true, &context));
        IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));

        IfFailGo(RunScript(fileName, fileContents, WScriptJsrt::FinalizeFree, nullptr, nullptr));

        unsigned int rcount = 0;
        IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(nullptr));
        ChakraRTInterface::JsRelease(context, &rcount);
        AssertMsg(rcount == 0, "Should only have had 1 ref from replay code and one ref from current context??");
#endif
    }
    else
    {
        LPCOLESTR contentsRaw = nullptr;

        char fullPath[_MAX_PATH];
        size_t len = 0;

        hr = Helpers::LoadScriptFromFile(fileName, fileContents, &lengthBytes);
        contentsRaw; lengthBytes; // Unused for now.

        IfFailGo(hr);
        if (HostConfigFlags::flags.GenerateLibraryByteCodeHeaderIsEnabled)
        {
            jsrtAttributes = (JsRuntimeAttributes)(jsrtAttributes | JsRuntimeAttributeSerializeLibraryByteCode);
        }

#if ENABLE_TTD
        if (doTTRecord)
        {
            //Ensure we run with experimental features (as that is what Node does right now).
            jsrtAttributes = static_cast<JsRuntimeAttributes>(jsrtAttributes | JsRuntimeAttributeEnableExperimentalFeatures);

            IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateRecordRuntime(jsrtAttributes, snapInterval, snapHistoryLength, Helpers::TTCreateStreamCallback, Helpers::TTWriteBytesToStreamCallback, Helpers::TTFlushAndCloseStreamCallback, nullptr, &runtime));
            chRuntime = runtime;

            JsContextRef context = JS_INVALID_REFERENCE;
            IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateContext(runtime, true, &context));

#if ENABLE_TTD
            //We need this here since this context is created in record
            IfJsErrorFailLog(ChakraRTInterface::JsSetObjectBeforeCollectCallback(context, nullptr, WScriptJsrt::JsContextBeforeCollectCallback));
#endif

            IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));
        }
        else
        {
            AssertMsg(!doTTReplay, "Should be handled in the else case above!!!");

            IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(jsrtAttributes, nullptr, &runtime));
            chRuntime = runtime;

            if (HostConfigFlags::flags.DebugLaunch)
            {
                Debugger* debugger = Debugger::GetDebugger(runtime);
                debugger->StartDebugging(runtime);
            }

            JsContextRef context = JS_INVALID_REFERENCE;
            IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));

            //Don't need collect callback since this is always in replay

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

        JsContextRef context = JS_INVALID_REFERENCE;
        IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
        IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));
#endif

#ifdef DEBUG
        ChakraRTInterface::SetCheckOpHelpersFlag(true);
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        ChakraRTInterface::SetOOPCFGRegistrationFlag(false);
#endif

        if (!WScriptJsrt::Initialize())
        {
            IfFailGo(E_FAIL);
        }

        if (_fullpath(fullPath, fileName, _MAX_PATH) == nullptr)
        {
            IfFailGo(E_FAIL);
        }

        len = strlen(fullPath);
        if (HostConfigFlags::flags.GenerateLibraryByteCodeHeaderIsEnabled)
        {
            if (HostConfigFlags::flags.GenerateLibraryByteCodeHeader != nullptr && *HostConfigFlags::flags.GenerateLibraryByteCodeHeader != _u('\0'))
            {
                CHAR libraryName[_MAX_PATH];
                CHAR ext[_MAX_EXT];
                _splitpath_s(fullPath, NULL, 0, NULL, 0, libraryName, _countof(libraryName), ext, _countof(ext));

                IfFailGo(CreateLibraryByteCodeHeader(fileContents, WScriptJsrt::FinalizeFree, lengthBytes, HostConfigFlags::flags.GenerateLibraryByteCodeHeader, libraryName));
            }
            else
            {
                fwprintf(stderr, _u("FATAL ERROR: -GenerateLibraryByteCodeHeader must provide the file name, i.e., -GenerateLibraryByteCodeHeader:<bytecode file name>, exiting\n"));
                IfFailGo(E_FAIL);
            }
        }
        else if (HostConfigFlags::flags.SerializedIsEnabled)
        {
            CreateAndRunSerializedScript(fileName, fileContents, WScriptJsrt::FinalizeFree, fullPath);
        }
        else
        {
            IfFailGo(RunScript(fileName, fileContents, WScriptJsrt::FinalizeFree, nullptr, fullPath));
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
#ifdef _WIN32 // looks on linux it always leak ThreadContextTLSEntry since there's no DllMain
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

#ifdef _WIN32
bool HandleJITServerFlag(int& argc, _Inout_updates_to_(argc, argc) LPWSTR argv[])
{
    LPCWSTR flag = L"-jitserver:";
    LPCWSTR flagWithoutColon = L"-jitserver";
    size_t flagLen = wcslen(flag);

    int i = 0;
    for (i = 1; i < argc; ++i)
    {
        if (!_wcsicmp(argv[i], flagWithoutColon))
        {
            connectionUuidString = L"";
            break;
        }
        else if (!_wcsnicmp(argv[i], flag, flagLen))
        {
            connectionUuidString = argv[i] + flagLen;
            if (wcslen(connectionUuidString) == 0)
            {
                fwprintf(stdout, L"[FAILED]: must pass a UUID to -jitserver:\n");
                return false;
            }
            else
            {
                break;
            }
        }
    }

    if (i == argc)
    {
        return false;
    }

    // remove this flag now
    HostConfigFlags::RemoveArg(argc, argv, i);

    return true;
}

typedef HRESULT(WINAPI *JsInitializeJITServerPtr)(UUID* connectionUuid, void* securityDescriptor, void* alpcSecurityDescriptor);

int _cdecl RunJITServer(int argc, __in_ecount(argc) LPWSTR argv[])
{
    ChakraRTInterface::ArgInfo argInfo = { argc, argv, PrintUsage, nullptr };
    HINSTANCE chakraLibrary = nullptr;
    bool success = ChakraRTInterface::LoadChakraDll(&argInfo, &chakraLibrary);
    int status = 0;
    JsInitializeJITServerPtr initRpcServer = nullptr;

    if (!success)
    {
        wprintf(L"\nDll load failed\n");
        return ERROR_DLL_INIT_FAILED;
    }

    UUID connectionUuid;
    status = UuidFromStringW((RPC_WSTR)connectionUuidString, &connectionUuid);
    if (status != RPC_S_OK)
    {
        goto cleanup;
    }

    initRpcServer = (JsInitializeJITServerPtr)GetProcAddress(chakraLibrary, "JsInitializeJITServer");
    status = initRpcServer(&connectionUuid, nullptr, nullptr);
    if (FAILED(status))
    {
        wprintf(L"InitializeJITServer failed by 0x%x\n", status);
        goto cleanup;
    }
    status = 0;

cleanup:
    if (chakraLibrary)
    {
        ChakraRTInterface::UnloadChakraDll(chakraLibrary);
    }
    return status;
}
#endif

unsigned int WINAPI StaticThreadProc(void *lpParam)
{
    ChakraRTInterface::ArgInfo* argInfo = static_cast<ChakraRTInterface::ArgInfo* >(lpParam);
    return ExecuteTestWithMemoryCheck(argInfo->filename);
}

#ifndef _WIN32
static char16** argv = nullptr;
int main(int argc, char** c_argv)
{
#ifndef CHAKRA_STATIC_LIBRARY
// xplat-todo: PAL free CH ?
    PAL_InitializeChakraCore();
#endif
    int origargc = argc; // store for clean-up later
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

#ifdef _WIN32
    bool runJITServer = HandleJITServerFlag(argc, argv);
#endif
    int retval = -1;
    HRESULT exitCode = E_FAIL;
    int cpos = 1;
    HINSTANCE chakraLibrary = nullptr;
    bool success = false;
    ChakraRTInterface::ArgInfo argInfo;
#ifdef _WIN32
    ATOM lock;
#endif

    if (argc < 2)
    {
        PrintUsage();
        PAL_Shutdown();
        retval = EXIT_FAILURE;
        goto return_cleanup;
    }

#ifdef _WIN32
    if (runJITServer)
    {
        retval = RunJITServer(argc, argv);
        goto return_cleanup;
    }
#endif

    for(int i = 1; i < argc; ++i)
    {
        const wchar *arg = argv[i];
        size_t arglen = wcslen(arg);

        // support - or / prefix for flags
        if (arglen >= 1 && (arg[0] == _u('-')
#ifdef _WIN32
            || arg[0] == _u('/') // '/' prefix for legacy (Windows-only because it starts a path on Unix)
#endif
            ))
        {
            // support -- prefix for flags
            if (arglen >= 2 && arg[0] == _u('-') && arg[1] == _u('-'))
            {
                arg += 2; // advance past -- prefix
            }
            else
            {
                arg += 1; // advance past - or / prefix
            }
        }

        arglen = wcslen(arg); // get length of flag after prefix
        if ((arglen == 1 && wcsncmp(arg, _u("v"),       arglen) == 0) ||
            (arglen == 7 && wcsncmp(arg, _u("version"), arglen) == 0))
        {
            PrintVersion();
            PAL_Shutdown();
            retval = EXIT_SUCCESS;
            goto return_cleanup;
        }
        else if (
#if !defined(ENABLE_DEBUG_CONFIG_OPTIONS) // release builds can display some kind of help message
            (arglen == 1 && wcsncmp(arg, _u("?"),    arglen) == 0) ||
#endif
            (arglen == 1 && wcsncmp(arg, _u("h"),    arglen) == 0) ||
            (arglen == 4 && wcsncmp(arg, _u("help"), arglen) == 0)
            )
        {
            PrintUsage();
            PAL_Shutdown();
            retval = EXIT_SUCCESS;
            goto return_cleanup;
        }
        else if(wcsstr(argv[i], _u("-TTRecord=")) == argv[i])
        {
            doTTRecord = true;
            wchar* ruri = argv[i] + wcslen(_u("-TTRecord="));
            Helpers::GetTTDDirectory(ruri, &ttUriLength, ttUri, ttUriBufferLength);
        }
        else if(wcsstr(argv[i], _u("-TTReplay=")) == argv[i])
        {
            doTTReplay = true;
            wchar* ruri = argv[i] + wcslen(_u("-TTReplay="));
            Helpers::GetTTDDirectory(ruri, &ttUriLength, ttUri, ttUriBufferLength);
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
            wchar *temp = argv[cpos];
            argv[cpos] = argv[i];
            argv[i] = temp;
            cpos++;
        }
    }
    argc = cpos;

    if(doTTRecord & doTTReplay)
    {
        fwprintf(stderr, _u("Cannot run in record and replay at same time!!!"));
        ExitProcess(0);
    }

    HostConfigFlags::pfnPrintUsage = PrintUsageFormat;

    // The following code is present to make sure we don't load
    // jscript9.dll etc with ch. Since that isn't a concern on non-Windows
    // builds, it's safe to conditionally compile it out.
#ifdef _WIN32
    lock = ::AddAtom(szChakraCoreLock);
    AssertMsg(lock, "failed to lock chakracore.dll");
#endif // _WIN32

    HostConfigFlags::HandleArgsFlag(argc, argv);

    argInfo = { argc, argv, PrintUsage, nullptr };
    success = ChakraRTInterface::LoadChakraDll(&argInfo, &chakraLibrary);

#if defined(CHAKRA_STATIC_LIBRARY) && !defined(NDEBUG)
    // handle command line flags
    OnChakraCoreLoaded(OnChakraCoreLoadedEntry);
#endif

    if (argInfo.filename == nullptr)
    {
        WideStringToNarrowDynamic(argv[1], &argInfo.filename);
    }

    if (success)
    {
#ifdef _WIN32
#if ENABLE_NATIVE_CODEGEN
        if (HostConfigFlags::flags.OOPJIT)
        {
            // TODO: Error checking
            JITProcessManager::StartRpcServer(argc, argv);
            ChakraRTInterface::ConnectJITServer(JITProcessManager::GetRpcProccessHandle(), nullptr, JITProcessManager::GetRpcConnectionId());
        }
#endif
        HANDLE threadHandle;
        threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, &StaticThreadProc, &argInfo, STACK_SIZE_PARAM_IS_A_RESERVATION, 0));

        if (threadHandle != nullptr)
        {
            DWORD waitResult = WaitForSingleObject(threadHandle, INFINITE);
            Assert(waitResult == WAIT_OBJECT_0);
            DWORD threadExitCode;
            GetExitCodeThread(threadHandle, &threadExitCode);
            exitCode = (HRESULT)threadExitCode;
            CloseHandle(threadHandle);
        }
        else
        {
            fwprintf(stderr, _u("FATAL ERROR: failed to create worker thread error code %d, exiting\n"), errno);
            AssertMsg(false, "failed to create worker thread");
        }
#else
        // On linux, execute on the same thread
        exitCode = ExecuteTestWithMemoryCheck(argInfo.filename);
#endif

        ChakraRTInterface::UnloadChakraDll(chakraLibrary);
    }
#if ENABLE_NATIVE_CODEGEN && defined(_WIN32)
    JITProcessManager::TerminateJITServer();
#endif

    PAL_Shutdown();
    retval = (int)exitCode;

return_cleanup:
#ifndef _WIN32
    if(argv != nullptr)
    {
        for(int i=0;i<origargc;i++)
        {
            free(argv[i]);
            argv[i] = nullptr;
        }
    }
    delete[] argv;
    argv = nullptr;
#endif
    return retval;
}
