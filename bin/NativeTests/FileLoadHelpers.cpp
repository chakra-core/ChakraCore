//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "Codex/Utf8Codex.h"

HRESULT FileLoadHelpers::LoadScriptFromFile(LPCSTR filename, LPCWSTR& contents, bool* isUtf8Out, LPCWSTR* contentsRawOut, UINT* lengthBytesOut, bool printFileOpenError)
{
    HRESULT hr = S_OK;
    LPCWSTR contentsRaw = nullptr;
    LPCUTF8 pRawBytes = nullptr;
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
    {
        size_t readBytes = fread((void*)contentsRaw, sizeof(char), lengthBytes, file);
        if (readBytes < lengthBytes)
        {
            fwprintf(stderr, _u("readBytes should be equal to lengthBytes"));
            IfFailGo(E_FAIL);
        }
    }

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

        utf8::DecodeUnitsIntoAndNullTerminate((char16*)contents, pRawBytes, pRawBytes + lengthBytes, decodeOptions);
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
