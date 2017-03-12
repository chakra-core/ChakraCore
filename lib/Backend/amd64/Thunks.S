//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

.intel_syntax noprefix
#include "unixasmmacros.inc"


//============================================================================================================
// Fake __chkstk
//============================================================================================================
.balign 16
LEAF_ENTRY __chkstk, _TEXT
        ret
LEAF_END __chkstk, _TEXT

//============================================================================================================
// NativeCodeGenerator::CheckCodeGenThunk
//============================================================================================================

//.extern _ZN19NativeCodeGenerator12CheckCodeGenEPN2Js14ScriptFunctionE
.balign 16
NESTED_ENTRY _ZN19NativeCodeGenerator17CheckCodeGenThunkEPN2Js16RecyclableObjectENS0_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp
        lea  rbp, [rsp]

        // save argument registers used by custom calling convention
        push_register rdi
        push_register rsi

        //  Js::JavascriptMethod NativeCodeGenerator::CheckCodeGen(
        //              Js::ScriptFunction * function)
        //
        //      RDI == function, setup by custom calling convention
        call C_FUNC(_ZN19NativeCodeGenerator12CheckCodeGenEPN2Js14ScriptFunctionE)

        pop_register rsi
        pop_register rdi
        pop_nonvol_reg rbp

        jmp rax
NESTED_END _ZN19NativeCodeGenerator17CheckCodeGenThunkEPN2Js16RecyclableObjectENS0_8CallInfoEz, _TEXT


//============================================================================================================
// NativeCodeGenerator::CheckAsmJsCodeGenThunk
//============================================================================================================

//.extern _ZN19NativeCodeGenerator17CheckAsmJsCodeGenEPN2Js14ScriptFunctionE
.balign 16
NESTED_ENTRY _ZN19NativeCodeGenerator22CheckAsmJsCodeGenThunkEPN2Js16RecyclableObjectENS0_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp             // push rbp and adjust CFA offset
        lea  rbp, [rsp]

        set_cfa_register rbp, (2*8)     // Set to compute CFA as: rbp + 16 (sizeof: [rbp] [ReturnAddress])

        // save argument registers used by custom calling convention
        push rdi
        push rsi
        push rdx
        push rcx
        push r8
        push r9

        sub rsp, 40h

        // ----- TODO: potentially xmm0-xmm7 args
        // spill potential floating point arguments to stack
        movaps xmmword ptr [rsp + 00h], xmm0
        movaps xmmword ptr [rsp + 10h], xmm1
        movaps xmmword ptr [rsp + 20h], xmm2
        movaps xmmword ptr [rsp + 30h], xmm3

        call C_FUNC(_ZN19NativeCodeGenerator17CheckAsmJsCodeGenEPN2Js14ScriptFunctionE)

        // restore potential floating point arguments from stack
        movaps xmm0, xmmword ptr [rsp + 00h]
        movaps xmm1, xmmword ptr [rsp + 10h]
        movaps xmm2, xmmword ptr [rsp + 20h]
        movaps xmm3, xmmword ptr [rsp + 30h]

        add rsp, 40h

        pop r9
        pop r8
        pop rcx
        pop rdx
        pop rsi
        pop rdi

        pop_nonvol_reg rbp
        jmp rax
NESTED_END _ZN19NativeCodeGenerator22CheckAsmJsCodeGenThunkEPN2Js16RecyclableObjectENS0_8CallInfoEz, _TEXT
