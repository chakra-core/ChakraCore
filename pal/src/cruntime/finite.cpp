//-------------------------------------------------------------------------------------------------------
// ChakraCore/Pal
// Contains portions (c) copyright Microsoft, portions copyright (c) the .NET Foundation and Contributors
// and edits (c) copyright the ChakraCore Contributors.
// See THIRD-PARTY-NOTICES.txt in the project root for .NET Foundation license
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*++



Module Name:

    finite.cpp

Abstract:

    Implementation of _finite function (Windows specific runtime function).



--*/

#include <math.h>
#include "pal/palinternal.h"
#include "pal/dbgmsg.h"

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

