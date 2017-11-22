;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

;void arm64_SAVE_REGISTERS(void*)
;
;   This method pushes sp and x1-x16,x19-x28 onto an array of 27 ULONG_PTR at x0.
;   By convention, sp is at offset 0
;
;       ULONG_PTR registers[27];
;       arm64_SAVE_REGISTERS(registers);
;
    OPT 2   ; disable listing

#include "ksarm64.h"

    OPT 1   ; re-enable listing

    TTL Lib\Common\Memory\arm64\arm64_SAVE_REGISTERS.asm

    EXPORT  arm64_SAVE_REGISTERS

    TEXTAREA

    LEAF_ENTRY arm64_SAVE_REGISTERS

    ; Can't use sp with stp so mov to a volatile register
    ; and then store onto passed in array
    mov   x17, sp
    str   x17,      [x0, #0x00]
    str   x1,       [x0, #0x08]
    stp   x2,  x3,  [x0, #0x10]
    stp   x4,  x5,  [x0, #0x20]
    stp   x6,  x7,  [x0, #0x30]
    stp   x8,  x9,  [x0, #0x40]
    stp   x10, x11, [x0, #0x50]
    stp   x12, x13, [x0, #0x60]
    stp   x14, x15, [x0, #0x70]
    stp   x16, x19, [x0, #0x80]
    stp   x20, x21, [x0, #0x90]
    stp   x22, x23, [x0, #0xA0]
    stp   x24, x25, [x0, #0xB0]
    stp   x26, x27, [x0, #0xC0]
    str   x28,      [x0, #0xD0]

    br      lr

    LEAF_END arm64_SAVE_REGISTERS

    END
