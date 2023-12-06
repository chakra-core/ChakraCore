;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Copyright (c) ChakraCore Project Contributors. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

;;============================================================================================================
; Fake __chkstk
;;============================================================================================================
LEAF_ENTRY __chkstk, _TEXT
        ret
LEAF_END __chkstk, _TEXT

;;============================================================================================================
; NativeCodeGenerator::CheckCodeGenThunk
;;============================================================================================================
    ;Js::Var NativeCodeGenerator::CheckCodeGenThunk(Js::RecyclableObject* function, Js::CallInfo callInfo, ...)
NESTED_ENTRY _ZN19NativeCodeGenerator17CheckCodeGenThunkEPN2Js16RecyclableObjectENS0_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED  fp, lr, -(16+16*8)   ; establish stack frame
    stp     d0, d1, [sp, #16+0*8]
    stp     d2, d3, [sp, #16+2*8]
    stp     d4, d5, [sp, #16+4*8]
    stp     d6, d7, [sp, #16+6*8]
    stp     x0, x1, [sp, #16+8*8]
    stp     x2, x3, [sp, #16+10*8]
    stp     x4, x5, [sp, #16+12*8]
    stp     x6, x7, [sp, #16+14*8]

    bl      C_FUNC(_ZN19NativeCodeGenerator12CheckCodeGenEPN2Js14ScriptFunctionE)  ; call  NativeCodeGenerator::CheckCodeGen
    mov     x15, x0               ; move entry point to x15

    ldp     d0, d1, [sp, #16+0*8]
    ldp     d2, d3, [sp, #16+2*8]
    ldp     d4, d5, [sp, #16+4*8]
    ldp     d6, d7, [sp, #16+6*8]
    ldp     x0, x1, [sp, #16+8*8]
    ldp     x2, x3, [sp, #16+10*8]
    ldp     x4, x5, [sp, #16+12*8]
    ldp     x6, x7, [sp, #16+14*8]
    EPILOG_RESTORE_REG_PAIR_INDEXED  fp, lr, (16+16*8)
    br      x15             ; jump (tail call) to new entryPoint

NESTED_END _ZN19NativeCodeGenerator17CheckCodeGenThunkEPN2Js16RecyclableObjectENS0_8CallInfoEz, _TEXT
