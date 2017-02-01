//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#include <sys/stat.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h> // _NSGetExecutablePath
#elif defined(__linux__)
#include <unistd.h> // readlink
#elif !defined(_WIN32)
#error "How to get the executable path for this platform?"
#endif // _WIN32 ?

//TODO: x-plat definitions
#ifdef _WIN32
typedef char16 TTDHostCharType;
typedef struct _wfinddata_t TTDHostFileInfo;
typedef intptr_t TTDHostFindHandle;
typedef struct _stat TTDHostStatType;

#define TTDHostPathSeparator _u("\\")
#define TTDHostPathSeparatorChar _u('\\')
#define TTDHostFindInvalid -1

size_t TTDHostStringLength(const TTDHostCharType* str)
{
    return wcslen(str);
}

void TTDHostInitEmpty(TTDHostCharType* dst)
{
    dst[0] = _u('\0');
}

void TTDHostInitFromUriBytes(TTDHostCharType* dst, const byte* uriBytes, size_t uriBytesLength)
{
    memcpy_s(dst, MAX_PATH * sizeof(TTDHostCharType), uriBytes, uriBytesLength);
    dst[uriBytesLength / sizeof(TTDHostCharType)] = _u('\0');

    AssertMsg(wcslen(dst) == (uriBytesLength / sizeof(TTDHostCharType)), "We have an null in the uri or our math is wrong somewhere.");
}

void TTDHostAppend(TTDHostCharType* dst, size_t dstLength, const TTDHostCharType* src)
{
    size_t srcLength = TTDHostStringLength(src);
    size_t dpos = TTDHostStringLength(dst);
    Helpers::TTReportLastIOErrorAsNeeded(dpos < dstLength, "The end of the string already exceeds the buffer");

    size_t srcByteLength = srcLength * sizeof(TTDHostCharType);
    size_t dstRemainingByteLength = (dstLength - dpos - 1) * sizeof(TTDHostCharType);
    Helpers::TTReportLastIOErrorAsNeeded(srcByteLength <= dstRemainingByteLength, "The source string must be able to fit within the destination buffer");

    memcpy_s(dst + dpos, dstRemainingByteLength, src, srcByteLength);
    dst[dpos + srcLength] = _u('\0');
}

void TTDHostAppendWChar(TTDHostCharType* dst, size_t dstLength, const wchar* src)
{
    size_t srcLength = wcslen(src);
    size_t dpos = TTDHostStringLength(dst);
    Helpers::TTReportLastIOErrorAsNeeded(dpos < dstLength, "The end of the string already exceeds the buffer");

    size_t dstRemainingLength = dstLength - dpos - 1;
    Helpers::TTReportLastIOErrorAsNeeded(srcLength <= dstRemainingLength, "The source string must be able to fit within the destination buffer");

    for(size_t i = 0; i < srcLength; ++i)
    {
        dst[dpos + i] = (char16)src[i];
    }

    dst[dpos + srcLength] = _u('\0');
}

void TTDHostAppendAscii(TTDHostCharType* dst, size_t dstLength, const char* src)
{
    size_t srcLength = strlen(src);
    size_t dpos = TTDHostStringLength(dst);
    Helpers::TTReportLastIOErrorAsNeeded(dpos < dstLength, "The end of the string already exceeds the buffer");

    size_t dstRemainingLength = dstLength - dpos - 1;
    Helpers::TTReportLastIOErrorAsNeeded(srcLength <= dstRemainingLength, "The source string must be able to fit within the destination buffer");

    for(size_t i = 0; i < srcLength; ++i)
    {
        dst[dpos + i] = (char16)src[i];
    }

    dst[dpos + srcLength] = _u('\0');
}

void TTDHostBuildCurrentExeDirectory(TTDHostCharType* path, size_t pathBufferLength)
{
    wchar exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    size_t i = wcslen(exePath) - 1;
    while(exePath[i] != TTDHostPathSeparatorChar)
    {
        --i;
    }
    exePath[i + 1] = _u('\0');

    TTDHostAppendWChar(path, pathBufferLength, exePath);
}

JsTTDStreamHandle TTDHostOpen(const TTDHostCharType* path, bool isWrite)
{
    FILE* res = nullptr;
    _wfopen_s(&res, path, isWrite ? _u("w+b") : _u("r+b"));

    return (JsTTDStreamHandle)res;
}

#define TTDHostCWD(dst, dstLength) _wgetcwd(dst, dstLength)
#define TTDDoPathInit(dst, dstLength)
#define TTDHostTok(opath, TTDHostPathSeparator, context) wcstok_s(opath, TTDHostPathSeparator, context)
#define TTDHostStat(cpath, statVal) _wstat(cpath, statVal)

#define TTDHostMKDir(cpath) _wmkdir(cpath)
#define TTDHostCHMod(cpath, flags) _wchmod(cpath, flags)
#define TTDHostRMFile(cpath) _wremove(cpath)

#define TTDHostFindFirst(strPattern, FileInformation) _wfindfirst(strPattern, FileInformation)
#define TTDHostFindNext(hFile, FileInformation) _wfindnext(hFile, FileInformation)
#define TTDHostFindClose(hFile) _findclose(hFile)

#define TTDHostDirInfoName(FileInformation) FileInformation.name

#define TTDHostRead(buff, size, handle) fread_s(buff, size, 1, size, (FILE*)handle);
#define TTDHostWrite(buff, size, handle) fwrite(buff, 1, size, (FILE*)handle)
#else
#include <unistd.h>
#include <cstring>
#include <libgen.h>
#include <dirent.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

typedef utf8char_t TTDHostCharType;
typedef struct dirent* TTDHostFileInfo;
typedef DIR* TTDHostFindHandle;
typedef struct stat TTDHostStatType;

#define TTDHostPathSeparator ((TTDHostCharType*)"/")
#define TTDHostPathSeparatorChar ((TTDHostCharType)'/')
#define TTDHostFindInvalid nullptr

#define TTDHostCharConvert(X) ((char*)X)
#define TTDHostUtf8CharConvert(X) ((TTDHostCharType*)X)

size_t TTDHostStringLength(const TTDHostCharType* str)
{
    return strlen(TTDHostCharConvert(str));
}

void TTDHostInitEmpty(TTDHostCharType* dst)
{
    dst[0] = '\0';
}

void TTDHostInitFromUriBytes(TTDHostCharType* dst, const byte* uriBytes, size_t uriBytesLength)
{
    memcpy_s(dst, MAX_PATH * sizeof(TTDHostCharType), uriBytes, uriBytesLength);
    dst[uriBytesLength / sizeof(TTDHostCharType)] = '\0';

    AssertMsg(TTDHostStringLength(dst) == (uriBytesLength / sizeof(TTDHostCharType)), "We have an null in the uri or our math is wrong somewhere.");
}

void TTDHostAppend(TTDHostCharType* dst, size_t dstLength, const TTDHostCharType* src)
{
    size_t srcLength = TTDHostStringLength(src);
    size_t dpos = TTDHostStringLength(dst);
    Helpers::TTReportLastIOErrorAsNeeded(dpos < dstLength, "The end of the string already exceeds the buffer");

    size_t srcByteLength = srcLength * sizeof(TTDHostCharType);
    size_t dstRemainingByteLength = (dstLength - dpos - 1) * sizeof(TTDHostCharType);
    Helpers::TTReportLastIOErrorAsNeeded(srcByteLength <= dstRemainingByteLength, "The source string must be able to fit within the destination buffer");

    memcpy_s(dst + dpos, dstRemainingByteLength, src, srcByteLength);
    dst[dpos + srcLength] = '\0';
}

void TTDHostAppendWChar(TTDHostCharType* dst, size_t dstLength, const wchar* src)
{
    size_t srcLength = wcslen(src);
    size_t dpos = TTDHostStringLength(dst);
    Helpers::TTReportLastIOErrorAsNeeded(dpos < dstLength, "The end of the string already exceeds the buffer");

    size_t dstRemainingLength = dstLength - dpos - 1;
    Helpers::TTReportLastIOErrorAsNeeded(srcLength <= dstRemainingLength, "The source string must be able to fit within the destination buffer");

    // TODO - analyze this function further
    utf8::EncodeIntoAndNullTerminate(dst + dpos, src, srcLength);
}

void TTDHostAppendAscii(TTDHostCharType* dst, size_t dstLength, const char* src)
{
    size_t srcLength = strlen(src);
    size_t dpos = TTDHostStringLength(dst);
    Helpers::TTReportLastIOErrorAsNeeded(dpos < dstLength, "The end of the string already exceeds the buffer");

    size_t srcByteLength = srcLength * sizeof(TTDHostCharType);
    size_t dstRemainingByteLength = (dstLength - dpos - 1) * sizeof(TTDHostCharType);
    Helpers::TTReportLastIOErrorAsNeeded(srcByteLength <= dstRemainingByteLength, "The source string must be able to fit within the destination buffer");

    memcpy_s(dst + dpos, dstRemainingByteLength, src, srcByteLength);
    dst[dpos + srcLength] = '\0';
}

void TTDHostBuildCurrentExeDirectory(TTDHostCharType* path, size_t pathBufferLength)
{
    TTDHostCharType exePath[MAX_PATH];
    //TODO: xplattodo move this logic to PAL
    #ifdef __APPLE__
    uint32_t tmpPathSize = sizeof(exePath);
    _NSGetExecutablePath(TTDHostCharConvert(exePath), &tmpPathSize);
    size_t len = TTDHostStringLength(exePath);
    #else
    size_t len = readlink("/proc/self/exe", TTDHostCharConvert(exePath), MAX_PATH);
    #endif

    size_t i = len - 1;
    while(exePath[i] != TTDHostPathSeparatorChar)
    {
        --i;
    }

    exePath[i + 1] = '\0';

    TTDHostAppend(path, pathBufferLength, exePath);
}

JsTTDStreamHandle TTDHostOpen(const TTDHostCharType* path, bool isWrite)
{
    return (JsTTDStreamHandle)fopen(TTDHostCharConvert(path), isWrite ? "w+b" : "r+b");
}

#define TTDHostCWD(dst, dstLength) TTDHostUtf8CharConvert(getcwd(TTDHostCharConvert(dst), dstLength))
#define TTDDoPathInit(dst, dstLength) TTDHostAppend(dst, dstLength, TTDHostPathSeparator)
#define TTDHostTok(opath, TTDHostPathSeparator, context) TTDHostUtf8CharConvert(strtok(TTDHostCharConvert(opath), TTDHostCharConvert(TTDHostPathSeparator)))
#define TTDHostStat(cpath, statVal) stat(TTDHostCharConvert(cpath), statVal)

#define TTDHostMKDir(cpath) mkdir(TTDHostCharConvert(cpath), 0777)
#define TTDHostCHMod(cpath, flags) chmod(TTDHostCharConvert(cpath), flags)
#define TTDHostRMFile(cpath) remove(TTDHostCharConvert(cpath))

#define TTDHostFindFirst(strPattern, FileInformation) opendir(TTDHostCharConvert(strPattern))
#define TTDHostFindNext(hFile, FileInformation) (*FileInformation = readdir(hFile))
#define TTDHostFindClose(hFile) closedir(hFile)

#define TTDHostDirInfoName(FileInformation) TTDHostUtf8CharConvert(FileInformation->d_name)

#define TTDHostRead(buff, size, handle) fread(buff, 1, size, (FILE*)handle)
#define TTDHostWrite(buff, size, handle) fwrite(buff, 1, size, (FILE*)handle)
#endif

HRESULT Helpers::LoadScriptFromFile(LPCSTR filename, LPCSTR& contents, UINT* lengthBytesOut /*= nullptr*/)
{
    HRESULT hr = S_OK;
    BYTE * pRawBytes = nullptr;
    UINT lengthBytes = 0;
    contents = nullptr;
    FILE * file = NULL;

    //
    // Open the file as a binary file to prevent CRT from handling encoding, line-break conversions,
    // etc.
    //
    if (fopen_s(&file, filename, "rb") != 0)
    {
#ifdef _WIN32
        DWORD lastError = GetLastError();
        char16 wszBuff[512];
        fprintf(stderr, "Error in opening file '%s' ", filename);
        wszBuff[0] = 0;
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,
            lastError,
            0,
            wszBuff,
            _countof(wszBuff),
            nullptr))
        {
            fwprintf(stderr, _u(": %s"), wszBuff);
        }
        fwprintf(stderr, _u("\n"));
#elif defined(_POSIX_VERSION)
        fprintf(stderr, "Error in opening file: ");
        perror(filename);
#endif
        IfFailGo(E_FAIL);
    }

    if (file != NULL)
    {
        // Determine the file length, in bytes.
        fseek(file, 0, SEEK_END);
        lengthBytes = ftell(file);
        fseek(file, 0, SEEK_SET);
        const size_t bufferLength = lengthBytes + sizeof(BYTE);
        pRawBytes = (LPBYTE)malloc(bufferLength);
        if (nullptr == pRawBytes)
        {
            fwprintf(stderr, _u("out of memory"));
            IfFailGo(E_OUTOFMEMORY);
        }

        //
        // Read the entire content as a binary block.
        //
        size_t readBytes = fread(pRawBytes, sizeof(BYTE), lengthBytes, file);
        if (readBytes < lengthBytes * sizeof(BYTE))
        {
            IfFailGo(E_FAIL);
        }

        pRawBytes[lengthBytes] = 0; // Null terminate it. Could be UTF16

        //
        // Read encoding to make sure it's supported
        //
        // Warning: The UNICODE buffer for parsing is supposed to be provided by the host.
        // This is not a complete read of the encoding. Some encodings like UTF7, UTF1, EBCDIC, SCSU, BOCU could be
        // wrongly classified as ANSI
        //
        {
            C_ASSERT(sizeof(WCHAR) == 2);
            if (bufferLength > 2)
            {
                if ((pRawBytes[0] == 0xFE && pRawBytes[1] == 0xFF) ||
                    (pRawBytes[0] == 0xFF && pRawBytes[1] == 0xFE) ||
                    (bufferLength > 4 && pRawBytes[0] == 0x00 && pRawBytes[1] == 0x00 &&
                        ((pRawBytes[2] == 0xFE && pRawBytes[3] == 0xFF) ||
                         (pRawBytes[2] == 0xFF && pRawBytes[3] == 0xFE))))

                {
                    // unicode unsupported
                    fwprintf(stderr, _u("unsupported file encoding. Only ANSI and UTF8 supported"));
                    IfFailGo(E_UNEXPECTED);
                }
            }
        }
    }

    contents = reinterpret_cast<LPCSTR>(pRawBytes);

Error:
    if (SUCCEEDED(hr))
    {
        if (lengthBytesOut)
        {
            *lengthBytesOut = lengthBytes;
        }
    }

    if (file != NULL)
    {
        fclose(file);
    }

    if (pRawBytes && reinterpret_cast<LPCSTR>(pRawBytes) != contents)
    {
        free(pRawBytes);
    }

    return hr;
}

LPCWSTR Helpers::JsErrorCodeToString(JsErrorCode jsErrorCode)
{
    bool hasException = false;
    ChakraRTInterface::JsHasException(&hasException);
    if (hasException)
    {
        WScriptJsrt::PrintException("", JsErrorScriptException);
    }

    switch (jsErrorCode)
    {
    case JsNoError:
        return _u("JsNoError");
        break;

    case JsErrorInvalidArgument:
        return _u("JsErrorInvalidArgument");
        break;

    case JsErrorNullArgument:
        return _u("JsErrorNullArgument");
        break;

    case JsErrorNoCurrentContext:
        return _u("JsErrorNoCurrentContext");
        break;

    case JsErrorInExceptionState:
        return _u("JsErrorInExceptionState");
        break;

    case JsErrorNotImplemented:
        return _u("JsErrorNotImplemented");
        break;

    case JsErrorWrongThread:
        return _u("JsErrorWrongThread");
        break;

    case JsErrorRuntimeInUse:
        return _u("JsErrorRuntimeInUse");
        break;

    case JsErrorBadSerializedScript:
        return _u("JsErrorBadSerializedScript");
        break;

    case JsErrorInDisabledState:
        return _u("JsErrorInDisabledState");
        break;

    case JsErrorCannotDisableExecution:
        return _u("JsErrorCannotDisableExecution");
        break;

    case JsErrorHeapEnumInProgress:
        return _u("JsErrorHeapEnumInProgress");
        break;

    case JsErrorOutOfMemory:
        return _u("JsErrorOutOfMemory");
        break;

    case JsErrorScriptException:
        return _u("JsErrorScriptException");
        break;

    case JsErrorScriptCompile:
        return _u("JsErrorScriptCompile");
        break;

    case JsErrorScriptTerminated:
        return _u("JsErrorScriptTerminated");
        break;

    case JsErrorFatal:
        return _u("JsErrorFatal");
        break;

    default:
        return _u("<unknown>");
        break;
    }
}

void Helpers::LogError(__in __nullterminated const char16 *msg, ...)
{
    va_list args;
    va_start(args, msg);
    wprintf(_u("ERROR: "));
    vfwprintf(stderr, msg, args);
    wprintf(_u("\n"));
    fflush(stdout);
    va_end(args);
}

HRESULT Helpers::LoadBinaryFile(LPCSTR filename, LPCSTR& contents, UINT& lengthBytes, bool printFileOpenError)
{
    HRESULT hr = S_OK;
    contents = nullptr;
    lengthBytes = 0;
    size_t result;
    FILE * file;

    //
    // Open the file as a binary file to prevent CRT from handling encoding, line-break conversions,
    // etc.
    //
    if (fopen_s(&file, filename, "rb") != 0)
    {
        if (printFileOpenError)
        {
#ifdef _WIN32
            DWORD lastError = GetLastError();
            char16 wszBuff[512];
            fprintf(stderr, "Error in opening file '%s' ", filename);
            wszBuff[0] = 0;
            if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                nullptr,
                lastError,
                0,
                wszBuff,
                _countof(wszBuff),
                nullptr))
            {
                fwprintf(stderr, _u(": %s"), wszBuff);
            }
#endif
            fprintf(stderr, "\n");
            IfFailGo(E_FAIL);
        }
        else
        {
            return E_FAIL;
        }
    }
    // file will not be nullptr if _wfopen_s succeeds
    __analysis_assume(file != nullptr);

    //
    // Determine the file length, in bytes.
    //
    fseek(file, 0, SEEK_END);
    lengthBytes = ftell(file);
    fseek(file, 0, SEEK_SET);
    contents = (LPCSTR)HeapAlloc(GetProcessHeap(), 0, lengthBytes);
    if (nullptr == contents)
    {
        fwprintf(stderr, _u("out of memory"));
        IfFailGo(E_OUTOFMEMORY);
    }
    //
    // Read the entire content as a binary block.
    //
    result = fread((void*)contents, sizeof(char), lengthBytes, file);
    if (result != lengthBytes)
    {
        fwprintf(stderr, _u("Read error"));
        IfFailGo(E_FAIL);
    }
    fclose(file);

Error:
    if (contents && FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, (void*)contents);
        contents = nullptr;
    }

    return hr;
}

void Helpers::TTReportLastIOErrorAsNeeded(BOOL ok, const char* msg)
{
    if(!ok)
    {
#ifdef _WIN32
        DWORD lastError = GetLastError();
        LPTSTR pTemp = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, lastError, 0, (LPTSTR)&pTemp, 0, NULL);
        fwprintf(stderr, _u("Error is: %s\n"), pTemp);
#else
        fprintf(stderr, "Error is: %i %s\n", errno, strerror(errno));
#endif
        fprintf(stderr, "Message is: %s\n", msg);

        AssertMsg(false, "IO Error!!!");
        exit(1);
    }
}

void Helpers::CreateDirectoryIfNeeded(size_t uriByteLength, const byte* uriBytes)
{
    TTDHostCharType opath[MAX_PATH];
    TTDHostInitFromUriBytes(opath, uriBytes, uriByteLength);

    TTDHostCharType cpath[MAX_PATH];
    TTDHostInitEmpty(cpath);
    TTDDoPathInit(cpath, MAX_PATH);

    TTDHostStatType statVal;
    TTDHostCharType* context = nullptr;
    TTDHostCharType* token = TTDHostTok(opath, TTDHostPathSeparator, &context);
    TTDHostAppend(cpath, MAX_PATH, token);

    //At least 1 part of the path must exist so iterate until we find it
    while(TTDHostStat(cpath, &statVal) == -1)
    {
        token = TTDHostTok(nullptr, TTDHostPathSeparator, &context);
        TTDHostAppend(cpath, MAX_PATH, TTDHostPathSeparator);
        TTDHostAppend(cpath, MAX_PATH, token);
    }

    //Now continue until we hit the part that doesn't exist (or the end of the path)
    while(token != nullptr && TTDHostStat(cpath, &statVal) != -1)
    {
        token = TTDHostTok(nullptr, TTDHostPathSeparator, &context);
        if(token != nullptr)
        {
            TTDHostAppend(cpath, MAX_PATH, TTDHostPathSeparator);
            TTDHostAppend(cpath, MAX_PATH, token);
        }
    }

    //Now if there is path left then continue build up the directory tree as we go
    while(token != nullptr)
    {
        int success = TTDHostMKDir(cpath);
        if(success != 0)
        {
            //we may fail because someone else created the directory -- that is ok
            Helpers::TTReportLastIOErrorAsNeeded(errno != ENOENT, "Failed to create directory");
        }

        token = TTDHostTok(nullptr, TTDHostPathSeparator, &context);
        if(token != nullptr)
        {
            TTDHostAppend(cpath, MAX_PATH, TTDHostPathSeparator);
            TTDHostAppend(cpath, MAX_PATH, token);
        }
    }
}

void Helpers::CleanDirectory(size_t uriByteLength, const byte* uriBytes)
{
    TTDHostFindHandle hFile;
    TTDHostFileInfo FileInformation;

    TTDHostCharType strPattern[MAX_PATH];
    TTDHostInitFromUriBytes(strPattern, uriBytes, uriByteLength);
    TTDHostAppendAscii(strPattern, MAX_PATH, "*.*");

    hFile = TTDHostFindFirst(strPattern, &FileInformation);
    if(hFile != TTDHostFindInvalid)
    {
        do
        {
            if(TTDHostDirInfoName(FileInformation)[0] != '.')
            {
                TTDHostCharType strFilePath[MAX_PATH];
                TTDHostInitFromUriBytes(strFilePath, uriBytes, uriByteLength);
                TTDHostAppend(strFilePath, MAX_PATH, TTDHostDirInfoName(FileInformation));

                // Set file attributes
                int statusch = TTDHostCHMod(strFilePath, S_IREAD | S_IWRITE);
                Helpers::TTReportLastIOErrorAsNeeded(statusch == 0, "Failed to chmod directory");

                int statusrm = TTDHostRMFile(strFilePath);
                Helpers::TTReportLastIOErrorAsNeeded(statusrm == 0, "Failed to delete file directory");
            }
        } while(TTDHostFindNext(hFile, &FileInformation) != TTDHostFindInvalid);

        // Close handle
        TTDHostFindClose(hFile);
    }
}

void Helpers::GetTTDDirectory(const wchar* curi, size_t* uriByteLength, byte* uriBytes)
{
    TTDHostCharType turi[MAX_PATH];
    TTDHostInitEmpty(turi);

    if(curi[0] != _u('~'))
    {
        TTDHostCharType* status = TTDHostCWD(turi, MAX_PATH);
        Helpers::TTReportLastIOErrorAsNeeded(status != nullptr, "Failed to chmod directory");

        TTDHostAppend(turi, MAX_PATH, TTDHostPathSeparator);

        TTDHostAppendWChar(turi, MAX_PATH, curi);
    }
    else
    {
        TTDHostBuildCurrentExeDirectory(turi, MAX_PATH);

        TTDHostAppendAscii(turi, MAX_PATH, "_ttdlog");
        TTDHostAppend(turi, MAX_PATH, TTDHostPathSeparator);

        TTDHostAppendWChar(turi, MAX_PATH, curi + 1);
    }

    //add a path separator if one is not already present
    if(curi[wcslen(curi) - 1] != (wchar)TTDHostPathSeparatorChar)
    {
        TTDHostAppend(turi, MAX_PATH, TTDHostPathSeparator);
    }

    size_t turiLength = TTDHostStringLength(turi);

    size_t byteLengthWNull = (turiLength + 1) * sizeof(TTDHostCharType);
    memcpy_s(uriBytes, byteLengthWNull, turi, byteLengthWNull);

    *uriByteLength = turiLength * sizeof(TTDHostCharType);
}

void CALLBACK Helpers::TTInitializeForWriteLogStreamCallback(size_t uriByteLength, const byte* uriBytes)
{
    //If the directory does not exist then we want to create it
    Helpers::CreateDirectoryIfNeeded(uriByteLength, uriBytes);

    //Clear the logging directory so it is ready for us to write into
    Helpers::CleanDirectory(uriByteLength, uriBytes);
}

JsTTDStreamHandle CALLBACK Helpers::TTCreateStreamCallback(size_t uriByteLength, const byte* uriBytes, const char* asciiResourceName, bool read, bool write, byte** relocatedUri, size_t* relocatedUriLength)
{
    AssertMsg((read | write) & (!read | !write), "Read/Write streams not supported yet -- defaulting to read only");

    //relocatedUri and relocatedUriLength are ignored since we don't do code relocation for debugging in CH

    void* res = nullptr;
    TTDHostCharType path[MAX_PATH];
    TTDHostInitFromUriBytes(path, uriBytes, uriByteLength);
    TTDHostAppendAscii(path, MAX_PATH, asciiResourceName);

    res = TTDHostOpen(path, write);
    if(res == nullptr)
    {
#if _WIN32
        fwprintf(stderr, _u("Failed to open file: %ls\n"), path);
#else
        fprintf(stderr, "Failed to open file: %s\n", path);
#endif
    }

    Helpers::TTReportLastIOErrorAsNeeded(res != nullptr, "Failed File Open");
    return res;
}

bool CALLBACK Helpers::TTReadBytesFromStreamCallback(JsTTDStreamHandle handle, byte* buff, size_t size, size_t* readCount)
{
    AssertMsg(handle != nullptr, "Bad file handle.");

    if(size > MAXDWORD)
    {
        *readCount = 0;
        return false;
    }

    BOOL ok = FALSE;
    *readCount = TTDHostRead(buff, size, (FILE*)handle);
    ok = (*readCount != 0);

    Helpers::TTReportLastIOErrorAsNeeded(ok, "Failed Read!!!");

    return ok ? true : false;
}

bool CALLBACK Helpers::TTWriteBytesToStreamCallback(JsTTDStreamHandle handle, const byte* buff, size_t size, size_t* writtenCount)
{
    AssertMsg(handle != nullptr, "Bad file handle.");

    if(size > MAXDWORD)
    {
        *writtenCount = 0;
        return false;
    }

    BOOL ok = FALSE;
    *writtenCount = TTDHostWrite(buff, size, (FILE*)handle);
    ok = (*writtenCount == size);

    Helpers::TTReportLastIOErrorAsNeeded(ok, "Failed Read!!!");

    return ok ? true : false;
}

void CALLBACK Helpers::TTFlushAndCloseStreamCallback(JsTTDStreamHandle handle, bool read, bool write)
{
    fflush((FILE*)handle);
    fclose((FILE*)handle);
}

#define SET_BINARY_PATH_ERROR_MESSAGE(path, msg) \
    str_len = (int) strlen(msg);                 \
    memcpy(path, msg, (size_t)str_len);          \
    path[str_len] = char(0)

void GetBinaryLocation(char *path, const unsigned size)
{
    AssertMsg(path != nullptr, "Path can not be nullptr");
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

// xplat-todo: Implement a corresponding solution for GetModuleFileNameW
// and cleanup PAL. [ https://github.com/Microsoft/ChakraCore/pull/2288 should be merged first ]
// GetModuleFileName* PAL is not reliable and forces us to explicitly double initialize PAL
// with argc / argv....
void GetBinaryPathWithFileNameA(char *path, const size_t buffer_size, const char* filename)
{
    char fullpath[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];

    char modulename[_MAX_PATH];
    GetBinaryLocation(modulename, _MAX_PATH);
    _splitpath_s(modulename, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
    _makepath_s(fullpath, drive, dir, filename, nullptr);

    size_t len = strlen(fullpath);
    if (len < buffer_size)
    {
        memcpy(path, fullpath, len * sizeof(char));
    }
    else
    {
        len = 0;
    }
    path[len] = char(0);
}
