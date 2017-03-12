//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef NUMBER_UTIL_INLINE
#define NUMBER_UTIL_INLINE
#endif

#if !defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
#if defined(_MSC_VER) // WINDOWS

#ifdef BIG_ENDIAN
#define __BIG_ENDIAN__
#else
#define __LITTLE_ENDIAN__
#endif

#else // NOT WINDOWS

#ifdef __BYTE_ORDER
#define X_BYTE_ORDER __BYTE_ORDER
#define X_LITTLE_ENDIAN __LITTLE_ENDIAN
#elif defined(BYTE_ORDER)
#define X_BYTE_ORDER BYTE_ORDER
#define X_LITTLE_ENDIAN LITTLE_ENDIAN
#else // BYTE_ORDER not defined
#error "Endiannes of this platform is undefined"
#endif // __BYTE_ORDER

#if X_BYTE_ORDER == X_LITTLE_ENDIAN
#define __LITTLE_ENDIAN__
#else // X_BYTE_ORDER != X_LITTLE_ENDIAN
#define __BIG_ENDIAN__
#endif

#undef X_BYTE_ORDER
#undef X_LITTLE_ENDIAN

#endif // defined(_MSC_VER)
#endif // !defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)

namespace Js
{
    NUMBER_UTIL_INLINE uint32 &NumberUtilities::LuHiDbl(double &dbl)
    {
#if defined(__BIG_ENDIAN__)
        return ((uint32 *)&dbl)[0];
#else //!BIG_ENDIAN
        return ((uint32 *)&dbl)[1];
#endif //!BIG_ENDIAN
    }

    NUMBER_UTIL_INLINE uint32 &NumberUtilities::LuLoDbl(double &dbl)
    {
#if defined(__BIG_ENDIAN__)
        return ((uint32 *)&dbl)[1];
#else //!BIG_ENDIAN
        return ((uint32 *)&dbl)[0];
#endif //!BIG_ENDIAN
    }

#if defined(_M_X64) && defined(_MSC_VER) && !defined(__clang__)
    NUMBER_UTIL_INLINE INT64 NumberUtilities::TryToInt64(double T1)
    {
        // _mm_cvttsd_si64x will result in 0x8000000000000000 if the value is NaN Inf or Zero, or overflows int64
        __m128d a;
        a = _mm_load_sd(&T1);
        return _mm_cvttsd_si64x(a);
    }
#else
    NUMBER_UTIL_INLINE INT64 NumberUtilities::TryToInt64(double T1)
    {
        INT64 T4_64;
#if defined(_M_IX86)
        // If SSE3 is available use FISTPP.  VC (dev10) generates a FISTP, but needs to
        // first change the FPU rounding, which is very slow...
        if (AutoSystemInfo::Data.SSE3Available())
        {
            // FISTTP will result in 0x8000000000000000 in T4_64 if the value is NaN Inf or Zero, or overflows int64
            _asm {
                FLD T1
                FISTTP T4_64
            }
        }
        else
#endif
#if defined(_M_ARM32_OR_ARM64)
        // Win8 286065: ARM: casts to int64 from double for NaNs, infinity, overflow:
        // - non-infinity NaNs -> 0
        // - infinity NaNs: -1.#INF -> 0x8000000000000000, 1.#INF  -> 0x7FFFFFFFFFFFFFFF.
        // - overflow: negative     -> 0x8000000000000000, positive-> 0x7FFFFFFFFFFFFFFF.
        // We have to take care of non-infinite NaNs to make sure the result is not a valid int64 rather than 0.
        if (IsNan(T1))
        {
            return Pos_InvalidInt64;
        }
        else if (T1 < -9223372036854775808.0) // -9223372036854775808 is double value corresponding to Neg_InvalidInt64.
        {
            // TODO: Remove this temp workaround.
            // This is to walk around CRT issue (Win8 404170): there is a band of values near/less than negative overflow
            // for which cast to int64 results in positive number (bug), then going further down in negative direction it turns
            // back to negative overflow value (as it should).
            return Pos_InvalidInt64;
        }
        else
#endif
        {
            // The cast will result in 0x8000000000000000 in T4_64 if the value is NaN Inf or Zero, or overflows int64
            T4_64 = static_cast<INT64>(T1);
        }

#if defined(_M_ARM32_OR_ARM64)
        if (T4_64 == Neg_InvalidInt64)
        {
            // Win8 391983: what happens in 64bit overflow is not spec'd. On ARM T4_64 would be 0x7F..FF but if we extend
            // ToInt32 to 64bit, because of ES5_9.5.5 the result would be 0x80..00. On Intel all overflows result in 0x80..00.
            // So, be consistent with Intel.
            return Pos_InvalidInt64;
        }
#endif

        return T4_64;
    }
#endif

    // Returns true <=> TryToInt64() call resulted in a valid value.
    NUMBER_UTIL_INLINE bool NumberUtilities::IsValidTryToInt64(__int64 value)
    {
#if defined(_M_ARM32_OR_ARM64)
        return value != Pos_InvalidInt64 && value != Neg_InvalidInt64;
#else
        return value != Pos_InvalidInt64;
#endif
    }

    NUMBER_UTIL_INLINE bool NumberUtilities::IsNan(double value)
    {
#if defined(_M_X64_OR_ARM64)
        // NaN is a range of values; all bits on the exponent are 1's and some nonzero significant.
        // no distinction on signed NaN's
        uint64 nCompare = ToSpecial(value);
        bool isNan = (0 == (~nCompare & 0x7FF0000000000000ull) &&
            0 != (nCompare & 0x000FFFFFFFFFFFFFull));
        return isNan;
#else
        return 0 == (~Js::NumberUtilities::LuHiDbl(value) & 0x7FF00000) &&
            (0 != Js::NumberUtilities::LuLoDbl(value) || 0 != (Js::NumberUtilities::LuHiDbl(value) & 0x000FFFFF));
#endif
    }

    NUMBER_UTIL_INLINE bool NumberUtilities::IsSpecial(double value, uint64 nSpecial)
    {
        // Perform a bitwise comparison using uint64 instead of a double comparison, since that
        // would trigger FPU exceptions, etc.
        uint64 nCompare = ToSpecial(value);
        return nCompare == nSpecial;
    }

    NUMBER_UTIL_INLINE uint64 NumberUtilities::ToSpecial(double value)
    {
        return  *(reinterpret_cast<uint64 *>(&value));
    }

    NUMBER_UTIL_INLINE uint32 NumberUtilities::ToSpecial(float value)
    {
        return  *(reinterpret_cast<uint32 *>(&value));
    }

    NUMBER_UTIL_INLINE float NumberUtilities::ReinterpretBits(int value)
    {
        return  *(reinterpret_cast<float *>(&value));
    }

    NUMBER_UTIL_INLINE double NumberUtilities::ReinterpretBits(int64 value)
    {
        return  *(reinterpret_cast<double *>(&value));
    }

    NUMBER_UTIL_INLINE bool NumberUtilities::IsFloat32NegZero(float value)
    {
        uint32 nCompare = ToSpecial(value);
        return nCompare == NumberConstants::k_Float32NegZero;
    }
}
