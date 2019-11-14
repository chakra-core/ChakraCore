//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
.intel_syntax noprefix
#include "unixasmmacros.inc"

// expected fvisibility=default
.global C_FUNC(amd64_CallWithFakeFrame)
.global C_FUNC(amd64_ReturnFromCallWithFakeFrame)
        // r8       = arg0.
        // rcx      = args size.
        // rdx      = spill size.
        // rsi      = original frame pointer.
        // rdi      = target.
.balign 16
NESTED_ENTRY amd64_CallWithFakeFrame, _TEXT, NoHandler
        // xplat-todo: need to have an equivalent function to __chkstk that we can
        // call here to verify that we have enough stack space

        mov rax, r8             // arg0

        push_nonvol_reg rbp
        mov rbp, rsi

        // Frame spill size.
        sub rsp, rdx

        // Save callee-saved xmm registers -- none on Sys V x64
        // movapd xmmword ptr [rsp + 0x90], xmm15
        // movapd xmmword ptr [rsp + 0x80], xmm14
        // movapd xmmword ptr [rsp + 0x70], xmm13
        // movapd xmmword ptr [rsp + 0x60], xmm12
        // movapd xmmword ptr [rsp + 0x50], xmm11
        // movapd xmmword ptr [rsp + 0x40], xmm10
        // movapd xmmword ptr [rsp + 0x30], xmm9
        // movapd xmmword ptr [rsp + 0x20], xmm8
        // movapd xmmword ptr [rsp + 0x10], xmm7
        // movapd xmmword ptr [rsp], xmm6

        // Save all callee saved registers.
        push r15
        push r14
        push r13
        push r12
        push rbx

        // Frame args size.
        sub  rsp, rcx

        jmp rdi

        // rcx = args size.
        // rdx = spill size.
NESTED_END amd64_CallWithFakeFrame, _TEXT


.balign 16
NESTED_ENTRY amd64_ReturnFromCallWithFakeFrame, _TEXT, NoHandler
        add  rsp, rcx

        pop  rbx
        pop  r12
        pop  r13
        pop  r14
        pop  r15

        // Restore callee-saved xmm registers -- none on Sys V x64; must match RegList.h
        // movapd xmm6, xmmword ptr [rsp]
        // movapd xmm7, xmmword ptr [rsp + 0x10]
        // movapd xmm8, xmmword ptr [rsp + 0x20]
        // movapd xmm9, xmmword ptr [rsp + 0x30]
        // movapd xmm10, xmmword ptr [rsp + 0x40]
        // movapd xmm11, xmmword ptr [rsp + 0x50]
        // movapd xmm12, xmmword ptr [rsp + 0x60]
        // movapd xmm13, xmmword ptr [rsp + 0x70]
        // movapd xmm14, xmmword ptr [rsp + 0x80]
        // movapd xmm15, xmmword ptr [rsp + 0x90]

        add  rsp, rdx

        pop_nonvol_reg rbp

        // Return to the real caller.
        ret
NESTED_END amd64_ReturnFromCallWithFakeFrame, _TEXT
