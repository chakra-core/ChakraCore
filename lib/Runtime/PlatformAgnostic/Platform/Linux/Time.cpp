//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Time.h"

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sched.h>

#if HAVE_MACH_ABSOLUTE_TIME
#include <mach/mach_time.h>
static mach_timebase_info_data_t s_TimebaseInfo;
#endif

static void GetTZI(LPTIME_ZONE_INFORMATION tzi) noexcept 
{
    time_t gtime = time(NULL);
    struct tm *ltime = gmtime(&gtime);
    time_t mtime = mktime(ltime);

    tzi->StandardBias = tzi->Bias = (int64_t)(mtime - gtime) / 60;

    tzset();
    localtime_r(&gtime, ltime);

    unsigned len = strlen(ltime->tm_zone);

#if defined(WCHAR_IS_CHAR16_T)
    for(int i=0; i<len; i++) {
        tzi->StandardName[i] = (char16_t)ltime->tm_zone[i];
    }
#elif defined(WCHAR_IS_WCHAR_T)
    mbstowcs( (wchar_t*) tzi->StandardName, ltime->tm_zone, 
              sizeof(tzi->StandardName) / sizeof(WCHAR));
#else
#error "WCHAR should be either wchar_t or char16_t"
#endif

    tzi->StandardName[len] = (WCHAR)0;
}

PALIMPORT
BOOL PALAPI TzSpecificLocalTimeToSystemTime(
    TIME_ZONE_INFORMATION *lpTimeZoneInformation,
    LPSYSTEMTIME            lpLocalTime,
    LPSYSTEMTIME            lpUniversalTime)
{
    // xplat-todo: implement this! (TzSpecificLocalTimeToSystemTime)
    return FALSE;
}

PALIMPORT
BOOL PALAPI SystemTimeToTzSpecificLocalTime(
    LPTIME_ZONE_INFORMATION lpTimeZone,
    LPSYSTEMTIME            lpUniversalTime,
    LPSYSTEMTIME            lpLocalTime)
{
    // xplat-todo: implement this! (SystemTimeToTzSpecificLocalTime)
    return FALSE;
}

PALIMPORT
BOOL PALAPI GetTimeZoneInformationForYear(
    USHORT                         wYear,
    LPDYNAMIC_TIME_ZONE_INFORMATION pdtzi,
    LPTIME_ZONE_INFORMATION        ptzi)
{
    // xplat-todo: implement this! (GetTimeZoneInformationForYear)
    GetTZI(ptzi);

    GetSystemTime(&ptzi->StandardDate);
    
    return TRUE;
}

PALIMPORT 
DWORD PALAPI GetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation)
{
    // xplat-todo: implement this! (GetTimeZoneInformation)
    GetTZI(lpTimeZoneInformation);
    
    GetSystemTime(&lpTimeZoneInformation->StandardDate);

    return 1; // TIME_ZONE_ID_STANDARD
}
