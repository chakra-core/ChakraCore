//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
.intel_syntax noprefix

.global amd64_CallWithFakeFrame
.global amd64_ReturnFromCallWithFakeFrame

        // r8       = arg0.
        // rcx      = args size.
        // rdx      = spill size.
        // rsi      = original frame pointer.
        // rdi      = target.
.align 16
.text
amd64_CallWithFakeFrame:        
        // xplat-todo: need to have an equivalent function to __chkstk that we can
        // call here to verify that we have enough stack space

        // The stack walker uses this marker to skip this frame.
        // Note that we use rip-relative addressing here since this
        // library is shared and therefore the code has to be position independent
        lea rax, [rip+amd64_ReturnFromCallWithFakeFrame]
        mov [rsp+8h], rax

        mov rax, [rsp + 28h]

        push rbp
        mov rbp, rsi

        // Frame spill size.
        sub rsp, rdx

        // Save callee-saved xmm registers
        movapd xmmword ptr [rsp + 90h], xmm15
        movapd xmmword ptr [rsp + 80h], xmm14
        movapd xmmword ptr [rsp + 70h], xmm13
        movapd xmmword ptr [rsp + 60h], xmm12
        movapd xmmword ptr [rsp + 50h], xmm11
        movapd xmmword ptr [rsp + 40h], xmm10
        movapd xmmword ptr [rsp + 30h], xmm9
        movapd xmmword ptr [rsp + 20h], xmm8
        movapd xmmword ptr [rsp + 10h], xmm7
        movapd xmmword ptr [rsp], xmm6

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
.align 16
amd64_ReturnFromCallWithFakeFrame:      
        add  rsp, rcx

        pop  rbx
        pop  r12
        pop  r13
        pop  r14
        pop  r15

        // Restore callee-saved xmm registers
        movapd xmm6, xmmword ptr [rsp]
        movapd xmm7, xmmword ptr [rsp + 10h]
        movapd xmm8, xmmword ptr [rsp + 20h]
        movapd xmm9, xmmword ptr [rsp + 30h]
        movapd xmm10, xmmword ptr [rsp + 40h]
        movapd xmm11, xmmword ptr [rsp + 50h]
        movapd xmm12, xmmword ptr [rsp + 60h]
        movapd xmm13, xmmword ptr [rsp + 70h]
        movapd xmm14, xmmword ptr [rsp + 80h]
        movapd xmm15, xmmword ptr [rsp + 90h]

        add  rsp, rdx

        pop  rbp

        // Return to the real caller.
        ret
