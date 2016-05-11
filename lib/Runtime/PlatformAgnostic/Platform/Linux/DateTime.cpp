//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Runtime.h"
#include <time.h>
#include <sys/time.h>
#include "ChakraPlatform.h"

namespace PlatformAgnostic
{
namespace DateTime
{

    #define updatePeriod 1000

    const WCHAR *Utility::GetStandardName(size_t *nameLength) {
        data.UpdateTimeZoneInfo();
        *nameLength = data.standardZoneNameLength;
        return data.standardZoneName;
    }

    const WCHAR *Utility::GetDaylightName(size_t *nameLength) {
        // We have an abbreviated standard or daylight name
        // based on the date and time zone we are in.
        return GetStandardName(nameLength);
    }

    void UtilityPlatformData::UpdateTimeZoneInfo()
    {
        uint32 tickCount = GetTickCount();
        if (tickCount - lastTimeZoneUpdateTickCount > updatePeriod)
        {
            time_t gtime = time(NULL);
            struct tm ltime;

            tzset();
            localtime_r(&gtime, &ltime);

            standardZoneNameLength = strlen(ltime.tm_zone);

        #if defined(WCHAR_IS_CHAR16_T)
            for(int i = 0; i < standardZoneNameLength; i++) {
                standardZoneName[i] = (char16_t)ltime.tm_zone[i];
            }
        #elif defined(WCHAR_IS_WCHAR_T)
            mbstowcs( (wchar_t*) standardZoneName, ltime.tm_zone,
                      sizeof(standardZoneName) / sizeof(WCHAR));
        #else
        #error "WCHAR should be either wchar_t or char16_t"
        #endif

            standardZoneName[standardZoneNameLength] = (WCHAR)0;
            lastTimeZoneUpdateTickCount = tickCount;
        }
    }

    // DateTimeHelper ******
    static inline void TM_TO_SYSTIME(struct tm *time, SYSTEMTIME *sysTime)
    {
        sysTime->wYear = time->tm_year + 1900;
        sysTime->wMonth = time->tm_mon + 1;
        sysTime->wDayOfWeek = time->tm_wday;
        sysTime->wDay = time->tm_mday;
        sysTime->wHour = time->tm_hour;
        sysTime->wMinute = time->tm_min;

        // C99 tm_sec value is between 0 and 60. (1 leap second.)
        // Discard leap second.
        // In any case, this is a representation of a wallclock time.
        // It can jump forwards and backwards.

        sysTime->wSecond = time->tm_sec % 60;
    }

    #define MIN_ZERO(value) value < 0 ? 0 : value

    static inline void SYSTIME_TO_TM(SYSTEMTIME *sysTime, struct tm *time)
    {
        time->tm_year = MIN_ZERO(sysTime->wYear - 1900);
        time->tm_mon = MIN_ZERO(sysTime->wMonth - 1);
        time->tm_wday = sysTime->wDayOfWeek;
        time->tm_mday = sysTime->wDay;
        time->tm_hour = sysTime->wHour;
        time->tm_min = sysTime->wMinute;
        time->tm_sec = sysTime->wSecond;
    }

    static void SysLocalToUtc(SYSTEMTIME *local, SYSTEMTIME *utc)
    {
        struct tm local_tm;
        SYSTIME_TO_TM(local, (&local_tm));

        // tm doesn't have milliseconds

        tzset();
        time_t utime = timegm(&local_tm);
        mktime(&local_tm); // we just want the gmt_off, don't care the result
        utime -= local_tm.tm_gmtoff; // reverse UTC

        struct tm utc_tm;
        if (gmtime_r(&utime, &utc_tm) == 0)
        {
            AssertMsg(false, "gmtime() failed");
        }

        TM_TO_SYSTIME((&utc_tm), utc);
        // put milliseconds back
        utc->wMilliseconds = local->wMilliseconds;
    }

    static void SysUtcToLocal(SYSTEMTIME *utc, SYSTEMTIME *local,
                          int &bias, int &offset, bool &isDaylightSavings)
    {
        struct tm utc_tm;
        SYSTIME_TO_TM(utc, (&utc_tm));

        // tm doesn't have milliseconds

        tzset();
        time_t ltime = timegm(&utc_tm);
        struct tm local_tm;
        localtime_r(&ltime, &local_tm);
        offset = local_tm.tm_gmtoff / 60;

        TM_TO_SYSTIME((&local_tm), local);

        // put milliseconds back
        local->wMilliseconds = utc->wMilliseconds;

        isDaylightSavings = local_tm.tm_isdst;
        bias = offset;
    }

    // DaylightTimeHelper ******
    double DaylightTimeHelper::UtcToLocal(double utcTime, int &bias,
                                          int &offset, bool &isDaylightSavings)
    {
        SYSTEMTIME utcSystem, localSystem;
        YMD ymd;

        // todo: can we make all these transformation more efficient?
        Js::DateUtilities::GetYmdFromTv(utcTime, &ymd);
        ymd.ToSystemTime(&utcSystem);

        SysUtcToLocal(&utcSystem, &localSystem,
                      bias, offset, isDaylightSavings);

        return Js::DateUtilities::TimeFromSt(&localSystem);
    }

    double DaylightTimeHelper::LocalToUtc(double localTime)
    {
        SYSTEMTIME utcSystem, localSystem;
        YMD ymd;

        // todo: can we make all these transformation more efficient?
        Js::DateUtilities::GetYmdFromTv(localTime, &ymd);
        ymd.ToSystemTime(&utcSystem);

        SysLocalToUtc(&utcSystem, &localSystem);

        return Js::DateUtilities::TimeFromSt(&localSystem);
    }
} // namespace DateTime
} // namespace PlatformAgnostic
