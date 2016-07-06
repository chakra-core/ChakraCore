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

    static const int daysInMonthLeap[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // The day numbers for the months of a leap year.
    static const int daysUpToMonthLeap[12] =
    {
          0,  31,  60,  91, 121, 152,
        182, 213, 244, 274, 305, 335,
    };

    static int DayNumber(int yearType, const SYSTEMTIME &date)
    {
        if (date.wYear == 0)
        {
            BOOL isLeap = yearType / 7; // yearType is a day of week of January 1st (number within range [0,6]) (+ 7 if year is a leap)
            int dayOfWeekOf1stOfMonth = (yearType + daysUpToMonthLeap[date.wMonth-1] - (int)(!isLeap && date.wMonth >= 3)) % 7;
            int numberOfDaysInThisMonth = daysInMonthLeap[date.wMonth-1] - (int)(!isLeap && date.wMonth == 2);
            int delta = date.wDayOfWeek - dayOfWeekOf1stOfMonth;
            return min((numberOfDaysInThisMonth - delta - 1) / 7, date.wDay - (int)(delta >= 0)) * 7 + delta + 1;
        }
        else
        {
            return date.wDay;
        }
    }

    // class Utilility ******
    void UtilityPlatformData::UpdateTimeZoneInfo()
    {
        uint32 tickCount = GetTickCount();
        if (tickCount - lastTimeZoneUpdateTickCount > updatePeriod)
        {
            GetTimeZoneInformation(&timeZoneInfo);

            // todo: check possible winrt issue
            // !defined(__cplusplus_winrt)
            // see https://msdn.microsoft.com/en-us/library/90s5c885.aspx
    #if defined(_WIN32)
            _tzset();
    #endif
            lastTimeZoneUpdateTickCount = tickCount;
        }
    }

    const WCHAR *Utility::GetStandardName(size_t *nameLength, const DateTime::YMD *_ /*caution! can be NULL. not used for Windows*/)
    {
        data.UpdateTimeZoneInfo();
        *nameLength = wcslen(data.timeZoneInfo.StandardName);
        return data.timeZoneInfo.StandardName;
    }

    const WCHAR *Utility::GetDaylightName(size_t *nameLength, const DateTime::YMD *_/*caution! can be NULL. not used for Windows*/)
    {
        data.UpdateTimeZoneInfo();
        *nameLength = wcslen(data.timeZoneInfo.DaylightName);
        return data.timeZoneInfo.DaylightName;
    }

    // class TimeZoneInfo ******

    // Cache should be invalid at the moment of creation
    // if january1 > nextJanuary1 cache is always invalid, so we don't care about other fields, because cache will be updated.
    TimeZoneInfo::TimeZoneInfo()
    {
        january1 = 1;
        nextJanuary1 = 0;
    }

    // Cache is valid for given time if this time is within a year for which cache was created, and cache was updated within 1 second of current moment
    bool TimeZoneInfo::IsValid(const double time)
    {
        return GetTickCount() - lastUpdateTickCount < updatePeriod && time >= january1 && time < nextJanuary1;
    }

    void TimeZoneInfo::Update(const double inputTime)
    {
        int year, yearType;
        Js::DateUtilities::GetYearFromTv(inputTime, year, yearType);
        int yearForInfo = year;

        // GetTimeZoneInformationForYear() works only with years > 1600, but JS works with wider range of years. So we take year closest to given.
        if (year < 1601)
        {
            yearForInfo = 1601;
        }
        else if (year > 2100)
        {
            yearForInfo = 2100;
        }
        TIME_ZONE_INFORMATION timeZoneInfo;
        if (GetTimeZoneInformationForYear((USHORT)yearForInfo, NULL, &timeZoneInfo))
        {
            isDaylightTimeApplicable = timeZoneInfo.StandardDate.wMonth != 0 && timeZoneInfo.DaylightDate.wMonth != 0;

            bias = timeZoneInfo.Bias;
            daylightBias = timeZoneInfo.DaylightBias;
            standardBias = timeZoneInfo.StandardBias;

            double day = DayNumber(yearType, timeZoneInfo.DaylightDate);
            double time = Js::DateUtilities::DayTimeFromSt(&timeZoneInfo.DaylightDate);
            daylightDate = Js::DateUtilities::TvFromDate(year, timeZoneInfo.DaylightDate.wMonth-1, day-1, time);

            day = DayNumber(yearType, timeZoneInfo.StandardDate);
            time = Js::DateUtilities::DayTimeFromSt(&timeZoneInfo.StandardDate);
            standardDate = Js::DateUtilities::TvFromDate(year, timeZoneInfo.StandardDate.wMonth-1, day-1, time);

            if (GetTimeZoneInformationForYear((USHORT)yearForInfo-1, NULL, &timeZoneInfo))
            {
                isJanuary1Critical = timeZoneInfo.Bias + timeZoneInfo.DaylightBias + timeZoneInfo.StandardBias != bias + daylightBias + standardBias;

                january1 = Js::DateUtilities::TvFromDate(year, 0, 0, 0);
                nextJanuary1 = january1 + DateTimeTicks_PerNonLeapYear + Js::DateUtilities::FLeap(year) * DateTimeTicks_PerDay;
                lastUpdateTickCount = GetTickCount();
            }
        }
    }

} // namespace DateTime
} // namespace PlatformAgnostic
