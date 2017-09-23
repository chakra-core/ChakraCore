// -------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// -------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

.syntax unified
.thumb

    LEAF_ENTRY arm_SAVE_REGISTERS, _TEXT
        str     sp, [r0], #+4
        stmia   r0, {r1-r12}

        bx      lr
    LEAF_END arm_SAVE_REGISTERS, _TEXT
