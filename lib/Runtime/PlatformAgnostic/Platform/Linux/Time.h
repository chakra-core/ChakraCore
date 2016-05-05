//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef RUNTIME_PLATFORM_AGNOSTIC_LINUX_TIME
#define RUNTIME_PLATFORM_AGNOSTIC_LINUX_TIME

#include "inc/pal.h"

typedef struct _TIME_ZONE_INFORMATION {
    LONG       Bias;
    WCHAR      StandardName[32];
    SYSTEMTIME StandardDate;
    LONG       StandardBias;
    WCHAR      DaylightName[32];
    SYSTEMTIME DaylightDate;
    LONG       DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

typedef struct _TIME_DYNAMIC_ZONE_INFORMATION {
    LONG       Bias;
    WCHAR      StandardName[32];
    SYSTEMTIME StandardDate;
    LONG       StandardBias;
    WCHAR      DaylightName[32];
    SYSTEMTIME DaylightDate;
    LONG       DaylightBias;
    WCHAR      TimeZoneKeyName[128];
    BOOLEAN    DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION, 
                                 *LPDYNAMIC_TIME_ZONE_INFORMATION;

PALIMPORT 
DWORD PALAPI GetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation);

PALIMPORT
BOOL PALAPI TzSpecificLocalTimeToSystemTime(
    TIME_ZONE_INFORMATION *lpTimeZoneInformation,
    LPSYSTEMTIME            lpLocalTime,
    LPSYSTEMTIME            lpUniversalTime);

PALIMPORT
BOOL PALAPI SystemTimeToTzSpecificLocalTime(
    LPTIME_ZONE_INFORMATION lpTimeZone,
    LPSYSTEMTIME            lpUniversalTime,
    LPSYSTEMTIME            lpLocalTime);

PALIMPORT
BOOL PALAPI GetTimeZoneInformationForYear(
    USHORT                         wYear,
    LPDYNAMIC_TIME_ZONE_INFORMATION pdtzi,
    TIME_ZONE_INFORMATION        *ptzi);

#endif // RUNTIME_PLATFORM_AGNOSTIC_LINUX_TIME
