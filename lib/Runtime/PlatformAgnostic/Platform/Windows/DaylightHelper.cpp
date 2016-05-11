//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "Common.h"
#include <time.h>
#include "ChakraPlatform.h"

namespace PlatformAgnostic
{
namespace DateTime
{

    #define updatePeriod 1000

    // minimal year for which windows has time zone information
    static const double criticalMin = Js::DateUtilities::TvFromDate(1601, 0, 1, 0);
    static const double criticalMax = Js::DateUtilities::TvFromDate(USHRT_MAX-1, 0, 0, 0);

    typedef BOOL(*DateConversionFunction)(
        _In_opt_ CONST PVOID lpTimeZoneInformation,
        _In_ CONST SYSTEMTIME * lpLocalTime,
        _Out_ LPSYSTEMTIME lpUniversalTime
        );

    static DateConversionFunction sysLocalToUtc = NULL;
    static DateConversionFunction sysUtcToLocal = NULL;
    static HINSTANCE g_timezonedll = NULL;

    static HINSTANCE TryLoadLibrary()
    {
        if (g_timezonedll == NULL)
        {
            HMODULE hLocal = LoadLibraryExW(_u("api-ms-win-core-timezone-l1-1-0.dll"),
                                            nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (hLocal != NULL)
            {
                if (InterlockedCompareExchangePointer((PVOID*) &g_timezonedll, hLocal, NULL) != NULL)
                {
                    FreeLibrary(hLocal);
                }
            }
        }

        if (g_timezonedll == NULL)
        {
            HMODULE hLocal = LoadLibraryExW(_u("kernel32.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (hLocal != NULL)
            {
                if (InterlockedCompareExchangePointer((PVOID*) &g_timezonedll, hLocal, NULL) != NULL)
                {
                    FreeLibrary(hLocal);
                }
            }
        }
        return g_timezonedll;
    }

    BOOL SysLocalToUtc(SYSTEMTIME *local, SYSTEMTIME *utc)
    {
        if (sysLocalToUtc == NULL)
        {
            HINSTANCE library = TryLoadLibrary();
            if (library != NULL && !CONFIG_ISENABLED(Js::ForceOldDateAPIFlag))
            {
                sysLocalToUtc = (DateConversionFunction)
                                GetProcAddress(library, "TzSpecificLocalTimeToSystemTimeEx");
            }
            if (sysLocalToUtc == NULL)
            {
                sysLocalToUtc = (DateConversionFunction)TzSpecificLocalTimeToSystemTime;
            }
        }
        return sysLocalToUtc(NULL, local, utc);
    }

    BOOL SysUtcToLocal(SYSTEMTIME *utc, SYSTEMTIME *local)
    {
        if (sysUtcToLocal == NULL)
        {
            HINSTANCE library = TryLoadLibrary();
            if (library != NULL)
            {
                sysUtcToLocal = (DateConversionFunction)
                                GetProcAddress(library, "SystemTimeToTzSpecificLocalTimeEx");
            }
            if (sysUtcToLocal == NULL)
            {
                sysUtcToLocal = (DateConversionFunction)SystemTimeToTzSpecificLocalTime;
            }
        }
        return sysUtcToLocal(NULL, utc, local);
    }

    static TimeZoneInfo* GetTimeZoneInfo(DaylightTimeHelperPlatformData &data, const double time)
    {
        if (data.cache1.IsValid(time)) return &(data.cache1);
        if (data.cache2.IsValid(time)) return &(data.cache2);

        if (data.useFirstCache)
        {
            data.cache1.Update(time);
            data.useFirstCache = false;
            return &(data.cache1);
        }
        else
        {
            data.cache2.Update(time);
            data.useFirstCache = true;
            return &(data.cache2);
        }
    }

    // we consider January 1st, December 31st and days when daylight savings time
    // starts and ands to be critical, because there might be ambiguous cases in
    // local->utc->local conversions, so in order to be consistent with Windows
    // we rely on it to perform conversions. But it is slow.
    static inline bool IsCritical(const double time, TimeZoneInfo *timeZoneInfo)
    {
        return time > criticalMin && time < criticalMax &&
            (fabs(time - timeZoneInfo->daylightDate) < DateTimeTicks_PerLargestTZOffset ||
            fabs(time - timeZoneInfo->standardDate) < DateTimeTicks_PerLargestTZOffset ||
            time > timeZoneInfo->january1 + DateTimeTicks_PerSafeEndOfYear ||
            (timeZoneInfo->isJanuary1Critical
              && time - timeZoneInfo->january1 < DateTimeTicks_PerLargestTZOffset));
    }

    // in slow path we use system API to perform conversion, but we still need
    // to know whether current time is standard or daylight savings in order to
    // create a string representation of a date. So just compare whether difference
    // between local and utc time equal to bias.
    static inline bool IsDaylightSavings(const double utcTime, const double localTime, const int bias)
    {
        return ((int)(utcTime - localTime)) / ((int)(DateTimeTicks_PerMinute)) != bias;
    }


    // This function does not properly handle boundary cases.
    // But while we use IsCritical we don't care about it.
    static bool IsDaylightSavingsUnsafe(const double time, TimeZoneInfo *timeZoneInfo)
    {
        return timeZoneInfo->isDaylightTimeApplicable
                && ((timeZoneInfo->daylightDate < timeZoneInfo->standardDate)
            ? timeZoneInfo->daylightDate <= time && time < timeZoneInfo->standardDate
            : time < timeZoneInfo->standardDate || timeZoneInfo->daylightDate <= time);
    }

    double UtcToLocalFast(const double utcTime, TimeZoneInfo *timeZoneInfo, int &bias,
                          int &offset, bool &isDaylightSavings)
    {
        double localTime;
        localTime = utcTime - DateTimeTicks_PerMinute * timeZoneInfo->bias;
        isDaylightSavings = IsDaylightSavingsUnsafe(utcTime, timeZoneInfo);
        if (isDaylightSavings)
        {
            localTime -= DateTimeTicks_PerMinute * timeZoneInfo->daylightBias;
        } else {
            localTime -= DateTimeTicks_PerMinute * timeZoneInfo->standardBias;
        }

        bias = timeZoneInfo->bias;
        offset = ((int)(localTime - utcTime)) / ((int)(DateTimeTicks_PerMinute));

        return localTime;
    }

    static double UtcToLocalCritical(DaylightTimeHelperPlatformData &data, const double utcTime,
                              TimeZoneInfo *timeZoneInfo, int &bias, int &offset, bool &isDaylightSavings)
    {
        double localTime;
        SYSTEMTIME utcSystem, localSystem;
        YMD ymd;

        Js::DateUtilities::GetYmdFromTv(utcTime, &ymd);
        ymd.ToSystemTime(&utcSystem);

        if (!SysUtcToLocal(&utcSystem, &localSystem))
        {
            // SysUtcToLocal can fail if the date is beyond extreme internal
            // boundaries (e.g. > ~30000 years). Fall back to our fast (but
            // less accurate) version if the call fails.
            return UtcToLocalFast(utcTime, timeZoneInfo, bias, offset, isDaylightSavings);
        }

        localTime = Js::DateUtilities::TimeFromSt(&localSystem);
        if (localSystem.wYear != utcSystem.wYear)
        {
            timeZoneInfo = GetTimeZoneInfo(data, localTime);
        }

        bias = timeZoneInfo->bias;
        isDaylightSavings = IsDaylightSavings(utcTime, localTime,
          timeZoneInfo->bias + timeZoneInfo->standardBias);

        offset = ((int)(localTime - utcTime)) / ((int)(DateTimeTicks_PerMinute));

        return localTime;
    }

    double DaylightTimeHelper::UtcToLocal(const double utcTime, int &bias, int &offset, bool &isDaylightSavings)
    {
        TimeZoneInfo *timeZoneInfo = GetTimeZoneInfo(data, utcTime);

        if (IsCritical(utcTime, timeZoneInfo))
        {
            return UtcToLocalCritical(data, utcTime, timeZoneInfo, bias, offset, isDaylightSavings);
        }
        else
        {
            return UtcToLocalFast(utcTime, timeZoneInfo, bias, offset, isDaylightSavings);
        }
    }

    static double LocalToUtcFast(const double localTime, TimeZoneInfo *timeZoneInfo)
    {
        double utcTime = localTime + DateTimeTicks_PerMinute * timeZoneInfo->bias;
        bool isDaylightSavings = IsDaylightSavingsUnsafe(localTime, timeZoneInfo);

        if (isDaylightSavings)
        {
            utcTime += DateTimeTicks_PerMinute * timeZoneInfo->daylightBias;
        } else {
            utcTime += DateTimeTicks_PerMinute * timeZoneInfo->standardBias;
        }

        return utcTime;
    }

    static double LocalToUtcCritical(const double localTime, TimeZoneInfo *timeZoneInfo)
    {
        SYSTEMTIME localSystem, utcSystem;
        YMD ymd;

        Js::DateUtilities::GetYmdFromTv(localTime, &ymd);
        ymd.ToSystemTime(&localSystem);

        if (!SysLocalToUtc(&localSystem, &utcSystem))
        {
            // Fall back to our fast (but less accurate) version if the call fails.
            return LocalToUtcFast(localTime, timeZoneInfo);
        }

        return Js::DateUtilities::TimeFromSt(&utcSystem);
    }

    double DaylightTimeHelper::LocalToUtc(const double localTime)
    {
        TimeZoneInfo *timeZoneInfo = GetTimeZoneInfo(data, localTime);

        if (IsCritical(localTime, timeZoneInfo))
        {
            return LocalToUtcCritical(localTime, timeZoneInfo);
        }
        else
        {
            return LocalToUtcFast(localTime, timeZoneInfo);
        }
    }

} // namespace DateTime
} // namespace PlatformAgnostic
