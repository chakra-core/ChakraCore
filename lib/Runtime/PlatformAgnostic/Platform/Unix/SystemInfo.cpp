//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Common.h"
#include "ChakraPlatform.h"

namespace PlatformAgnostic
{
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
