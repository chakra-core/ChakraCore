//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "Codex/Utf8Codex.h"

///
/// Use the codex library to encode a UTF16 string to UTF8.
/// The caller is responsible for freeing the memory, which is allocated
/// using malloc.
/// The returned string is null terminated.
///
HRESULT Helpers::WideStringToNarrowDynamic(LPCWSTR sourceString, LPSTR* destStringPtr)
{
    size_t cchSourceString = wcslen(sourceString);
    
    if (cchSourceString >= MAXUINT32)
    {
        return E_OUTOFMEMORY;
    }

    size_t cbDestString = (cchSourceString + 1) * 3;

    // Check for overflow- cbDestString should be >= cchSourceString
    if (cbDestString < cchSourceString)
    {
        return E_OUTOFMEMORY;
    }

    utf8char_t* destString = (utf8char_t*)malloc(cbDestString);
    if (destString == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    size_t cbDecoded = utf8::EncodeIntoAndNullTerminate(destString, sourceString, (charcount_t) cchSourceString);
    Assert(cbDecoded <= cbDestString);
    static_assert(sizeof(utf8char_t) == sizeof(char), "Needs to be valid for cast");
    *destStringPtr = (char*)destString;
    return S_OK;
}

///
/// Use the codex library to encode a UTF8 string to UTF16.
/// The caller is responsible for freeing the memory, which is allocated
/// using malloc.
/// The returned string is null terminated.
///
HRESULT Helpers::NarrowStringToWideDynamic(LPCSTR sourceString, LPWSTR* destStringPtr)
{
    size_t cbSourceString = strlen(sourceString);
    charcount_t cchDestString = utf8::ByteIndexIntoCharacterIndex((LPCUTF8) sourceString, cbSourceString);
    size_t cbDestString = (cchDestString + 1) * sizeof(WCHAR);
    
    // Check for overflow- cbDestString should be >= cchSourceString
    if (cbDestString < cchDestString)
    {
        return E_OUTOFMEMORY;
    }

    WCHAR* destString = (WCHAR*)malloc(cbDestString);
    if (destString == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    utf8::DecodeIntoAndNullTerminate(destString, (LPCUTF8) sourceString, cchDestString);
    static_assert(sizeof(utf8char_t) == sizeof(char), "Needs to be valid for cast");
    *destStringPtr = destString;
    return S_OK;
}

HRESULT Helpers::LoadScriptFromFile(LPCSTR filename, LPCWSTR& contents, bool* isUtf8Out, LPCWSTR* contentsRawOut, UINT* lengthBytesOut, bool printFileOpenError)
{
    HRESULT hr = S_OK;
    LPCWSTR contentsRaw = nullptr;
    byte * pRawBytes = nullptr;
    UINT lengthBytes = 0;
    bool isUtf8 = false;
    contents = nullptr;
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
            fwprintf(stderr, _u("\n"));
#elif defined(_POSIX_VERSION)
            fprintf(stderr, "Error in opening file: ");
            perror(filename);
#endif            
            IfFailGo(E_FAIL);
        }
        else
        {
            return E_FAIL;
        }
    }

    //
    // Determine the file length, in bytes.
    //
    fseek(file, 0, SEEK_END);
    lengthBytes = ftell(file);
    fseek(file, 0, SEEK_SET);
    contentsRaw = (LPCWSTR)HeapAlloc(GetProcessHeap(), 0, lengthBytes + sizeof(WCHAR));
    if (nullptr == contentsRaw)
    {
        fwprintf(stderr, _u("out of memory"));
        IfFailGo(E_OUTOFMEMORY);
    }

    //
    // Read the entire content as a binary block.
    //
    fread((void*)contentsRaw, sizeof(char), lengthBytes, file);
    fclose(file);
    *(WCHAR*)((byte*)contentsRaw + lengthBytes) = _u('\0'); // Null terminate it. Could be LPCWSTR.

    //
    // Read encoding, handling any conversion to Unicode.
    //
    // Warning: The UNICODE buffer for parsing is supposed to be provided by the host.
    // This is not a complete read of the encoding. Some encodings like UTF7, UTF1, EBCDIC, SCSU, BOCU could be
    // wrongly classified as ANSI
    //
    pRawBytes = (byte*)contentsRaw;
    if ((0xEF == *pRawBytes && 0xBB == *(pRawBytes + 1) && 0xBF == *(pRawBytes + 2)))
    {
        isUtf8 = true;
    }
    else if (0xFFFE == *contentsRaw || (0x0000 == *contentsRaw && 0xFEFF == *(contentsRaw + 1)))
    {
        // unicode unsupported
        fwprintf(stderr, _u("unsupported file encoding"));
        IfFailGo(E_UNEXPECTED);
    }
    else if (0xFEFF == *contentsRaw)
    {
        // unicode LE
        contents = contentsRaw;
    }
    else
    {
        // Assume UTF8
        isUtf8 = true;
    }


    if (isUtf8)
    {
        utf8::DecodeOptions decodeOptions = utf8::doAllowInvalidWCHARs;

        UINT cUtf16Chars = utf8::ByteIndexIntoCharacterIndex(pRawBytes, lengthBytes, decodeOptions);
        contents = (LPCWSTR)HeapAlloc(GetProcessHeap(), 0, (cUtf16Chars + 1) * sizeof(WCHAR));
        if (nullptr == contents)
        {
            fwprintf(stderr, _u("out of memory"));
            IfFailGo(E_OUTOFMEMORY);
        }

        utf8::DecodeIntoAndNullTerminate((char16*) contents, pRawBytes, cUtf16Chars, decodeOptions);
    }

Error:
    if (SUCCEEDED(hr) && isUtf8Out)
    {
        Assert(contentsRawOut);
        Assert(lengthBytesOut);
        *isUtf8Out = isUtf8;
        *contentsRawOut = contentsRaw;
        *lengthBytesOut = lengthBytes;
    }
    else if (contentsRaw && (contentsRaw != contents)) // Otherwise contentsRaw is lost. Free it if it is different to contents.
    {
        HeapFree(GetProcessHeap(), 0, (void*)contentsRaw);
    }

    if (contents && FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, (void*)contents);
        contents = nullptr;
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
