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

    static void TTReportLastIOErrorAsNeeded(BOOL ok, const char* msg);
    static void CreateDirectoryIfNeeded(size_t uriByteLength, const byte* uriBytes);
    static void CleanDirectory(size_t uriByteLength, const byte* uriBytes);

    static void GetTTDDirectory(const wchar* curi, size_t* uriByteLength, byte* uriBytes);

    static void CALLBACK TTInitializeForWriteLogStreamCallback(size_t uriByteLength, const byte* uriBytes);
    static JsTTDStreamHandle CALLBACK TTCreateStreamCallback(size_t uriByteLength, const byte* uriBytes, const char* asciiResourceName, bool read, bool write);

    static bool CALLBACK TTReadBytesFromStreamCallback(JsTTDStreamHandle handle, byte* buff, size_t size, size_t* readCount);
    static bool CALLBACK TTWriteBytesToStreamCallback(JsTTDStreamHandle handle, const byte* buff, size_t size, size_t* writtenCount);
    static void CALLBACK TTFlushAndCloseStreamCallback(JsTTDStreamHandle handle, bool read, bool write);
};
