//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace PlatformAgnostic
{
    class SystemInfo
    {
    public:
        static bool GetMaxVirtualMemory(size_t *totalAS);

#define SET_BINARY_PATH_ERROR_MESSAGE(path, msg) \
    str_len = (int) strlen(msg);                 \
    memcpy(path, msg, (size_t)str_len);          \
    path[str_len] = char(0)

#ifdef _WIN32
        static bool GetBinaryLocation(char *path, const unsigned size)
        {
            // TODO: make AssertMsg available under PlatformAgnostic
            //AssertMsg(path != nullptr, "Path can not be nullptr");
            //AssertMsg(size < INT_MAX, "Isn't it too big for a path buffer?");
            LPWSTR wpath = (WCHAR*)malloc(sizeof(WCHAR) * size);
            int str_len;
            if (!wpath)
            {
                SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: GetModuleFileName has failed. OutOfMemory!");
                return false;
            }
            str_len = GetModuleFileNameW(NULL, wpath, size - 1);
            if (str_len <= 0)
            {
                SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: GetModuleFileName has failed.");
                free(wpath);
                return false;
            }

            str_len = WideCharToMultiByte(CP_UTF8, 0, wpath, str_len, path, size, NULL, NULL);
            free(wpath);

            if (str_len <= 0)
            {
                SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: GetModuleFileName (WideCharToMultiByte) has failed.");
                return false;
            }

            if ((unsigned)str_len > size - 1)
            {
                str_len = (int)size - 1;
            }
            path[str_len] = char(0);
            return true;
        }

        // Overloaded GetBinaryLocation: receive the location of current binary in char16.
        // size: the second parameter is the size of the path buffer in count of wide characters.
        static bool GetBinaryLocation(char16 *path, const unsigned size)
        {
            // TODO: make AssertMsg available under PlatformAgnostic
            //AssertMsg(path != nullptr, "Path can not be nullptr");
            //AssertMsg(size < INT_MAX, "Isn't it too big for a path buffer?");
            int str_len = GetModuleFileNameW(NULL, path, size);
            if (str_len <= 0)
            {
                wcscpy_s(path, size, _u("GetBinaryLocation: GetModuleFileName has failed."));
                return false;
            }
            return true;
        }
#else
        static bool GetBinaryLocation(char *path, const unsigned size);
        static bool GetBinaryLocation(char16 *path, const unsigned size);
#endif
    };
} // namespace PlatformAgnostic
