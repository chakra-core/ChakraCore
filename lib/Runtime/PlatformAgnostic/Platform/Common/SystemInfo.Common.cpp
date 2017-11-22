//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "../../Common/Core/CommonTypedefs.h"
#include "../../Common/PlatformAgnostic/SystemInfo.h"
#include "../../../../Common/Codex/Utf8Codex.h"

#if defined(__APPLE__)
#include <mach-o/dyld.h> // _NSGetExecutablePath
#elif defined(__linux__)
#include <unistd.h> // readlink
#endif

namespace PlatformAgnostic
{
    bool SystemInfo::GetBinaryLocation(char *path, const unsigned size)
    {
        // TODO: make AssertMsg available under PlatformAgnostic
        //AssertMsg(path != nullptr, "Path can not be nullptr");
        //AssertMsg(size < INT_MAX, "Isn't it too big for a path buffer?");
#ifdef __APPLE__
        uint32_t path_size = (uint32_t)size;
        char *tmp = nullptr;
        int str_len;
        if (_NSGetExecutablePath(path, &path_size))
        {
            SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: _NSGetExecutablePath has failed.");
            return false;
        }

        tmp = (char*)malloc(size);
        char *result = realpath(path, tmp);
        str_len = strlen(result);
        memcpy(path, result, str_len);
        free(tmp);
        path[str_len] = char(0);
        return true;
#elif defined(__linux__)
        int str_len = readlink("/proc/self/exe", path, size - 1);
        if (str_len <= 0)
        {
            SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: /proc/self/exe has failed.");
            return false;
        }
        path[str_len] = char(0);
        return true;
#else
        #warning "Implement GetBinaryLocation for this platform"
#endif
    }

    bool SystemInfo::GetBinaryLocation(char16 *path, const unsigned size)
    {
        int tmp_size = size * 3;
        char *tmp = (char*)malloc(tmp_size);
        if (!GetBinaryLocation(tmp, tmp_size))
        {
            free(tmp);
            return false;
        }
        if (utf8::DecodeUnitsIntoAndNullTerminate(path, (LPCUTF8&)tmp, (LPCUTF8)tmp + strlen(tmp)) <= 0)
        {
            free(tmp);
            wcscpy_s(path, size, _u("GetBinaryLocation: DecodeUnitsIntoAndNullTerminate has failed."));
            return false;
        }
        free(tmp);
        return true;
    }
} // namespace PlatformAgnostic
