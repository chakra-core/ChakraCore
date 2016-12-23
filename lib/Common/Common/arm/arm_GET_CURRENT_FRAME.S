// -------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// -------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

.syntax unified
.thumb

    // Var arm_GET_CURRENT_FRAME(void) //
    LEAF_ENTRY arm_GET_CURRENT_FRAME, _TEXT
        mov     r0,r11
        bx      lr
    LEAF_END arm_GET_CURRENT_FRAME, _TEXT
