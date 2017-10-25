//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <stdafx.h>
#include <ChPlatformAgnostic.h>
#include <mach-o/dyld.h> // _NSGetExecutablePath

namespace ChPlatformAgnostic
{
    bool Module::GetBinaryLocation(char* const path, const charcount_t size, charcount_t* const resultStrLength)
    {
        if (path == nullptr || resultStrLength == nullptr || size > UINT_MAX) return false;

        char executablePath[PATH_MAX];
        uint32_t path_size = _countof(executablePath);
        if (_NSGetExecutablePath(executablePath, &path_size))
        {
            printf("GetBinaryLocation failed: _NSGetExecutablePath returned 0.");
            return false;
        }

        char *result = realpath(executablePath, path);

        *resultStrLength = strlen(result);
        
        return true;
    }
}
