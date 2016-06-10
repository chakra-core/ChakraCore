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

    // This method is expected to return UTC time (See MSDN GetSystemTime)
    double HiResTimer::GetSystemTime()
    {
// todo: remove/move this when we have BSD/UNIX folder
// todo-osx: mach_absolute_time
#ifndef __APPLE__
        struct timespec fast_time;
        // method below returns UTC time. So, nothing else is needed
        // we use clock_gettime first due to expectation of better accuracy
        if (clock_gettime(CLOCK_REALTIME, &fast_time) == 0)
        {
            return (fast_time.tv_sec * DateTimeTicks_PerSecond)
                   + (int32_t) (fast_time.tv_nsec / 1e6);
        }
#endif

        // in case of clock_gettime fails we use the implementation below
        struct tm utc_tm;
        struct timeval timeval;
        time_t time_now = time(NULL);

        // gettimeofday return value needs to be converted to UTC time
        // see call below for gmtime_r
        int timeofday_retval = gettimeofday(&timeval,NULL);

        if (gmtime_r(&time_now, &utc_tm) == NULL)
        {
            AssertMsg(false, "gmtime() failed");
        }

        size_t milliseconds = 0;
        if(timeofday_retval != -1)
        {
            milliseconds = timeval.tv_usec / DateTimeTicks_PerSecond;

            int old_sec = utc_tm.tm_sec;
            int new_sec = timeval.tv_sec % 60; // C99 0-60 (1sec leap)

            // just in case we reached the next
            // second in the interval between time() and gettimeofday()
            if(old_sec != new_sec)
            {
                milliseconds = 999;
            }  
        }                      

        milliseconds = (utc_tm.tm_hour * DateTimeTicks_PerHour)
                        + (utc_tm.tm_min * DateTimeTicks_PerMinute)
                        + (utc_tm.tm_sec * DateTimeTicks_PerSecond)
                        + milliseconds;

        return Js::DateUtilities::TvFromDate(1900 + utc_tm.tm_year, utc_tm.tm_mon, 
                                            utc_tm.tm_mday - 1, milliseconds);
    }

    double HiResTimer::Now()
    {
        return GetSystemTime();
    }

} // namespace DateTime
} // namespace PlatformAgnostic
