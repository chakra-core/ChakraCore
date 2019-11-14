// -------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// -------------------------------------------------------------------------------------------------------

.intel_syntax noprefix
#include "unixasmmacros.inc"

//void amd64_SAVE_REGISTERS(registers)
//
//   This method pushes the 16 general purpose registers into the passed in array.
//   By convention, the stack pointer will always be stored at registers[0]
//
//       void* registers[16];
//       amd64_SAVE_REGISTERS(registers);
//
.globl C_FUNC(amd64_SAVE_REGISTERS)
C_FUNC(amd64_SAVE_REGISTERS):   
        mov [rdi+0x00], rsp
        mov [rdi+0x08], rax
        mov [rdi+0x10], rbx
        mov [rdi+0x18], rcx
        mov [rdi+0x20], rdx
        mov [rdi+0x28], rbp
        mov [rdi+0x30], rsi
        mov [rdi+0x38], rdi
        mov [rdi+0x40], r8
        mov [rdi+0x48], r9
        mov [rdi+0x50], r10
        mov [rdi+0x58], r11
        mov [rdi+0x60], r12
        mov [rdi+0x68], r13
        mov [rdi+0x70], r14
        mov [rdi+0x78], r15
        ret
