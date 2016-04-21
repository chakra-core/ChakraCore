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
        mov [rdi+00h], rsp
        mov [rdi+08h], rax
        mov [rdi+10h], rbx
        mov [rdi+18h], rcx
        mov [rdi+20h], rdx
        mov [rdi+28h], rbp
        mov [rdi+30h], rsi
        mov [rdi+38h], rdi
        mov [rdi+40h], r8
        mov [rdi+48h], r9
        mov [rdi+50h], r10
        mov [rdi+58h], r11
        mov [rdi+60h], r12
        mov [rdi+68h], r13
        mov [rdi+70h], r14
        mov [rdi+78h], r15
        ret
