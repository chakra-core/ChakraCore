//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "PlatformAgnostic/ChPlatformAgnostic.h"
#include "Common.h"
#include <strsafe.h>

namespace ChPlatformAgnostic
{
    bool Module::GetBinaryLocation(char* const path, const charcount_t size, charcount_t* const resultStrLength)
    {
        size_t bufferSize = sizeof(WCHAR) * size;

        if (path == nullptr || bufferSize < size) 
        {
            printf("GetBinaryLocation failed: invalid parameters");
            return false;
        }

        LPWSTR wpath = (WCHAR*)malloc(bufferSize);
        if (!wpath)
        {
            printf("GetBinaryLocation failed: out of memory!");
            return false;
        }

        DWORD numCharsCopied = GetModuleFileNameW(NULL, wpath, size);
        if (numCharsCopied == 0)
        {
            printf("GetBinaryLocation failed: GetModuleFileNameW failed with 0x%x!", ::GetLastError());
            free(wpath);
            return false;
        }

        // Terminate the buffer in case GetModuleFileName didn't
        wpath[size - 1] = L'\0';

        numCharsCopied = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, path, size, NULL, NULL);
        free(wpath);

        if (numCharsCopied == 0)
        {
            printf("GetBinaryLocation failed: WideCharToMultiByte failed with 0x%x!", ::GetLastError());
            return false;
        }

        *resultStrLength = numCharsCopied;
        return true;
    }
}
