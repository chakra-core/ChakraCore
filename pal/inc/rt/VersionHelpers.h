//-------------------------------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef PAL_INC_VERSIONHELPERS_H
#define PAL_INC_VERSIONHELPERS_H

#define IsWindowsVersionOrGreater(a,b,c) false

#ifdef x
    SOMETHING IS TERRIBLY WRONG
#endif

#define x(y) \
inline bool y() { return false; }

x(IsWindowsXPOrGreater)
x(IsWindowsXPSP1OrGreater)
x(IsWindowsXPSP2OrGreater)
x(IsWindowsXPSP3OrGreater)
x(IsWindowsVistaOrGreater)
x(IsWindowsVistaSP1OrGreater)
x(IsWindowsVistaSP2OrGreater)
x(IsWindows7OrGreater)
x(IsWindows7SP1OrGreater)
x(IsWindows8OrGreater)
x(IsWindows8Point1OrGreater)
x(IsWindowsServer)

#undef x

#ifdef x
    DEFINITION FOR "x" SHOULD NOT LEAK
#endif

#endif // PAL_INC_VERSIONHELPERS_H
