//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
// ---------------------------------------------------------------------------
// safemath.h
//
// overflow checking infrastructure
// ---------------------------------------------------------------------------


#ifndef SAFEMATH_H_
#define SAFEMATH_H_

#include <debugmacrosext.h>

#ifndef _ASSERTE_SAFEMATH
#ifdef _ASSERTE
// Use _ASSERTE if we have it (should always be the case in the CLR)
#define _ASSERTE_SAFEMATH _ASSERTE
#else
// Otherwise (eg. we're being used from a tool like SOS) there isn't much
// we can rely on that is both available everywhere and rotor-safe.  In
// several other tools we just take the recourse of disabling asserts,
// we'll do the same here.
// Ideally we'd have a collection of common utilities available evererywhere.
#define _ASSERTE_SAFEMATH(a)
#endif
#endif

#if !defined(__IOS__) && !defined(OSX_SDK_TR1) && defined(__APPLE__)
#include <AvailabilityMacros.h>
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < MAC_OS_X_VERSION_10_9
#define OSX_SDK_TR1
#endif
#endif

#ifdef OSX_SDK_TR1
#include <tr1/type_traits>
#else
#include <type_traits>
#endif

//==================================================================
// Semantics: if val can be represented as the exact same value
// when cast to Dst type, then FitsIn<Dst>(val) will return true;
// otherwise FitsIn returns false.
//
// Dst and Src must both be integral types.
//
// It's important to note that most of the conditionals in this
// function are based on static type information and as such will
// be optimized away. In particular, the case where the signs are
// identical will result in no code branches.

#ifdef _PREFAST_
#pragma warning(push)
#pragma warning(disable:6326) // PREfast warning: Potential comparison of a constant with another constant
#endif // _PREFAST_

template <typename Dst, typename Src>
inline bool FitsIn(Src val)
{
#ifdef _MSC_VER
    static_assert_no_msg(!__is_class(Dst));
    static_assert_no_msg(!__is_class(Src));
#endif

    if (std::is_signed<Src>::value == std::is_signed<Dst>::value)
    {   // Src and Dst are equally signed
        if (sizeof(Src) <= sizeof(Dst))
        {   // No truncation is possible
            return true;
        }
        else
        {   // Truncation is possible, requiring runtime check
            return val == (Src)((Dst)val);
        }
    }
    else if (std::is_signed<Src>::value)
    {   // Src is signed, Dst is unsigned
#ifdef __GNUC__
        // Workaround for GCC warning: "comparison is always
        // false due to limited range of data type."
        if (!(val == 0 || val > 0))
#else
        if (val < 0)
#endif
        {   // A negative number cannot be represented by an unsigned type
            return false;
        }
        else
        {
            if (sizeof(Src) <= sizeof(Dst))
            {   // No truncation is possible
                return true;
            }
            else
            {   // Truncation is possible, requiring runtime check
                return val == (Src)((Dst)val);
            }
        }
    }
    else
    {   // Src is unsigned, Dst is signed
        if (sizeof(Src) < sizeof(Dst))
        {   // No truncation is possible. Note that Src is strictly
            // smaller than Dst.
            return true;
        }
        else
        {   // Truncation is possible, requiring runtime check
#ifdef __GNUC__
            // Workaround for GCC warning: "comparison is always
            // true due to limited range of data type." If in fact
            // Dst were unsigned we'd never execute this code
            // anyway.
            return ((Dst)val > 0 || (Dst)val == 0) &&
#else
            return ((Dst)val >= 0) &&
#endif
                   (val == (Src)((Dst)val));
        }
    }
}

// Requires that Dst is an integral type, and that DstMin and DstMax are the
// minimum and maximum values of that type, respectively.  Returns "true" iff
// "val" can be represented in the range [DstMin..DstMax] (allowing loss of precision, but
// not truncation).
template <INT64 DstMin, UINT64 DstMax>
inline bool FloatFitsInIntType(float val)
{
    float DstMinF = static_cast<float>(DstMin);
    float DstMaxF = static_cast<float>(DstMax);
    return DstMinF <= val && val <= DstMaxF;
}

template <INT64 DstMin, UINT64 DstMax>
inline bool DoubleFitsInIntType(double val)
{
    double DstMinD = static_cast<double>(DstMin);
    double DstMaxD = static_cast<double>(DstMax);
    return DstMinD <= val && val <= DstMaxD;
}

#ifdef _PREFAST_
#pragma warning(pop)
#endif //_PREFAST_

#define ovadd_lt(a, b, rhs) (((a) + (b) <  (rhs) ) && ((a) + (b) >= (a)))
#define ovadd_le(a, b, rhs) (((a) + (b) <= (rhs) ) && ((a) + (b) >= (a)))
#define ovadd_gt(a, b, rhs) (((a) + (b) >  (rhs) ) || ((a) + (b) < (a)))
#define ovadd_ge(a, b, rhs) (((a) + (b) >= (rhs) ) || ((a) + (b) < (a)))

#define ovadd3_gt(a, b, c, rhs) (((a) + (b) + (c) > (rhs)) || ((a) + (b) < (a)) || ((a) + (b) + (c) < (c)))

#if defined(_TARGET_X86_) && defined( _MSC_VER )
#define S_SIZE_T_WP64BUG(v)  S_SIZE_T( static_cast<UINT32>( v ) )
#else
#define S_SIZE_T_WP64BUG(v)  S_SIZE_T( v )
#endif

 #endif // SAFEMATH_H_
