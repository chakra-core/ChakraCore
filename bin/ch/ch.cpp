//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "core/AtomLockGuids.h"

unsigned int MessageBase::s_messageCount = 0;

LPCWSTR hostName = L"ch.exe";
JsRuntimeHandle chRuntime = JS_INVALID_RUNTIME_HANDLE;

BOOL doTTRecord = false;
BOOL doTTDebug = false;
wchar_t* ttUri = nullptr;

wchar_t* dbgIPAddr = nullptr;
unsigned short dbgPort = 0;

extern "C"
HRESULT __stdcall OnChakraCoreLoadedEntry(TestHooks& testHooks)
{
    return ChakraRTInterface::OnChakraCoreLoaded(testHooks);
}

JsRuntimeAttributes jsrtAttributes = static_cast<JsRuntimeAttributes>(JsRuntimeAttributeAllowScriptInterrupt);
LPCWSTR JsErrorCodeToString(JsErrorCode jsErrorCode)
{
    switch (jsErrorCode)
    {
    case JsNoError:
        return L"JsNoError";
        break;

    case JsErrorInvalidArgument:
        return L"JsErrorInvalidArgument";
        break;

    case JsErrorNullArgument:
        return L"JsErrorNullArgument";
        break;

    case JsErrorNoCurrentContext:
        return L"JsErrorNoCurrentContext";
        break;

    case JsErrorInExceptionState:
        return L"JsErrorInExceptionState";
        break;

    case JsErrorNotImplemented:
        return L"JsErrorNotImplemented";
        break;

    case JsErrorWrongThread:
        return L"JsErrorWrongThread";
        break;

    case JsErrorRuntimeInUse:
        return L"JsErrorRuntimeInUse";
        break;

    case JsErrorBadSerializedScript:
        return L"JsErrorBadSerializedScript";
        break;

    case JsErrorInDisabledState:
        return L"JsErrorInDisabledState";
        break;

    case JsErrorCannotDisableExecution:
        return L"JsErrorCannotDisableExecution";
        break;

    case JsErrorHeapEnumInProgress:
        return L"JsErrorHeapEnumInProgress";
        break;

    case JsErrorOutOfMemory:
        return L"JsErrorOutOfMemory";
        break;

    case JsErrorScriptException:
        return L"JsErrorScriptException";
        break;

    case JsErrorScriptCompile:
        return L"JsErrorScriptCompile";
        break;

    case JsErrorScriptTerminated:
        return L"JsErrorScriptTerminated";
        break;

    case JsErrorFatal:
        return L"JsErrorFatal";
        break;

    default:
        return L"<unknown>";
        break;
    }
}

#define IfJsErrorFailLog(expr) do { JsErrorCode jsErrorCode = expr; if ((jsErrorCode) != JsNoError) { fwprintf(stderr, L"ERROR: " TEXT(#expr) L" failed. JsErrorCode=0x%x (%s)\n", jsErrorCode, JsErrorCodeToString(jsErrorCode)); fflush(stderr); goto Error; } } while (0)

int HostExceptionFilter(int exceptionCode, _EXCEPTION_POINTERS *ep)
{
    ChakraRTInterface::NotifyUnhandledException(ep);

    bool crashOnException = false;
    ChakraRTInterface::GetCrashOnExceptionFlag(&crashOnException);

    if (exceptionCode == EXCEPTION_BREAKPOINT || (crashOnException && exceptionCode != 0xE06D7363))
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    fwprintf(stderr, L"FATAL ERROR: %ls failed due to exception code %x\n", hostName, exceptionCode);
    fflush(stderr);

    return EXCEPTION_EXECUTE_HANDLER;
}

void __stdcall PrintUsageFormat()
{
    wprintf(L"\nUsage: ch.exe [flaglist] filename\n");
}

void __stdcall PrintUsage()
{
#ifndef DEBUG
    wprintf(L"\nUsage: ch.exe filename"
            L"\n[flaglist] is not supported for Release mode\n");
#else
    PrintUsageFormat();
    wprintf(L"Try 'ch.exe -?' for help\n");
#endif
}

// On success the param byteCodeBuffer will be allocated in the function.
// The caller of this function should de-allocate the memory.
HRESULT GetSerializedBuffer(LPCOLESTR fileContents, __out BYTE **byteCodeBuffer, __out DWORD *byteCodeBufferSize)
{
    HRESULT hr = S_OK;
    *byteCodeBuffer = nullptr;
    *byteCodeBufferSize = 0;
    BYTE *bcBuffer = nullptr;

    DWORD bcBufferSize = 0;
    IfJsErrorFailLog(ChakraRTInterface::JsSerializeScript(fileContents, bcBuffer, &bcBufferSize));
    // Above call will return the size of the buffer only, once succeed we need to allocate memory of that much and call it again.
    if (bcBufferSize == 0)
    {
        AssertMsg(false, "bufferSize should not be zero");
        IfFailGo(E_FAIL);
    }
    bcBuffer = new BYTE[bcBufferSize];
    DWORD newBcBufferSize = bcBufferSize;
    IfJsErrorFailLog(ChakraRTInterface::JsSerializeScript(fileContents, bcBuffer, &newBcBufferSize));
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

HRESULT CreateLibraryByteCodeHeader(LPCOLESTR fileContents, BYTE * contentsRaw, DWORD lengthBytes, LPCWSTR bcFullPath, LPCWSTR libraryNameWide)
{
    HRESULT hr = S_OK;
    HANDLE bcFileHandle = nullptr;
    BYTE *bcBuffer = nullptr;
    DWORD bcBufferSize = 0;
    IfFailGo(GetSerializedBuffer(fileContents, &bcBuffer, &bcBufferSize));

    bcFileHandle = CreateFile(bcFullPath, GENERIC_WRITE, FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (bcFileHandle == INVALID_HANDLE_VALUE)
    {
        IfFailGo(E_FAIL);
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
    size_t convertedChars;
    char libraryNameNarrow[MAX_PATH + 1];
    IfFalseGo((wcstombs_s(&convertedChars, libraryNameNarrow, libraryNameWide, _TRUNCATE) == 0));
    IfFalseGo(WriteFile(bcFileHandle, libraryNameNarrow, (DWORD)strlen(libraryNameNarrow), &written, nullptr));
    outputStr = "[] = {\r\n/* 00000000 */";
    IfFalseGo(WriteFile(bcFileHandle, outputStr, (DWORD)strlen(outputStr), &written, nullptr));

    for (unsigned int i = 0; i < bcBufferSize; i++)
    {
        char scratch[6];
        auto scratchLen = sizeof(scratch);
        int num = _snprintf_s(scratch, scratchLen, " 0x%02X", bcBuffer[i]);
        Assert(num == 5);
        IfFalseGo(WriteFile(bcFileHandle, scratch, (DWORD)(scratchLen - 1), &written, nullptr));

        // Add a comma and a space if this is not the last item
        if (i < bcBufferSize - 1)
        {
            char commaSpace[2];
            _snprintf_s(commaSpace, sizeof(commaSpace), ",");  // close quote, new line, offset and open quote
            IfFalseGo(WriteFile(bcFileHandle, commaSpace, (DWORD)strlen(commaSpace), &written, nullptr));
        }

        // Add a line break every 16 scratches, primarily so the compiler doesn't complain about the string being too long.
        // Also, won't add for the last scratch
        if (i % 16 == 15 && i < bcBufferSize - 1)
        {
            char offset[17];
            _snprintf_s(offset, sizeof(offset), "\r\n/* %08X */", i + 1);  // close quote, new line, offset and open quote
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

#if ENABLE_TTD
    ChakraRTInterface::JsTTDNotifyHostCallbackCreatedOrCanceled(false, false, task, msg->GetId());
#endif

    messageQueue->Push(msg);
}

static void TTDebuggerCommands(wchar_t* msg)
{
    wprintf(L"Enter a command.\n");

    wprintf(L"c  - continue\n");
    wprintf(L"s  - step\n");
    wprintf(L"si - step into\n");

    wprintf(L"rl - reverse local\n");
    wprintf(L"rd - reverse dynamic\n");

    wprintf(L"o - originator for callback (if available)\n");
}

static void TTDebuggerPrintVar(wchar_t* buff)
{
    wchar_t* ebuff = buff + 1;
    while(*ebuff != L'\0' && iswblank(*ebuff))
    {
        ebuff++;
    }

    if(wcslen(ebuff) == 0)
    {
        wprintf(L"Bad variable name: %ls\n", buff);
        return;
    }

    ChakraRTInterface::JsTTDPrintVariable(ebuff);
}

static bool TTDebuggerHandleStep(wchar_t* buff)
{
    if(wcslen(buff) == 1 && buff[0] == L'c')
    {
        ChakraRTInterface::JsTTDSetContinueBP();
        return true;
    }
    else
    {
        if(wcslen(buff) == 1 && buff[0] == L's')
        {
            ChakraRTInterface::JsTTDSetStepBP(false);
            return true;
        }
        else if(wcslen(buff) == 2 && buff[0] == L's' && buff[1] == L'i')
        {
            ChakraRTInterface::JsTTDSetStepBP(true);
            return true;
        }
        else
        {
            TTDebuggerCommands(L"Unknown command.\n");
            return false;
        }
    }
}

static bool TTDebuggerHandleReverseStep(wchar_t* buff, INT64* optEventTimeRequest, wchar_t** optStaticRequestMessage)
{
    *optEventTimeRequest = -1;
    *optStaticRequestMessage = nullptr;

    if(wcslen(buff) == 2 && buff[1] == L'l')
    {
        bool noPrevious = false;
        INT64 rootEventTime = -1;
        UINT64 ftime = 0;
        UINT64 ltime = 0;
        UINT32 line = 0;
        UINT32 column = 0;
        UINT32 sourceId = 0;
        ChakraRTInterface::JsTTDGetExecutionTimeInfo(true, &noPrevious, &rootEventTime, &ftime, &ltime, &line, &column, &sourceId);

        if(noPrevious)
        {
            wprintf(L"No previous statment in this event -- use originator command (o)\n");
            return false;
        }
        else
        {
            *optEventTimeRequest = rootEventTime;
            *optStaticRequestMessage = L"Reverse step in local scope.";

            ChakraRTInterface::JsTTDSetBP(rootEventTime, ftime, ltime, line, column, sourceId);
            return true;
        }
    }
    else if(wcslen(buff) == 2 && buff[1] == L'd')
    {
        INT64 rootEventTime = -1;
        UINT64 ftime = 0;
        UINT64 ltime = 0;
        UINT32 line = 0;
        UINT32 column = 0;
        UINT32 sourceId = 0;

        //first see if we just returned from a call (we may have an exception as well but be in a finally)
        bool hasReturn = false;
        ChakraRTInterface::JsTTDGetLastFunctionReturnTimeInfo(&hasReturn, &rootEventTime, &ftime, &ltime, &line, &column, &sourceId);

        if(hasReturn)
        {
            *optEventTimeRequest = rootEventTime;
            *optStaticRequestMessage = L"Reverse to callee return.";

            ChakraRTInterface::JsTTDSetBP(rootEventTime, ftime, ltime, line, column, sourceId);
            return true;
        }

        //next see if we have an exception
        bool hasException = false;
        ChakraRTInterface::JsTTDGetLastExceptionThrowTimeInfo(&hasException, &rootEventTime, &ftime, &ltime, &line, &column, &sourceId);

        if(hasException)
        {
            *optEventTimeRequest = rootEventTime;
            *optStaticRequestMessage = L"Reverse to exception source (from breakpoint).";

            ChakraRTInterface::JsTTDSetBP(rootEventTime, ftime, ltime, line, column, sourceId);
            return true;
        }

        //finally see if we can just reverse locally
        bool noPrevious = false;
        ChakraRTInterface::JsTTDGetExecutionTimeInfo(true, &noPrevious, &rootEventTime, &ftime, &ltime, &line, &column, &sourceId);

        if(noPrevious)
        {
            wprintf(L"No previous statment in this event -- use originator command (o)\n");
            return false;
        }
        else
        {
            *optEventTimeRequest = rootEventTime;
            *optStaticRequestMessage = L"Reverse step in local scope.";

            ChakraRTInterface::JsTTDSetBP(rootEventTime, ftime, ltime, line, column, sourceId);
            return true;
        }
    }
    else
    {
        TTDebuggerCommands(L"Unknown command.\n");
        return false;
    }
}

static bool TTDebuggerHandleGotoOriginator(wchar_t* buff, INT64* optEventTimeRequest, wchar_t** optStaticRequestMessage)
{
    *optEventTimeRequest = -1;
    *optStaticRequestMessage = nullptr;

    if(wcslen(buff) == 1)
    {
        bool hasEvent = false;
        bool hasEventInfo = false;
        INT64 rootEventTime = -1;
        UINT64 ftime = 0;
        UINT64 ltime = 0;
        UINT32 line = 0;
        UINT32 column = 0;
        UINT32 sourceId = 0;
        ChakraRTInterface::JsTTDGetCurrentCallbackOperationTimeInfo(true, &hasEvent, &hasEventInfo, &rootEventTime, &ftime, &ltime, &line, &column, &sourceId);

        if(!hasEvent)
        {
            wprintf(L"The origin of this event is not known (before the recording was initiated) or it was invoked directly by the host.\n");
            return false;
        }
        if(!hasEventInfo)
        {
            //
            //TODO: we need to fire off a sequence of steps with the top-level handler to:
            //      (1) Execute the specified event
            //      (2) Get the info and set the breakpoint
            //

            wprintf(L"Event with the originator info was not previously executed -- we don't support automatic re-execution yet!!!\n");
            return false;
        }
        else
        {
            *optEventTimeRequest = rootEventTime;
            *optStaticRequestMessage = L"Reverse step in local scope.";

            ChakraRTInterface::JsTTDSetBP(rootEventTime, ftime, ltime, line, column, sourceId);
            return true;
        }
    }
    else
    {
        TTDebuggerCommands(L"Unknown command.\n");
        return false;
    }
}

static bool CALLBACK TTDebuggerCallback(INT64* optEventTimeRequest, wchar_t** optStaticRequestMessage)
{
    wchar_t inputArray[256];
    *optEventTimeRequest = -1;
    *optStaticRequestMessage = nullptr;

    while(true)
    {
        wprintf(L"TTDebug>");
        std::fgetws(inputArray, 256, stdin);

        //trim any leading whitespace
        wchar_t* buff = inputArray;
        while(*buff != L'\0' && iswblank(*buff))
        {
            buff++;
        }

        //get rid of \n
        wchar_t* tail = buff + wcslen(buff) - 1;
        *tail = L'\0';
        tail--;

        //trim any tailing whitespace
        while(tail >= buff && iswblank(*tail))
        {
            *tail = L'\0';
            tail--;
        }

        if(wcslen(buff) == 0)
        {
            TTDebuggerCommands(L"Enter a command.\n");
        }
        else if(buff[0] == L'p')
        {
            TTDebuggerPrintVar(buff);
        }
        else if(buff[0] == L'c' || buff[0] == L's')
        {
            bool ok = TTDebuggerHandleStep(buff);
            if(ok)
            {
                return true;
            }
        }
        else if(buff[0] == L'r')
        {
            bool ok = TTDebuggerHandleReverseStep(buff, optEventTimeRequest, optStaticRequestMessage);
            if(ok)
            {
                return false;
            }
        }
        else if(buff[0] == L'o')
        {
            bool ok = TTDebuggerHandleGotoOriginator(buff, optEventTimeRequest, optStaticRequestMessage);
            if(ok)
            {
                return false;
            }
        }
        else if(wcslen(buff) == 1 && buff[0] == L'q')
        {
            ExitProcess(0);
        }
        else
        {
            TTDebuggerCommands(L"Unknown command.\n");
        }
    }
}

JsValueRef LoadNamedProperty(JsValueRef obj, LPCWSTR name)
{
    JsPropertyIdRef pid;
    ChakraRTInterface::JsGetPropertyIdFromName(name, &pid);
    JsValueRef val;
    ChakraRTInterface::JsGetProperty(obj, pid, &val);
    return val;
}
unsigned int LoadNamedPropertyAsUInt(JsValueRef obj, LPCWSTR name)
{
    JsValueRef rval = LoadNamedProperty(obj, name);
    int val = -1;
    ChakraRTInterface::JsNumberToInt(rval, &val);
    AssertMsg(val >= 0, "Failed conversion.");
    return (unsigned int)val;
}

void StartupDebuggerPortAsNeeded()
{
    if(dbgIPAddr != nullptr)
    {
        wchar_t* path = (wchar_t*)CoTaskMemAlloc(MAX_PATH * sizeof(wchar_t));
        path[0] = L'\0';

        GetModuleFileName(NULL, path, MAX_PATH);

        //
        //TODO: this is an ugly semi-hard coded path we need to fix up
        //
        wchar_t* spos = wcsstr(path, L"\\ch.exe");
        AssertMsg(spos != nullptr, "Something got renamed or moved!!!");

        int ccount = (int)((((byte*)spos) - ((byte*)path)) / sizeof(wchar_t));
        std::wstring dbgPath;
        dbgPath.append(path, 0, ccount);
        dbgPath.append(L"\\..\\chakra_debug.js");

        LPCWSTR contents = nullptr;
        Helpers::LoadScriptFromFile(dbgPath.c_str(), contents);

        DebuggerCh::SetDbgSrcInfo(contents);
        DebuggerCh::StartDebugging(chRuntime, dbgIPAddr, dbgPort);

        CoTaskMemFree(path);
    }
}

void CreateDirectoryIfNeeded(const wchar_t* path)
{
    bool isPathDirName = (path[wcslen(path) - 1] == L'\\');

    std::wstring fullpath(path);
    if(!isPathDirName)
    {
        fullpath.append(L"\\");
    }

    DWORD dwAttrib = GetFileAttributes(fullpath.c_str());
    if((dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        return;
    }

    BOOL success = CreateDirectory(fullpath.c_str(), NULL);
    if(!success)
    {
        DWORD lastError = GetLastError();
        LPTSTR pTemp = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, lastError, LANG_NEUTRAL, (LPTSTR)&pTemp, 0, NULL);
        fwprintf(stderr, L": %s", pTemp);

        AssertMsg(false, "Failed Directory Create");
    }
}

void DeleteDirectory(const wchar_t* path)
{
    HANDLE hFile;
    WIN32_FIND_DATA FileInformation;

    bool isPathDirName = (path[wcslen(path) - 1] == L'\\');

    std::wstring strPattern(path);
    if(!isPathDirName)
    {
        strPattern.append(L"\\");
    }
    strPattern.append(L"*.*");

    hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if(FileInformation.cFileName[0] != '.')
            {
                std::wstring strFilePath(path);
                if(!isPathDirName)
                {
                    strFilePath.append(L"\\");
                }
                strFilePath.append(FileInformation.cFileName);

                if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    DeleteDirectory(strFilePath.c_str());
                    ::RemoveDirectory(strFilePath.c_str());
                }
                else
                {
                    // Set file attributes
                    ::SetFileAttributes(strFilePath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    ::DeleteFile(strFilePath.c_str());
                }
            }
        } while(::FindNextFile(hFile, &FileInformation) == TRUE);

        // Close handle
        ::FindClose(hFile);
    }
}

void GetFileFromURI(const wchar_t* uri, std::wstring& res)
{
    int urilen = (int)wcslen(uri);
    int fpos = 0;
    for(int spos = urilen - 1; spos >= 0; --spos)
    {
        if(uri[spos] == L'\\' || uri[spos] == L'/')
        {
            fpos = spos + 1;
            break;
        }
    }

    res.append(uri + fpos);
}

void GetDefaultTTDDirectory(std::wstring& res, const wchar_t* optExtraDir)
{
    wchar_t* path = (wchar_t*)CoTaskMemAlloc(MAX_PATH * sizeof(wchar_t));
    path[0] = L'\0';

    GetModuleFileName(NULL, path, MAX_PATH);

    wchar_t* spos = wcsstr(path, L"\\Build\\VcBuild\\");
    AssertMsg(spos != nullptr, "Something got renamed or moved!!!");

    int ccount = (int)((((byte*)spos) - ((byte*)path)) / sizeof(wchar_t));
    res.append(path, 0, ccount);
    res.append(L"\\test\\_ttdlog\\");

    if(wcslen(optExtraDir) == 0)
    {
        res.append(L"_defaultLog");
    }
    else
    {
        res.append(optExtraDir);
    }

    wchar_t lastChar = res.back();
    if(lastChar != '\\')
    {
        res.append(L"\\");
    }

    CoTaskMemFree(path);
}

static void CALLBACK GetTTDDirectory(const wchar_t* uri, wchar_t** fullTTDUri)
{
    std::wstring logDir;

    if(uri[0] != L'!')
    {
        logDir.append(uri);
    }
    else
    {
        GetDefaultTTDDirectory(logDir, uri + 1);
    }

    if(logDir.back() != L'\\')
    {
        logDir.push_back(L'\\');
    }

    int uriLength = (int)(logDir.length() + 1);
    *fullTTDUri = (wchar_t*)CoTaskMemAlloc(uriLength * sizeof(wchar_t));
    memcpy(*fullTTDUri, logDir.c_str(), uriLength * sizeof(wchar_t));
}

static void CALLBACK TTInitializeForWriteLogStreamCallback(const wchar_t* uri)
{
    //Clear the logging directory so it is ready for us to write into
    DeleteDirectory(uri);
}

static HANDLE CALLBACK TTGetLogStreamCallback(const wchar_t* uri, bool read, bool write)
{
    AssertMsg((read | write) & !(read & write), "Should be either read or write and at least one.");

    std::wstring logFile(uri);
    logFile.append(L"ttdlog.json");

    HANDLE res = INVALID_HANDLE_VALUE;
    if(read)
    {
        res = CreateFile(logFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    else
    {
        CreateDirectoryIfNeeded(uri);

        res = CreateFile(logFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if(res == INVALID_HANDLE_VALUE)
    {
        DWORD lastError = GetLastError();
        LPTSTR pTemp = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, lastError, LANG_NEUTRAL, (LPTSTR)&pTemp, 0, NULL);
        fwprintf(stderr, L": %s\n", pTemp);
        fwprintf(stderr, L"Failed on file: %ls\n", logFile.c_str());

        AssertMsg(false, "Failed File Open");
    }

    return res;
}

static HANDLE CALLBACK TTGetSnapshotStreamCallback(const wchar_t* logRootUri, const wchar_t* snapId, bool read, bool write, wchar_t** snapContainerUri)
{
    AssertMsg((read | write) & !(read & write), "Should be either read or write and at least one.");

    std::wstring snapDir(logRootUri);
    snapDir.append(L"snap_");
    snapDir.append(snapId);
    snapDir.append(L"\\");

    int resUriCount = (int)(wcslen(snapDir.c_str()) + 1);
    *snapContainerUri = (wchar_t*)CoTaskMemAlloc(resUriCount * sizeof(wchar_t));
    memcpy(*snapContainerUri, snapDir.c_str(), resUriCount * sizeof(wchar_t));

    std::wstring snapFile(snapDir);
    snapFile.append(L"snapshot.json");

    HANDLE res = INVALID_HANDLE_VALUE;
    if(read)
    {
        res = CreateFile(snapFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    else
    {
        //create the directory if it does not exist
        CreateDirectory(snapDir.c_str(), NULL);

        res = CreateFile(snapFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if(res == INVALID_HANDLE_VALUE)
    {
        DWORD lastError = GetLastError();
        LPTSTR pTemp = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, lastError, LANG_NEUTRAL, (LPTSTR)&pTemp, 0, NULL);
        fwprintf(stderr, L": %s\n", pTemp);
        fwprintf(stderr, L"Failed on file: %ls\n", snapFile.c_str());

        AssertMsg(false, "Failed File Open");
    }

    return res;
}

static HANDLE CALLBACK TTGetSrcCodeStreamCallback(const wchar_t* snapContainerUri, const wchar_t* documentid, const wchar_t* srcFileName, bool read, bool write)
{
    AssertMsg((read | write) & !(read & write), "Should be either read or write and at least one.");

    std::wstring sFile;
    GetFileFromURI(srcFileName, sFile);

    std::wstring srcPath(snapContainerUri);
    srcPath.append(documentid);
    srcPath.append(L"_");
    srcPath.append(sFile.c_str());

    HANDLE res = INVALID_HANDLE_VALUE;
    if(read)
    {
        res = CreateFile(srcPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    else
    {
        res = CreateFile(srcPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if(res == INVALID_HANDLE_VALUE)
    {
        DWORD lastError = GetLastError();
        LPTSTR pTemp = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, lastError, LANG_NEUTRAL, (LPTSTR)&pTemp, 0, NULL);
        fwprintf(stderr, L": %s\n", pTemp);
        fwprintf(stderr, L"Failed on file: %ls\n", srcPath.c_str());

        AssertMsg(false, "Failed File Open");
    }

    return res;
}

static BOOL CALLBACK TTReadBytesFromStreamCallback(HANDLE strm, BYTE* buff, DWORD size, DWORD* readCount)
{
    AssertMsg(strm != INVALID_HANDLE_VALUE, "Bad file handle.");

    *readCount = 0;
    BOOL ok = ReadFile(strm, buff, size, readCount, NULL);
    AssertMsg(ok, "Read failed.");

    return ok;
}

static BOOL CALLBACK TTWriteBytesToStreamCallback(HANDLE strm, BYTE* buff, DWORD size, DWORD* writtenCount)
{
    AssertMsg(strm != INVALID_HANDLE_VALUE, "Bad file handle.");

    BOOL ok = WriteFile(strm, buff, size, writtenCount, NULL);
    AssertMsg(*writtenCount == size, "Write Failed");

    return ok;
}

static void CALLBACK TTFlushAndCloseStreamCallback(HANDLE strm, bool read, bool write)
{
    AssertMsg((read | write) & !(read & write), "Should be either read or write and at least one.");

    if(strm != INVALID_HANDLE_VALUE)
    {
        if(write)
        {
            FlushFileBuffers(strm);
        }

        CloseHandle(strm);
    }
}

HRESULT RunScript(LPCWSTR fileName, LPCWSTR fileContents, BYTE *bcBuffer, wchar_t *fullPath, bool firstScript)
{
    HRESULT hr = S_OK;
    MessageQueue * messageQueue = new MessageQueue();
    WScriptJsrt::AddMessageQueue(messageQueue);

    IfJsErrorFailLog(ChakraRTInterface::JsSetPromiseContinuationCallback(PromiseContinuationCallback, (void*)messageQueue));

    if(wcslen(fileName) >= 14 && wcscmp(fileName + wcslen(fileName) - 14, L"ttdSentinal.js") == 0)
    {
#if !ENABLE_TTD
        wprintf(L"Sential js file is only ok when in TTDebug mode!!!\n");
        return E_FAIL;
#else
        if(!doTTDebug)
        {
            wprintf(L"Sential js file is only ok when in TTDebug mode!!!\n");
            return E_FAIL;
        }

        ChakraRTInterface::JsTTDStartTimeTravelDebugging();

        ChakraRTInterface::JsTTDSetDebuggerCallback(&TTDebuggerCallback);
        ChakraRTInterface::JsTTDSetStepBP(firstScript); //ignored if we set the free-run flag

        try
        {
            INT64 snapEventTime = -1;
            INT64 nextEventTime = -2;

            while(true)
            {
                IfJsErrorFailLog(ChakraRTInterface::JsTTDPrepContextsForTopLevelEventMove(chRuntime, nextEventTime, &snapEventTime));

                ChakraRTInterface::JsTTDMoveToTopLevelEvent(snapEventTime, nextEventTime);

                JsErrorCode res = ChakraRTInterface::JsTTDReplayExecution(&nextEventTime);

                //handle any uncaught exception by immediately time-traveling to the throwing line
                if(res == JsErrorCategoryScript)
                {
                    INT64 rootEventTime = -1;
                    UINT64 ftime = 0;
                    UINT64 ltime = 0;
                    UINT32 line = 0;
                    UINT32 column = 0;
                    UINT32 sourceId = 0;

                    //first see if we have an exception
                    bool hasException = false;
                    ChakraRTInterface::JsTTDGetLastExceptionThrowTimeInfo(&hasException, &rootEventTime, &ftime, &ltime, &line, &column, &sourceId);

                    if(hasException)
                    {
                        wprintf(L"An unhandled script exception occoured!!!\n");
                        wprintf(L"Time-Travel to throwing line y/n?\n");

                        wchar_t input[16];
                        std::fgetws(input, 16, stdin);
                        if(input[0] == 'y' || input[0] == 'Y')
                        {
                            wprintf(L"Time-travel to throwing line initiated...\n");

                            nextEventTime = rootEventTime;
                            ChakraRTInterface::JsTTDSetBP(rootEventTime, ftime, ltime, line, column, sourceId);
                        }
                        else
                        {
                            ExitProcess(0);
                        }
                    }
                }

                if(nextEventTime == -1)
                {
                    wprintf(L"\nReached end of Execution -- Exiting.\n");
                    break;
                }
            }
        }
        catch(...)
        {
            wprintf(L"Exception.");
        }
#endif
    }
    else
    {
        Assert(fileContents != nullptr || bcBuffer != nullptr);
        JsErrorCode runScript;
        if(bcBuffer != nullptr)
        {
            runScript = ChakraRTInterface::JsRunSerializedScript(fileContents, bcBuffer, WScriptJsrt::GetNextSourceContext(), fullPath, nullptr /*result*/);
        }
        else
        {
            JsValueRef functionRef;
            runScript = ChakraRTInterface::JsParseScriptWithFlags(fileContents, WScriptJsrt::GetNextSourceContext(), fullPath, JsParseScriptAttributeNone, &functionRef);
            unsigned short argc = 1;
            JsValueRef argv[1];
            ChakraRTInterface::JsGetUndefinedValue(argv);

            if(firstScript && dbgIPAddr != nullptr)
            {
                JsValueRef functionInfo;
                ChakraRTInterface::JsDiagGetFunctionPosition(functionRef, &functionInfo);

                unsigned int scriptId = LoadNamedPropertyAsUInt(functionInfo, L"scriptId");
                unsigned int stmtLine = LoadNamedPropertyAsUInt(functionInfo, L"stmtStartLine");
                unsigned int stmtColumn = LoadNamedPropertyAsUInt(functionInfo, L"stmtStartColumn");

                JsValueRef bp;
                ChakraRTInterface::JsDiagSetBreakpoint(scriptId, stmtLine, stmtColumn, &bp);
            }

            if(runScript == JsNoError)
            {
#if ENABLE_TTD
                if(doTTRecord)
                {
                    ChakraRTInterface::JsTTDStartTimeTravelRecording();
                }

                runScript = ChakraRTInterface::JsTTDCallFunction(-1, functionRef, argv, argc, nullptr /*result*/);
#else
                runScript = ChakraRTInterface::JsCallFunction(functionRef, argv, argc, nullptr /*result*/);
#endif
            }
        }

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
    if (messageQueue != nullptr)
    {
        delete messageQueue;
    }
    return hr;
}

HRESULT CreateAndRunSerializedScript(LPCWSTR fileName, LPCWSTR fileContents, wchar_t *fullPath)
{
    HRESULT hr = S_OK;
    JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
    JsContextRef context = JS_INVALID_REFERENCE, current = JS_INVALID_REFERENCE;
    BYTE *bcBuffer = nullptr;
    DWORD bcBufferSize = 0;
    IfFailGo(GetSerializedBuffer(fileContents, &bcBuffer, &bcBufferSize));

    // Bytecode buffer is created in one runtime and will be executed on different runtime.

    IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(jsrtAttributes, nullptr, &runtime));
    chRuntime = runtime;

    IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
    IfJsErrorFailLog(ChakraRTInterface::JsGetCurrentContext(&current));
    IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));

    // Initialized the WScript object on the new context
    if (!WScriptJsrt::Initialize())
    {
        IfFailGo(E_FAIL);
    }

    IfFailGo(RunScript(fileName, fileContents, bcBuffer, fullPath, false));

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

HRESULT ExecuteTest(LPCWSTR fileName)
{
    HRESULT hr = S_OK;
    LPCWSTR fileContents = nullptr;
    JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;

    if(wcslen(fileName) >= 14 && wcscmp(fileName + wcslen(fileName) - 14, L"ttdSentinal.js") == 0)
    {
#if !ENABLE_TTD
        wprintf(L"Sential js file is only ok when in TTDebug mode!!!\n");
        return E_FAIL;
#else
        if(!doTTDebug)
        {
            wprintf(L"Sential js file is only ok when in TTDebug mode!!!\n");
            return E_FAIL;
        }

        IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateDebugRuntime(jsrtAttributes, ttUri, nullptr, &runtime));
        chRuntime = runtime;

        StartupDebuggerPortAsNeeded();
        ChakraRTInterface::JsTTDSetIOCallbacks(runtime, &GetTTDDirectory, &TTInitializeForWriteLogStreamCallback, &TTGetLogStreamCallback, &TTGetSnapshotStreamCallback, &TTGetSrcCodeStreamCallback, &TTReadBytesFromStreamCallback, &TTWriteBytesToStreamCallback, &TTFlushAndCloseStreamCallback);

        JsContextRef context = JS_INVALID_REFERENCE;
        IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateContext(runtime, &context));
        IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));

        if(!WScriptJsrt::Initialize())
        {
            IfFailGo(E_FAIL);
        }

        IfFailGo(RunScript(fileName, fileContents, nullptr, nullptr, true));
#endif
    }
    else
    {
        bool isUtf8 = false;
        LPCOLESTR contentsRaw = nullptr;
        UINT lengthBytes = 0;
        hr = Helpers::LoadScriptFromFile(fileName, fileContents, &isUtf8, &contentsRaw, &lengthBytes);
        contentsRaw; lengthBytes; // Unused for now.

        IfFailGo(hr);
        if(HostConfigFlags::flags.GenerateLibraryByteCodeHeaderIsEnabled)
        {
            jsrtAttributes = (JsRuntimeAttributes)(jsrtAttributes | JsRuntimeAttributeSerializeLibraryByteCode);
        }

#if ENABLE_TTD
        if(doTTRecord)
        {
            IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateRecordRuntime(jsrtAttributes, ttUri, nullptr, &runtime));
            chRuntime = runtime;

            StartupDebuggerPortAsNeeded();
            ChakraRTInterface::JsTTDSetIOCallbacks(runtime, &GetTTDDirectory, &TTInitializeForWriteLogStreamCallback, &TTGetLogStreamCallback, &TTGetSnapshotStreamCallback, &TTGetSrcCodeStreamCallback, &TTReadBytesFromStreamCallback, &TTWriteBytesToStreamCallback, &TTFlushAndCloseStreamCallback);

            JsContextRef context = JS_INVALID_REFERENCE;
            IfJsErrorFailLog(ChakraRTInterface::JsTTDCreateContext(runtime, &context));
            IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));
        }
        else
        {
            IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(jsrtAttributes, nullptr, &runtime));
            chRuntime = runtime;

            StartupDebuggerPortAsNeeded();

            JsContextRef context = JS_INVALID_REFERENCE;
            IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
            IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));
        }
#else
        IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(jsrtAttributes, nullptr, &runtime));
        chRuntime = runtime;

        JsContextRef context = JS_INVALID_REFERENCE;
        IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
        IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));
#endif

        if(!WScriptJsrt::Initialize())
        {
            IfFailGo(E_FAIL);
        }

        wchar_t fullPath[_MAX_PATH];

        if(_wfullpath(fullPath, fileName, _MAX_PATH) == nullptr)
        {
            IfFailGo(E_FAIL);
        }

        // canonicalize that path name to lower case for the profile storage
        size_t len = wcslen(fullPath);
        for(size_t i = 0; i < len; i++)
        {
            fullPath[i] = towlower(fullPath[i]);
        }

        if(HostConfigFlags::flags.GenerateLibraryByteCodeHeaderIsEnabled)
        {
            if(isUtf8)
            {
                if(HostConfigFlags::flags.GenerateLibraryByteCodeHeader != nullptr && *HostConfigFlags::flags.GenerateLibraryByteCodeHeader != L'\0')
                {
                    WCHAR libraryName[_MAX_PATH];
                    WCHAR ext[_MAX_EXT];
                    _wsplitpath_s(fullPath, NULL, 0, NULL, 0, libraryName, _countof(libraryName), ext, _countof(ext));

                    IfFailGo(CreateLibraryByteCodeHeader(fileContents, (BYTE*)contentsRaw, lengthBytes, HostConfigFlags::flags.GenerateLibraryByteCodeHeader, libraryName));
                }
                else
                {
                    fwprintf(stderr, L"FATAL ERROR: -GenerateLibraryByteCodeHeader must provide the file name, i.e., -GenerateLibraryByteCodeHeader:<bytecode file name>, exiting\n");
                    IfFailGo(E_FAIL);
                }
            }
            else
            {
                fwprintf(stderr, L"FATAL ERROR: GenerateLibraryByteCodeHeader flag can only be used on UTF8 file, exiting\n");
                IfFailGo(E_FAIL);
            }
        }
        else if(HostConfigFlags::flags.SerializedIsEnabled)
        {
            if(isUtf8)
            {
                CreateAndRunSerializedScript(fileName, fileContents, fullPath);
            }
            else
            {
                fwprintf(stderr, L"FATAL ERROR: Serialized flag can only be used on UTF8 file, exiting\n");
                IfFailGo(E_FAIL);
            }
        }
        else
        {
            IfFailGo(RunScript(fileName, fileContents, nullptr, fullPath, true));
        }
    }

Error:
    DebuggerCh::CloseDebuggerIfNeeded();

    ChakraRTInterface::JsSetCurrentContext(nullptr);

    if (runtime != JS_INVALID_RUNTIME_HANDLE)
    {
        ChakraRTInterface::JsDisposeRuntime(runtime);
    }

    _flushall();

    return hr;
}

HRESULT ExecuteTestWithMemoryCheck(BSTR fileName)
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

    __try
    {
        hr = ExecuteTest(fileName);
    }
    __except (HostExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
    {
        _flushall();

        // Exception happened, so we probably didn't clean up properly,
        // Don't exit normally, just terminate
        TerminateProcess(::GetCurrentProcess(), GetExceptionCode());
    }

    _flushall();
#ifdef CHECK_MEMORY_LEAK
    ChakraRTInterface::SetEnableCheckMemoryLeakOutput(true);
#endif
    return hr;
}


unsigned int WINAPI StaticThreadProc(void *lpParam)
{
    ChakraRTInterface::ArgInfo* argInfo = static_cast<ChakraRTInterface::ArgInfo* >(lpParam);
    _endthreadex(ExecuteTestWithMemoryCheck(*(argInfo->filename)));
    return 0;
}

int _cdecl wmain(int argc, __in_ecount(argc) LPWSTR argv[])
{
    if(argc < 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    int cpos = -1;
    for(int i = 0; i < argc; ++i)
    {
        if(wcsstr(argv[i], L"-TTRecord:") == argv[i])
        {
            doTTRecord = true;
            ttUri = argv[i] + wcslen(L"-TTRecord:");
        }
        else if(wcsstr(argv[i], L"-TTDebug:") == argv[i])
        {
            doTTDebug = true;
            ttUri = argv[i] + wcslen(L"-TTDebug:");
        }
        else if(wcsstr(argv[i], L"--debug-brk=") == argv[i])
        {
            dbgIPAddr = L"127.0.0.1";

            LPCWSTR portStr = argv[i] + wcslen(L"--debug-brk=");
            dbgPort = (unsigned short)_wtoi(portStr);
        }
        else
        {
            cpos++;
        }

        if(cpos != i)
        {
            argv[cpos] = argv[i];
        }
    }
    argc = cpos + 1;

    HostConfigFlags::pfnPrintUsage = PrintUsageFormat;

    ATOM lock = ::AddAtom(szChakraCoreLock);
    AssertMsg(lock, "failed to lock chakracore.dll");

    HostConfigFlags::HandleArgsFlag(argc, argv);

    CComBSTR fileName;

    ChakraRTInterface::ArgInfo argInfo = { argc, argv, PrintUsage, &fileName.m_str };
    HINSTANCE chakraLibrary = ChakraRTInterface::LoadChakraDll(argInfo);

    if (fileName.m_str == nullptr) 
    {
        fileName = CComBSTR(argv[1]);
    }

    if (chakraLibrary != nullptr)
    {
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
            fwprintf(stderr, L"FATAL ERROR: failed to create worker thread error code %d, exiting\n", errno);
            AssertMsg(false, "failed to create worker thread");
        }
        ChakraRTInterface::UnloadChakraDll(chakraLibrary);
    }

    return 0;
}
