//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "Common.h"
#include "ChakraPlatform.h"
#include <Bcrypt.h>

namespace PlatformAgnostic
{
namespace DateTime
{
    // Quantization code adapted from the version in Edge
    template<uint64 frequencyOfQuantization>
    class JitterManager
    {
        double frequencyOfSampling = 1.0;
        double quantizationToSelectedScaleFactor = 1.0;
        bool highFreq = false;
        double currentRandomWindowScaled = 0.0;
        ULONGLONG currentQuantizedQpc = 0;
    public:
        JitterManager()
        {
            // NOTE: We could cache the (1000/frequency) operation, as a double,
            // that is used later to convert from seconds to milliseconds so that
            // we don't keep recomputing it every time we need to convert from QPC
            // to milliseconds (high-resolution only).  However, doing so would mean
            // we have to worry about loss of precision that occurs through rounding
            // and its propagation through any arithmetic operations subsequent to
            // the conversion. Such loss of precision isn't necessarily significant,
            // since the time values returned from CTimeManager aren't expected to be
            // used in more than 1 or 2 subsequent arithmetic operations. The other
            // potential source of precision loss occurs when a floating point value
            // gets converted from a normal form to a denormal form, which only happens
            // when trying to store a number smaller than 2^(-126), and we should never
            // see a frequency big enough to cause that.  For example, worst case we would
            // need a processor frequency of (1000/(2^(-126))=8.50705917*10^(31) GHz
            // to go denormal.
            LARGE_INTEGER tempFreq;
            highFreq = QueryPerformanceFrequency(&tempFreq);
            if (!highFreq)
            {
                // If we don't have a high-frequency source, the best we can do is GetSystemTime,
                // which has a 1ms frequency.
                frequencyOfSampling = 1000;
            }
            else
            {
                frequencyOfSampling = (double)tempFreq.QuadPart;
            }

            // frequency.QuadPart is the frequency of the QPC in counts per second.  For quantinization,
            // we want to scale the QPC to units of counts per selected frequency.  We calculate the scale
            // factor now:
            //
            //      e.g. 500us = 2000 counts per second
            //
            quantizationToSelectedScaleFactor = frequencyOfSampling / frequencyOfQuantization;

            // If the scale factor is less than one, it means the hardware's QPC frequency is already
            // the selected frequency or greater. In this case, we clamp to 1. This makes the arithmetic
            // in QuantizeToFreq a no-op, which avoids losing further precision.
            //
            // We clamp to 1 rather than use an early exit to avoid untested blocks.
            quantizationToSelectedScaleFactor = max(quantizationToSelectedScaleFactor, 1.0);
        }

        uint64 QuantizedQPC(uint64 qpc)
        {
            // Due to further analysis of some attacks, we're jittering on a more granular
            // frequency of as much as a full millisecond.

            // 'qpc' is currently in units of frequencyOfSampling counts per second.  We want to
            // convert to units of frequencyOfQuantization, where sub-frequencyOfQuantization precision
            // is expressed via the fractional part of a floating point number.
            //
            // We perform the conversion now, using the scale factor we precalculated in the
            // constructor.
            double preciseScaledResult = qpc / quantizationToSelectedScaleFactor;

            // We now remove the fractional components, quantizing precision to the selected frequency by both ceiling and floor.
            double ceilResult = ceil(preciseScaledResult);
            double floorResult = floor(preciseScaledResult);

            // Convert the results back to original QPC units, now at selected precision.
            ceilResult *= quantizationToSelectedScaleFactor;
            floorResult *= quantizationToSelectedScaleFactor;

            // Convert these back to an integral value, taking the ceiling, even for the floored result.
            // This will give us consistent results as the quantum moves (i.e. what is currently the
            // quantizedQPC ends up being the floored QPC once we roll into the next window).
            ULONGLONG quantizedQpc = static_cast<ULONGLONG>(ceil(ceilResult));
            ULONGLONG quantizedQpcFloored = static_cast<ULONGLONG>(ceil(floorResult));

            // The below converts the delta to milliseconds and checks that our quantized value does not
            // diverge by more than our target quantization (plus an epsilon equal to 1 tick of the QPC).
            AssertMsg(((quantizedQpc - qpc) * 1000.0 / frequencyOfSampling) <= (1000.0 / frequencyOfQuantization) + (1000.0 / frequencyOfSampling),
                "W3C Timing: Difference between 'qpc' and 'quantizedQpc' expected to be <= selected frequency + epsilon.");

            // If we're seeing this particular quantum for the first time, calculate a random portion of
            // the beginning of the quantum in which we'll floor (and continue to report the previous quantum)
            // instead of snapping to the next quantum.
            // We don't do any of this quantiziation randomness if the scale factor is 1 (presumably because the
            // QPC resolution was less than our selected quantum).
            if (quantizationToSelectedScaleFactor != 1.0 && quantizedQpc != currentQuantizedQpc)
            {
                BYTE data[1];
                NTSTATUS status = BCryptGenRandom(nullptr, data, sizeof(data), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
                AssertOrFailFast(status == 0);
                //Release_Assert(status == 0); IE does not have Release_Assert, but Chakra does.
                if (BCRYPT_SUCCESS(status))
                {
                    // Convert the random byte to a double in the range [0.0, 1.0)
                    // Note: if this ends up becoming performance critical, we can substitute the calculation with a
                    // 2K lookup table (256 * 8) bytes.
                    const double VALUES_IN_BYTE = 256.0;
                    const double random0to1 = ((double)data[0] / VALUES_IN_BYTE);

                    currentRandomWindowScaled = random0to1;
                }
                else
                {
                    currentRandomWindowScaled = (double)(rand() % 256) / 256.0;
                }

                // Record the fact that we've already generated the random range for this particular quantum.
                // Note that this is not the reported one, but the actual quantum as calculated from the QPC.
                currentQuantizedQpc = quantizedQpc;
            }

            // Grab off the fractional portion of the preciseScaledResult. As an example:
            //   QueryPerformanceFrequency is 1,000,000
            //   This means a QPC is 1us.
            //   Quantum of 1000us means a QPC of 125 would result in a fractional portion of 0.125
            //   (math works out to (125 % 1000) / 1000).
            // This fractional portion is then evaluated in order to determine whether or not to snap
            // forward and use the next quantum, or use the floored one. This calculation gives us random
            // windows in which specific 1000us timestamps are observed for a non-deterministic amount of time.
            double preciseScaledFractional = preciseScaledResult - ((ULONGLONG)preciseScaledResult);
            if (preciseScaledFractional < currentRandomWindowScaled)
            {
                quantizedQpc = quantizedQpcFloored;
            }

            return quantizedQpc;
        }
    };

    // Set to jitter to an accuracy of 1000 ticks/second or 1ms
    static thread_local JitterManager<1000> timeJitter;

    double GetQuantizedSystemTime()
    {
        SYSTEMTIME stTime;
        ::GetSystemTime(&stTime);
        // In the event that we have no high-res timers available, we don't have the needed
        // granularity to jitter at a 1ms precision. We need to do something (as otherwise,
        // the timer precision is higher than on a high-precision machine), but the best we
        // can do is likely to strongly reduce the timer accuracy. Here we group by 4ms.
        stTime.wMilliseconds = (stTime.wMilliseconds / 4) * 4;
        return Js::DateUtilities::TimeFromSt(&stTime);
    }

    double HiResTimer::GetSystemTime()
    {
        SYSTEMTIME stTime;
        ::GetSystemTime(&stTime);
        return Js::DateUtilities::TimeFromSt(&stTime);
    }

    // determine if the system time is being adjusted every tick to gradually
    // bring it inline with a time server.
    static double GetAdjustFactor()
    {
        DWORD dwTimeAdjustment = 0;
        DWORD dwTimeIncrement = 0;
        BOOL fAdjustmentDisabled = FALSE;
        BOOL fSuccess = GetSystemTimeAdjustment(&dwTimeAdjustment, &dwTimeIncrement, &fAdjustmentDisabled);
        if (!fSuccess || fAdjustmentDisabled)
        {
            return 1;
        }
        return ((double)dwTimeAdjustment) / ((double)dwTimeIncrement);
    }

    double HiResTimer::Now()
    {
        if(!data.fHiResAvailable)
        {
            return GetQuantizedSystemTime();
        }

        if(!data.fInit)
        {
            if (!QueryPerformanceFrequency((LARGE_INTEGER *) &(data.freq)))
            {
                data.fHiResAvailable = false;
                return GetQuantizedSystemTime();
            }
            data.fInit = true;
        }

#if DBG
        uint64 f;
        Assert(QueryPerformanceFrequency((LARGE_INTEGER *)&f) && f == data.freq);
#endif
        // try better resolution time using perf counters
        uint64 count;
        if( !QueryPerformanceCounter((LARGE_INTEGER *) &count))
        {
            data.fHiResAvailable = false;
            return GetQuantizedSystemTime();
        }

        double time = GetSystemTime();

        // quantize the timestamp to hinder timing attacks
        count = timeJitter.QuantizedQPC(count);

        // there is a base time and count set.
        if (!data.fReset
            && (count >= data.baseMsCount)) // Make sure we don't regress
        {
            double elapsed = ((double)(count - data.baseMsCount)) * 1000 / data.freq;

            // if the system time is being adjusted every tick, adjust the
            // precise time delta accordingly.
            if (data.dAdjustFactor != 1)
            {
                elapsed = elapsed * data.dAdjustFactor;
            }

            double preciseTime = data.dBaseTime + elapsed;

            if (fabs(preciseTime - time) < 25 // the time computed via perf counter is off by 25ms
                && preciseTime >= data.dLastTime)  // the time computed via perf counter is running backwards
            {
                data.dLastTime = preciseTime;
                return data.dLastTime;
            }
        }

        //reset
        data.dBaseTime = time;
        data.dAdjustFactor = GetAdjustFactor();
        data.baseMsCount = count;

        double dSinceLast = time - data.dLastTime;
        if (dSinceLast < -3000) // if new time is significantly behind (3s), use it:
        {                       // the clock may have been set backwards.
            data.dLastTime = time;
        }
        else
        {
            data.dLastTime = max(data.dLastTime, time); // otherwise, make sure we don't regress the time.
        }

        data.fReset = false;
        return data.dLastTime;
    }

} // namespace DateTime
} // namespace PlatformAgnostic
