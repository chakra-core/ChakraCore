//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Common.h"
#include "ChakraPlatform.h"
#include <sys/sysctl.h>

namespace PlatformAgnostic
{
    SystemInfo::PlatformData SystemInfo::data;

    SystemInfo::PlatformData::PlatformData()
    {
        // Get Total Ram
        int totalRamHW [] = { CTL_HW, HW_MEMSIZE };

        size_t length = sizeof(size_t);
        if(sysctl(totalRamHW, 2, &totalRam, &length, NULL, 0) == -1)
        {
            totalRam = 0;
        }
    }

    bool SystemInfo::GetMaxVirtualMemory(size_t *totalAS)
    {
        struct rlimit limit;
        if (getrlimit(RLIMIT_AS, &limit) != 0)
        {
            return false;
        }

        *totalAS = limit.rlim_cur;
        return true;
    }
}
