//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef __APPLE__
// todo: for BSD consider moving this file into macOS folder
#include "../Linux/DateTime.cpp"
#else
#include "Common.h"
#include "ChakraPlatform.h"
#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFTimeZone.h>

namespace PlatformAgnostic
{
namespace DateTime
{
    const WCHAR *Utility::GetStandardName(size_t *nameLength, const DateTime::YMD *ymd)
    {
        AssertMsg(ymd != NULL, "xplat needs DateTime::YMD is defined for this call");
        double tv = Js::DateUtilities::TvFromDate(ymd->year, ymd->mon, ymd->mday, ymd->time);
        int64_t absoluteTime = tv / 1000;
        absoluteTime -= kCFAbsoluteTimeIntervalSince1970;

        CFTimeZoneRef timeZone = CFTimeZoneCopySystem();

        int offset = (int)CFTimeZoneGetSecondsFromGMT(timeZone, (CFAbsoluteTime)absoluteTime);
        absoluteTime -= offset;

        char tz_name[128];
        CFStringRef abbr = CFTimeZoneCopyAbbreviation(timeZone, absoluteTime);
        CFStringGetCString(abbr, tz_name, sizeof(tz_name), kCFStringEncodingUTF16);
        wcscpy_s(data.standardName, 32, reinterpret_cast<WCHAR*>(tz_name));
        data.standardNameLength = CFStringGetLength(abbr);
        CFRelease(abbr);

        *nameLength = data.standardNameLength;
        return data.standardName;
    }

    const WCHAR *Utility::GetDaylightName(size_t *nameLength, const DateTime::YMD *ymd)
    {
        // xplat only gets the actual zone name for the given date
        return GetStandardName(nameLength, ymd);
    }

    static time_t IsDST(double tv, int *offset)
    {
        CFTimeZoneRef timeZone = CFTimeZoneCopySystem();
        int64_t absoluteTime = tv / 1000;
        absoluteTime -= kCFAbsoluteTimeIntervalSince1970;
        *offset = (int)CFTimeZoneGetSecondsFromGMT(timeZone, (CFAbsoluteTime)absoluteTime);

        return CFTimeZoneIsDaylightSavingTime(timeZone, (CFAbsoluteTime)absoluteTime);
    }

    static void YMDLocalToUtc(double localtv, YMD *utc)
    {
        int mOffset = 0;
        bool isDST = IsDST(localtv, &mOffset);
        localtv -= DateTimeTicks_PerSecond * mOffset;
        Js::DateUtilities::GetYmdFromTv(localtv, utc);
    }

    static void YMDUtcToLocal(double utctv, YMD *local,
                          int &bias, int &offset, bool &isDaylightSavings)
    {
        int mOffset = 0;
        bool isDST = IsDST(utctv, &mOffset);
        utctv += DateTimeTicks_PerSecond * mOffset;
        Js::DateUtilities::GetYmdFromTv(utctv, local);
        isDaylightSavings = isDST;
        bias = mOffset / 60;
        offset = bias;
    }

    // DaylightTimeHelper ******
    double DaylightTimeHelper::UtcToLocal(double utcTime, int &bias,
                                          int &offset, bool &isDaylightSavings)
    {
        YMD local;
        YMDUtcToLocal(utcTime, &local, bias, offset, isDaylightSavings);

        return Js::DateUtilities::TvFromDate(local.year, local.mon, local.mday, local.time);
    }

    double DaylightTimeHelper::LocalToUtc(double localTime)
    {
        YMD utc;
        YMDLocalToUtc(localTime, &utc);

        return Js::DateUtilities::TvFromDate(utc.year, utc.mon, utc.mday, utc.time);
    }
} // namespace DateTime
} // namespace PlatformAgnostic
#endif
