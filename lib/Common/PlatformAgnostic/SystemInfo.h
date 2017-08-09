//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef RUNTIME_PLATFORM_AGNOSTIC_COMMON_SYSTEMINFO
#define RUNTIME_PLATFORM_AGNOSTIC_COMMON_SYSTEMINFO

namespace PlatformAgnostic
{
    class SystemInfo
    {

        class PlatformData
        {
            public:
            size_t totalRam;

            PlatformData();
        };
        static PlatformData data;
    public:

        static bool GetTotalRam(size_t *totalRam)
        {
            if (SystemInfo::data.totalRam == 0)
            {
                return false;
            }

            *totalRam = SystemInfo::data.totalRam;
            return true;
        }

        static bool GetMaxVirtualMemory(size_t *totalAS);

#define SET_BINARY_PATH_ERROR_MESSAGE(path, msg) \
    str_len = (int) strlen(msg);                 \
    memcpy(path, msg, (size_t)str_len);          \
    path[str_len] = char(0)

#ifdef _WIN32
        static void GetBinaryLocation(char *path, const unsigned size)
        {
            // TODO: make AssertMsg available under PlatformAgnostic
            //AssertMsg(path != nullptr, "Path can not be nullptr");
            //AssertMsg(size < INT_MAX, "Isn't it too big for a path buffer?");
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
                str_len = (int)size - 1;
            }
            path[str_len] = char(0);
        }
#else
        static void GetBinaryLocation(char *path, const unsigned size);
#endif
    };
} // namespace PlatformAgnostic

#endif // RUNTIME_PLATFORM_AGNOSTIC_COMMON_SYSTEMINFO
