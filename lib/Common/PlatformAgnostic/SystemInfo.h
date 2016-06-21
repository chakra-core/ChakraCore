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
    };
} // namespace PlatformAgnostic

#endif // RUNTIME_PLATFORM_AGNOSTIC_COMMON_SYSTEMINFO
