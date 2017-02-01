// -------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// -------------------------------------------------------------------------------------------------------
// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

// ==++==
//

//
// ==--==
#include "unixasmmacros.inc"
#include "asmconstants.h"

.syntax unified
.thumb

// LPVOID __stdcall GetCurrentIP(void)//
    LEAF_ENTRY GetCurrentIP, _TEXT
        mov     r0, lr
        bx      lr
    LEAF_END GetCurrentIP, _TEXT

// LPVOID __stdcall GetCurrentSP(void)//
    LEAF_ENTRY GetCurrentSP, _TEXT
        mov     r0, sp
        bx      lr
    LEAF_END GetCurrentSP, _TEXT
