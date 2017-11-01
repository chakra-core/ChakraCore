//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <stdafx.h>
#include <ChPlatformAgnostic.h>
#include <unistd.h> // readlink
#include <limits.h>
#include <errno.h>

namespace ChPlatformAgnostic
{
    bool Module::GetBinaryLocation(char* const path, const charcount_t size, charcount_t* const resultStrLength)
    {
        if (path == nullptr || resultStrLength == nullptr || size > UINT_MAX) return false;

        int str_len = readlink("/proc/self/exe", path, size - 1);
        if (str_len < 0)
        {
            printf("GetBinaryLocation failed: reading from /proc/self/exe failed with error 0x%x.", errno);
            return false;
        }
        path[str_len] = char(0);

        *resultStrLength = str_len;
        return true;
    }
}
