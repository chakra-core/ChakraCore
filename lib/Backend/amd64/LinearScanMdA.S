//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

.intel_syntax noprefix
#include "unixasmmacros.inc"


//------------------------------------------------------------------------------
// LinearScanMD::SaveAllRegisters(BailOutRecord *const bailOutRecord)

.balign 16
LEAF_ENTRY _ZN12LinearScanMD26SaveAllRegistersEP13BailOutRecord, _TEXT

    // [rsp + 7 * 8] == saved rax
    // [rsp + 8 * 8] == saved rdi
    // [rsp + 9 * 8] == saved rsi
    // rdi == bailOutRecord
    // rsi == condition

    mov rax, [rdi] // bailOutRecord->globalBailOutRecordDataTable
    mov rax, [rax] // bailOutRecord->globalBailOutRecordDataTable->registerSaveSpace

    // Save r8 first to free up a register
    mov [rax + 8 * 8], r8

    // Save the original values of rax, rdi, and rsi into the actual register save space
    mov r8, [rsp + 7 * 8] // saved rax
    mov [rax + 0 * 8], r8
    mov r8, [rsp + 8 * 8] // saved rdi
    mov [rax + 7 * 8], r8
    mov r8, [rsp + 9 * 8] // saved rsi
    mov [rax + 6 * 8], r8

    // Save remaining registers
    mov [rax + 1 * 8], rcx
    mov [rax + 2 * 8], rdx
    mov [rax + 3 * 8], rbx
    // [rax + 4 * 8] == save space for rsp, which doesn't need to be saved since bailout uses rbp for stack access
    mov [rax + 5 * 8], rbp
    // mov [rax + 6 * 8], rsi   // rsi saved above
    // mov [rax + 7 * 8], rdi   // rdi saved above
    // mov [rax + 8 * 8], r8    // r8 was saved earlier
    mov [rax + 9 * 8], r9
    mov [rax + 10 * 8], r10
    mov [rax + 11 * 8], r11
    mov [rax + 12 * 8], r12
    mov [rax + 13 * 8], r13
    mov [rax + 14 * 8], r14
    mov [rax + 15 * 8], r15

    // Save all XMM regs (full width)
    movups xmmword ptr [rax + 80h], xmm0         // [rax + 16 * 8 + 0 * 16] = xmm0
    movups xmmword ptr [rax + 90h], xmm1         // [rax + 16 * 8 + 1 * 16] = xmm1
    movups xmmword ptr [rax + 0a0h], xmm2        //  ...
    // movups xmmword ptr [rax + 0b0h], xmm3    // xplat: WHY this one fails to compile...
    movups xmmword ptr [rax + 11 * 16], xmm3
    movups xmmword ptr [rax + 0c0h], xmm4
    movups xmmword ptr [rax + 0d0h], xmm5
    movups xmmword ptr [rax + 0e0h], xmm6
    movups xmmword ptr [rax + 0f0h], xmm7
    movups xmmword ptr [rax + 100h], xmm8
    movups xmmword ptr [rax + 110h], xmm9
    movups xmmword ptr [rax + 120h], xmm10
    movups xmmword ptr [rax + 130h], xmm11
    movups xmmword ptr [rax + 140h], xmm12
    movups xmmword ptr [rax + 150h], xmm13
    movups xmmword ptr [rax + 160h], xmm14
    movups xmmword ptr [rax + 170h], xmm15       // [rax + 16 * 8 + 15 * 16] = xmm15

    ret

LEAF_END _ZN12LinearScanMD26SaveAllRegistersEP13BailOutRecord, _TEXT


//------------------------------------------------------------------------------
// LinearScanMD::SaveAllRegistersAndBailOut(BailOutRecord *const bailOutRecord)

.balign 16
NESTED_ENTRY _ZN12LinearScanMD26SaveAllRegistersAndBailOutEP13BailOutRecord, _TEXT, NoHandler

    // We follow Custom calling convention
    // [rsp + 1 * 8] == saved rax
    // [rsp + 2 * 8] == saved rdi
    // rdi == bailOutRecord

    // Relative to this function, SaveAllRegisters expects:
    //     [rsp + 3 * 8] == saved rsi
    // Since rsi is not a parameter to this function, it won't be saved on the stack by jitted code, so copy it there now

    mov [rsp + 3 * 8], rsi

    sub rsp, 28h        // use the same as Windows x64 so register locations are the same
    .cfi_adjust_cfa_offset 0x28

    call C_FUNC(_ZN12LinearScanMD26SaveAllRegistersEP13BailOutRecord)

    add rsp, 28h        // deallocate stack space
    .cfi_adjust_cfa_offset -0x28

    jmp C_FUNC(_ZN13BailOutRecord7BailOutEPKS_)

NESTED_END _ZN12LinearScanMD26SaveAllRegistersAndBailOutEP13BailOutRecord, _TEXT


//------------------------------------------------------------------------------
// LinearScanMD::SaveAllRegistersAndBranchBailOut(BranchBailOutRecord *const bailOutRecord, const BOOL condition)

.balign 16
NESTED_ENTRY _ZN12LinearScanMD32SaveAllRegistersAndBranchBailOutEP19BranchBailOutRecordi, _TEXT, NoHandler

    // We follow custom calling convention
    // [rsp + 1 * 8] == saved rax
    // [rsp + 2 * 8] == saved rdi
    // [rsp + 3 * 8] == saved rsi
    // rdi == bailOutRecord
    // rsi == condition

    sub rsp, 28h        // use the same as Windows x64 so register locations are the same
    .cfi_adjust_cfa_offset 0x28

    call C_FUNC(_ZN12LinearScanMD26SaveAllRegistersEP13BailOutRecord)

    add rsp, 28h        // deallocate stack space
    .cfi_adjust_cfa_offset -0x28

    jmp C_FUNC(_ZN19BranchBailOutRecord7BailOutEPKS_i)

NESTED_END _ZN12LinearScanMD32SaveAllRegistersAndBranchBailOutEP19BranchBailOutRecordi, _TEXT
