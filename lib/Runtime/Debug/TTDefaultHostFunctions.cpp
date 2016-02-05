//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
//THIS FILE IS A TEMP WORKAROUND FOR NODE UNTIL WE ADD MORE HOST SUPPORT
//

#include "RuntimeDebugPch.h"

#if ENABLE_TTD_FORCE_RECORD_NODE

#pragma warning( push )
#pragma warning( disable : 4995 )
#include <string>

namespace NodeSupportDefaultImpl
{
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

        wchar_t* spos = path + wcslen(path);
        while(spos != path && *spos != L'\\')
        {
            spos--;
        }
        AssertMsg(spos != nullptr, "Something got renamed or moved!!!");

        int ccount = (int)((((byte*)spos) - ((byte*)path)) / sizeof(wchar_t));
        res.append(path, 0, ccount);

        if(res.back() != '\\')
        {
            res.append(L"\\");
        }

        res.append(optExtraDir);

        if(res.back() != '\\')
        {
            res.append(L"\\");
        }

        CoTaskMemFree(path);
    }

    void CALLBACK GetTTDDirectory(const wchar_t* uri, wchar_t** fullTTDUri)
    {
        std::wstring logDir;

        GetDefaultTTDDirectory(logDir, uri);

        if(logDir.back() != L'\\')
        {
            logDir.push_back(L'\\');
        }

        int uriLength = (int)(logDir.length() + 1);
        *fullTTDUri = (wchar_t*)CoTaskMemAlloc(uriLength * sizeof(wchar_t));
        memcpy(*fullTTDUri, logDir.c_str(), uriLength * sizeof(wchar_t));
    }

    void CALLBACK TTInitializeForWriteLogStreamCallback(const wchar_t* uri)
    {
        //Clear the logging directory so it is ready for us to write into
        DeleteDirectory(uri);
    }

    HANDLE CALLBACK TTGetLogStreamCallback(const wchar_t* uri, bool read, bool write)
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

    HANDLE CALLBACK TTGetSnapshotStreamCallback(const wchar_t* logRootUri, const wchar_t* snapId, bool read, bool write, wchar_t** snapContainerUri)
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

    HANDLE CALLBACK TTGetSrcCodeStreamCallback(const wchar_t* snapContainerUri, const wchar_t* documentid, const wchar_t* srcFileName, bool read, bool write)
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

    BOOL CALLBACK TTReadBytesFromStreamCallback(HANDLE strm, BYTE* buff, DWORD size, DWORD* readCount)
    {
        AssertMsg(strm != INVALID_HANDLE_VALUE, "Bad file handle.");

        *readCount = 0;
        BOOL ok = ReadFile(strm, buff, size, readCount, NULL);
        AssertMsg(ok, "Read failed.");

        return ok;
    }

    BOOL CALLBACK TTWriteBytesToStreamCallback(HANDLE strm, BYTE* buff, DWORD size, DWORD* writtenCount)
    {
        AssertMsg(strm != INVALID_HANDLE_VALUE, "Bad file handle.");

        BOOL ok = WriteFile(strm, buff, size, writtenCount, NULL);
        AssertMsg(*writtenCount == size, "Write Failed");

        return ok;
    }

    void CALLBACK TTFlushAndCloseStreamCallback(HANDLE strm, bool read, bool write)
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
}

#pragma warning( pop )

#endif