;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Copyright (c) ChakraCore Project Contributors. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

//void arm64_SAVE_REGISTERS(registers)
//
//   This method pushes the 16 general purpose registers into the passed in array.
//   By convention, the stack pointer will always be stored at registers[0]
//
//       void* registers[16];
//       arm64_SAVE_REGISTERS(registers);
//
.p2align 2
.globl C_FUNC(arm64_SAVE_REGISTERS)
C_FUNC(arm64_SAVE_REGISTERS):
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
