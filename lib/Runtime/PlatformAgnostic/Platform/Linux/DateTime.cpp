//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Common.h"
#include <time.h>
#include <sys/time.h>
#include "ChakraPlatform.h"

namespace PlatformAgnostic
{
namespace DateTime
{

    static inline bool IsLeap(const int year)
    {
        return (0 == (year & 3)) && (0 != (year % 100) || 0 == (year % 400));
    }

    // Windows DateTime implementation normalizes the year beyond <1900 >2100
    // mktime etc. broken-out time bases 1900
    static inline int NormalizeYMDYear(const int base_year)
    {
        int retval = base_year;

        if (base_year < -2100)
        {
            retval = 2100;
        }
        else if (base_year < -1900)
        {
            retval = base_year * -1;
        }
        else if (base_year >= 0 && base_year < 100)
        {
            retval = 1900 + base_year;
        }
        else if (base_year < 0)
        {
            retval = NormalizeYMDYear(-1 * base_year);
        }

        return retval;
    }

    static inline int UpdateToYMDYear(const int base_year, const struct tm *time)
    {
        int year = time->tm_year;

        if (base_year < -2100)
        {
            const int diff = year - 2100;
            year = abs(base_year) - diff;
        }

        if (base_year < -1900)
        {
            year *= -1;
        }
        else if (base_year >= 0 && base_year < 100)
        {
            year -= 1900;
        }
        else if (base_year < 0)
        {
            const int org_base_year = NormalizeYMDYear(-1 * base_year);
            year = base_year - (org_base_year - year);
        }

        return year;
    }

    static void YMD_TO_TM(const YMD *ymd, struct tm *time, bool *leap_added)
    {
        time->tm_year = NormalizeYMDYear(ymd->year);
        time->tm_mon = ymd->mon;
        time->tm_wday = ymd->wday;
        time->tm_mday = ymd->mday;
        int t = ymd->time;
        t /= DateTimeTicks_PerSecond; // discard ms
        time->tm_sec = t % 60;
        t /= 60;
        time->tm_min = t % 60;
        t /= 60;
        time->tm_hour = t;

        // mktime etc. broken-out time accepts 1900 as a start year while epoch is 1970
        // temporarily add a calendar day for leap pass
        bool leap_year = IsLeap(time->tm_year);
        *leap_added = false;
        if (ymd->yday == 60 && leap_year) {
            time->tm_mday++;
            *leap_added = true;
        }
    }

    static void TM_TO_YMD(const struct tm *time, YMD *ymd, const bool leap_added, const int base_year)
    {
        ymd->year = UpdateToYMDYear(base_year, time);
        ymd->mon = time->tm_mon;
        ymd->mday = time->tm_mday;

        ymd->time = (time->tm_hour * DateTimeTicks_PerHour)
                  + (time->tm_min * DateTimeTicks_PerMinute)
                  + (time->tm_sec * DateTimeTicks_PerSecond);

        // mktime etc. broken-out time accepts 1900 as a start year while epoch is 1970
        // minus the previously added calendar day (see YMD_TO_TM)
        if (leap_added)
        {
            AssertMsg(ymd->mday >= 0, "Day of month can't be a negative number");
            if (ymd->mday == 0)
            {
                ymd->mday = 29;
                ymd->mon = 1;
            }
            else
            {
                ymd->mday--;
            }
        }
    }

    static void CopyTimeZoneName(WCHAR *wstr, size_t *length, const char *tm_zone)
    {
        *length = strlen(tm_zone);

        for(int i = 0; i < *length; i++)
        {
            wstr[i] = (WCHAR)tm_zone[i];
        }

        wstr[*length] = (WCHAR)0;
    }

    const WCHAR *Utility::GetStandardName(size_t *nameLength, const DateTime::YMD *ymd)
    {
        AssertMsg(ymd != NULL, "xplat needs DateTime::YMD is defined for this call");
        struct tm time_tm;
        bool leap_added;
        YMD_TO_TM(ymd, &time_tm, &leap_added);
        mktime(&time_tm); // get zone name for the given date
        CopyTimeZoneName(data.standardName, &data.standardNameLength, time_tm.tm_zone);
        *nameLength = data.standardNameLength;
        return data.standardName;
    }

    const WCHAR *Utility::GetDaylightName(size_t *nameLength, const DateTime::YMD *ymd)
    {
        // xplat only gets the actual zone name for the given date
        return GetStandardName(nameLength, ymd);
    }

    static void YMDLocalToUtc(YMD *local, YMD *utc)
    {
        struct tm local_tm;
        bool leap_added;
        YMD_TO_TM(local, (&local_tm), &leap_added);

        // tm doesn't have milliseconds
        int milliseconds = local->time % 1000;

        tzset();
        time_t utime = timegm(&local_tm);

        // we alter the original date
        // and mktime doesn't know that
        // so calculate gmtoff manually and keep dst from being included
        mktime(&local_tm);
        utime -= local_tm.tm_gmtoff;

        struct tm utc_tm;
        if (gmtime_r(&utime, &utc_tm) == 0)
        {
            AssertMsg(false, "gmtime() failed");
        }

        TM_TO_YMD((&utc_tm), utc, leap_added, local->year);
        // put milliseconds back
        utc->time += milliseconds;
    }

    static void YMDUtcToLocal(YMD *utc, YMD *local,
                          int &bias, int &offset, bool &isDaylightSavings)
    {
        struct tm utc_tm;
        bool leap_added;
        YMD_TO_TM(utc, &utc_tm, &leap_added);

        // tm doesn't have milliseconds
        int milliseconds = utc->time % 1000;

        tzset();
        time_t ltime = timegm(&utc_tm);
        struct tm local_tm;
        localtime_r(&ltime, &local_tm);

        TM_TO_YMD((&local_tm), local, leap_added, utc->year);
        // put milliseconds back
        local->time += milliseconds;

        // ugly hack but;
        // right in between dst pass
        // we need mktime trick to get the correct dst
        utc_tm.tm_isdst = 1;
        ltime = mktime(&utc_tm);
        ltime += utc_tm.tm_gmtoff;
        localtime_r(&ltime, &utc_tm);

        isDaylightSavings = utc_tm.tm_isdst;
        offset = utc_tm.tm_gmtoff / 60;
        bias = offset;
    }

    // DaylightTimeHelper ******
    double DaylightTimeHelper::UtcToLocal(double utcTime, int &bias,
                                          int &offset, bool &isDaylightSavings)
    {
        YMD ymdUTC, local;

        Js::DateUtilities::GetYmdFromTv(utcTime, &ymdUTC);
        YMDUtcToLocal(&ymdUTC, &local,
                      bias, offset, isDaylightSavings);

        return Js::DateUtilities::TvFromDate(local.year, local.mon, local.mday, local.time);
    }

    double DaylightTimeHelper::LocalToUtc(double localTime)
    {
        YMD ymdLocal, utc;

        Js::DateUtilities::GetYmdFromTv(localTime, &ymdLocal);
        YMDLocalToUtc(&ymdLocal, &utc);

        return Js::DateUtilities::TvFromDate(utc.year, utc.mon, utc.mday, utc.time);
    }
} // namespace DateTime
} // namespace PlatformAgnostic
