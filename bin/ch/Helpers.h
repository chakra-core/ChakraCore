//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class Helpers
{
public :
    static HRESULT LoadScriptFromFile(LPCSTR filename, LPCWSTR& contents, bool* isUtf8Out = nullptr, LPCWSTR* contentsRawOut = nullptr, UINT* lengthBytesOut = nullptr, bool printFileOpenError = true);

    static HRESULT WideStringToNarrowDynamic(LPCWSTR sourceString, LPSTR* destStringPtr);
    static HRESULT NarrowStringToWideDynamic(LPCSTR sourceString, LPWSTR* destStringPtr);
    static LPCWSTR JsErrorCodeToString(JsErrorCode jsErrorCode);
    static void LogError(__in __nullterminated const char16 *msg, ...);

    static void TTReportLastIOErrorAsNeeded(BOOL ok, char* msg);
    static void CreateDirectoryIfNeeded(const char16* path);
    static void DeleteDirectory(const char16* path);
    static void GetFileFromURI(const char16* uri, char16** res);
    static void GetDefaultTTDDirectory(char16** res, const char16* optExtraDir);

    static void CALLBACK GetTTDDirectory(const char16* uri, char16** fullTTDUri);
    static void CALLBACK TTInitializeForWriteLogStreamCallback(const char16* uri);
    static HANDLE TTOpenStream_Helper(const char16* uri, bool read, bool write);

    static HANDLE CALLBACK TTGetLogStreamCallback(const char16* uri, bool read, bool write);
    static HANDLE CALLBACK TTGetSnapshotStreamCallback(const char16* uri, const char16* snapId, bool read, bool write);
    static HANDLE CALLBACK TTGetSrcCodeStreamCallback(const char16* uri, const char16* bodyCtrId, const char16* srcFileName, bool read, bool write);
    static bool CALLBACK TTReadBytesFromStreamCallback(HANDLE handle, BYTE* buff, DWORD size, DWORD* readCount);
    static bool CALLBACK TTWriteBytesToStreamCallback(HANDLE handle, BYTE* buff, DWORD size, DWORD* writtenCount);
    static void CALLBACK TTFlushAndCloseStreamCallback(HANDLE handle, bool read, bool write);
};
