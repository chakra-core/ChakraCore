//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class Helpers
{
public :
    static HRESULT LoadScriptFromFile(LPCSTR filename, LPCSTR& contents, UINT* lengthBytesOut = nullptr);
    static LPCWSTR JsErrorCodeToString(JsErrorCode jsErrorCode);
    static void LogError(__in __nullterminated const char16 *msg, ...);
    static HRESULT LoadBinaryFile(LPCSTR filename, LPCSTR& contents, UINT& lengthBytes, bool printFileOpenError = true);

    static void TTReportLastIOErrorAsNeeded(BOOL ok, const char* msg);
    static void CreateTTDDirectoryAsNeeded(size_t* uriLength, char* uri, const char* asciiDir1, const wchar* asciiDir2);
    static void GetTTDDirectory(const wchar* curi, size_t* uriLength, char* uri, size_t bufferLength);

    static JsTTDStreamHandle CALLBACK TTCreateStreamCallback(size_t uriLength, const char* uri, size_t asciiNameLength, const char* asciiName, bool read, bool write);
    static bool CALLBACK TTReadBytesFromStreamCallback(JsTTDStreamHandle handle, byte* buff, size_t size, size_t* readCount);
    static bool CALLBACK TTWriteBytesToStreamCallback(JsTTDStreamHandle handle, const byte* buff, size_t size, size_t* writtenCount);
    static void CALLBACK TTFlushAndCloseStreamCallback(JsTTDStreamHandle handle, bool read, bool write);
};
