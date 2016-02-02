//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#if ENABLE_TTD_FORCE_RECORD_NODE

#pragma once

namespace NodeSupportDefaultImpl
{
    void CALLBACK GetTTDDirectory(const wchar_t* uri, wchar_t** fullTTDUri);
    void CALLBACK TTInitializeForWriteLogStreamCallback(const wchar_t* uri);
    HANDLE CALLBACK TTGetLogStreamCallback(const wchar_t* uri, bool read, bool write);
    HANDLE CALLBACK TTGetSnapshotStreamCallback(const wchar_t* logRootUri, const wchar_t* snapId, bool read, bool write, wchar_t** snapContainerUri);
    HANDLE CALLBACK TTGetSrcCodeStreamCallback(const wchar_t* snapContainerUri, const wchar_t* documentid, const wchar_t* srcFileName, bool read, bool write);
    BOOL CALLBACK TTReadBytesFromStreamCallback(HANDLE strm, BYTE* buff, DWORD size, DWORD* readCount);
    BOOL CALLBACK TTWriteBytesToStreamCallback(HANDLE strm, BYTE* buff, DWORD size, DWORD* writtenCount);
    void CALLBACK TTFlushAndCloseStreamCallback(HANDLE strm, bool read, bool write);
}

#endif
