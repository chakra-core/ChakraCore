//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information. 
//

/*++



Module Name:

    finite.cpp

Abstract:

    Implementation of _finite function (Windows specific runtime function).



--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include <math.h>

#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif  // HAVE_IEEEFP_H
#include <errno.h>

#define PAL_NAN sqrt(-1.0)
#define PAL_POSINF -log(0.0)
#define PAL_NEGINF log(0.0)

SET_DEFAULT_DEBUG_CHANNEL(CRT);


/*++
Function:
  _finite

Determines whether given double-precision floating point value is finite.

Return Value

_finite returns a nonzero value (TRUE) if its argument x is not
infinite, that is, if -INF < x < +INF. It returns 0 (FALSE) if the
argument is infinite or a NaN.

Parameter

x  Double-precision floating-point value

--*/
int
__cdecl
_finite(
        double x)
{
    int ret;
    PERF_ENTRY(_finite);
    ENTRY("_finite (x=%f)\n", x);
#if defined(_IA64_) && defined (_HPUX_)
    ret = !isnan(x) && x != PAL_POSINF && x != PAL_NEGINF;
#else
    ret = isfinite(x);
#endif
    LOGEXIT("_finite returns int %d\n", ret);
    PERF_EXIT(_finite);
    return ret;
}


/*++
Function:
  _isnan

See MSDN doc
--*/
int
__cdecl
_isnan(
       double x)
{
    int ret;

    PERF_ENTRY(_isnan);
    ENTRY("_isnan (x=%f)\n", x);
    ret = isnan(x);
    LOGEXIT("_isnan returns int %d\n", ret);
    PERF_EXIT(_isnan);
    return ret;
}

/*++
Function:
  _copysign

See MSDN doc
--*/
double 
__cdecl 
_copysign(
          double x,
          double y)
{
    double ret;

    PERF_ENTRY(_copysign);
    ENTRY("_copysign (x=%f,y=%f)\n", x, y);
    ret = copysign(x, y);
    LOGEXIT("_copysign returns double %f\n", ret);
    PERF_EXIT(_copysign);
    return ret;
}

/*++
Function:
_copysignf

See MSDN doc
--*/
float
__cdecl
_copysignf(
    float x,
    float y)
{
    double ret;

    PERF_ENTRY(_copysignf);
    ENTRY("_copysignf (x=%f,y=%f)\n", x, y);
    ret = copysignf(x, y);
    LOGEXIT("_copysignf returns float %f\n", ret);
    PERF_EXIT(_copysignf);
    return ret;
}

/*++
Function:
    acos

See MSDN.
--*/
PALIMPORT double __cdecl PAL_acos(double x)
{
    double ret;

    PERF_ENTRY(acos);
    ENTRY("acos (x=%f)\n", x);
#if !HAVE_COMPATIBLE_ACOS
    errno = 0;
#endif  // HAVE_COMPATIBLE_ACOS
    ret = acos(x);
#if !HAVE_COMPATIBLE_ACOS
    if (errno == EDOM)
    {
        ret = PAL_NAN;  // NaN
    }
#endif  // HAVE_COMPATIBLE_ACOS
    LOGEXIT("acos returns double %f\n", ret);
    PERF_EXIT(acos);
    return ret;
}

/*++
Function:
    asin

See MSDN.
--*/
PALIMPORT double __cdecl PAL_asin(double x)
{
    double ret;

    PERF_ENTRY(asin);
    ENTRY("asin (x=%f)\n", x);
#if !HAVE_COMPATIBLE_ASIN
    errno = 0;
#endif  // HAVE_COMPATIBLE_ASIN
    ret = asin(x);
#if !HAVE_COMPATIBLE_ASIN
    if (errno == EDOM)
    {
        ret = PAL_NAN;  // NaN
    }
#endif  // HAVE_COMPATIBLE_ASIN
    LOGEXIT("asin returns double %f\n", ret);
    PERF_EXIT(asin);
    return ret;
}

/*++
Function:
    atan2

See MSDN.
--*/
PALIMPORT double __cdecl PAL_atan2(double y, double x)
{
    double ret;

    PERF_ENTRY(atan2);
    ENTRY("atan2 (y=%f, x=%f)\n", y, x);
#if !HAVE_COMPATIBLE_ATAN2
    errno = 0;
#endif  // !HAVE_COMPATIBLE_ATAN2
    ret = atan2(y, x);
#if !HAVE_COMPATIBLE_ATAN2
    if (errno == EDOM)
    {
#if HAVE_COPYSIGN
        if (x == 0.0 && y == 0.0)
        {
            const double sign_x = copysign (1.0, x);
            const double sign_y = copysign (1.0, y);
            if (sign_x > 0)
            {
                ret = copysign (0.0, sign_y);
            }
            else
            {
                ret = copysign (atan2 (0.0, -1.0), sign_y);
            }
        }
#else   // HAVE_COPYSIGN
#error  Missing copysign or equivalent on this platform!
#endif  // HAVE_COPYSIGN
    }
#endif  // !HAVE_COMPATIBLE_ATAN2
    LOGEXIT("atan2 returns double %f\n", ret);
    PERF_EXIT(atan2);
    return ret;
}

/*++
Function:
    exp

See MSDN.
--*/
PALIMPORT double __cdecl PAL_exp(double x)
{
    double ret;

    PERF_ENTRY(exp);
    ENTRY("exp (x=%f)\n", x);
#if !HAVE_COMPATIBLE_EXP
    if (x == 1.0) 
    {
        ret = M_E;
    }
    else
    {
        ret = exp(x);
    }
#else // !HAVE_COMPATIBLE_EXP
    ret = exp(x);
#endif // !HAVE_COMPATIBLE_EXP
    LOGEXIT("exp returns double %f\n", ret);
    PERF_EXIT(exp);
    return ret;
}

/*++
Function:
    labs

See MSDN.
--*/
PALIMPORT LONG __cdecl PAL_labs(LONG l)
{
    long lRet;

    PERF_ENTRY(labs);
    ENTRY("labs (l=%ld)\n", l);
    
    lRet = labs(l);    

    LOGEXIT("labs returns long %ld\n", lRet);
    PERF_EXIT(labs);
      /* This explicit cast to LONG is used to silence any potential warnings
         due to implicitly casting the native long lRet to LONG when returning. */
    return (LONG)lRet;
}

/*++
Function:
    log

See MSDN.
--*/
PALIMPORT double __cdecl PAL_log(double x)
{
    double ret;

    PERF_ENTRY(log);
    ENTRY("log (x=%f)\n", x);
#if !HAVE_COMPATIBLE_LOG
    errno = 0;
#endif  // !HAVE_COMPATIBLE_LOG
    ret = log(x);
#if !HAVE_COMPATIBLE_LOG
    if (errno == EDOM)
    {
        if (x < 0)
        {
	    ret = PAL_NAN;    // NaN
        }
    }
#endif  // !HAVE_COMPATIBLE_LOG
    LOGEXIT("log returns double %f\n", ret);
    PERF_EXIT(log);
    return ret;
}

/*++
Function:
    log10

See MSDN.
--*/
PALIMPORT double __cdecl PAL_log10(double x)
{
    double ret;

    PERF_ENTRY(log10);
    ENTRY("log10 (x=%f)\n", x);
#if !HAVE_COMPATIBLE_LOG10
    errno = 0;
#endif  // !HAVE_COMPATIBLE_LOG10
    ret = log10(x);
#if !HAVE_COMPATIBLE_LOG10
    if (errno == EDOM)
    {
        if (x < 0)
        {
	    ret = PAL_NAN;    // NaN
        }
    }
#endif  // !HAVE_COMPATIBLE_LOG10
    LOGEXIT("log10 returns double %f\n", ret);
    PERF_EXIT(log10);
    return ret;
}
