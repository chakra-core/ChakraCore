;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------
    OPT 2   ; disable listing
#include "ksarm64.h"
    OPT 1   ; re-enable listing

    TTL Lib\Backend\arm64\Thunks.asm

    ;Js::Var NativeCodeGenerator::CheckCodeGenThunk(Js::RecyclableObject* function, Js::CallInfo callInfo, ...)
    EXPORT  |?CheckCodeGenThunk@NativeCodeGenerator@@SAPEAXPEAVRecyclableObject@Js@@UCallInfo@3@ZZ|

    ;Js::JavascriptMethod NativeCodeGenerator::CheckCodeGen(Js::JavascriptFunction * function)
    IMPORT  |?CheckCodeGen@NativeCodeGenerator@@SAP6APEAXPEAVRecyclableObject@Js@@UCallInfo@3@ZZPEAVScriptFunction@3@@Z|

#if defined(_CONTROL_FLOW_GUARD)
    IMPORT __guard_check_icall_fptr
#endif

    TEXTAREA

;;============================================================================================================
; NativeCodeGenerator::CheckCodeGenThunk
;;============================================================================================================
    ;Js::Var NativeCodeGenerator::CheckCodeGenThunk(Js::RecyclableObject* function, Js::CallInfo callInfo, ...)
    NESTED_ENTRY ?CheckCodeGenThunk@NativeCodeGenerator@@SAPEAXPEAVRecyclableObject@Js@@UCallInfo@3@ZZ

    PROLOG_SAVE_REG_PAIR fp, lr, #-(16*8+32)!          ; establish stack frame
    PROLOG_SAVE_REG_PAIR x19, x20, #16
    stp     x0, x1, [sp, #32+0*8]
    stp     x2, x3, [sp, #32+2*8]
    stp     x4, x5, [sp, #32+4*8]
    stp     x6, x7, [sp, #32+6*8]
    stp     x8, x9, [sp, #32+8*8]
    stp     x10, x11, [sp, #32+10*8]
    stp     x12, x13, [sp, #32+12*8]
    stp     x14, x15, [sp, #32+14*8]

    bl      |?CheckCodeGen@NativeCodeGenerator@@SAP6APEAXPEAVRecyclableObject@Js@@UCallInfo@3@ZZPEAVScriptFunction@3@@Z|  ; call  NativeCodeGenerator::CheckCodeGen

#if defined(_CONTROL_FLOW_GUARD)
    mov     x19, x0               ; save entryPoint in x19
    adrp    x17, __guard_check_icall_fptr
    ldr     x17, [x17, __guard_check_icall_fptr]
    blr     x17
    mov     x17, x19              ; restore entryPoint in x17
#else
    mov     x17, x0               ; back up entryPoint in x17
#endif

    ldp     x0, x1, [sp, #32+0*8]
    ldp     x2, x3, [sp, #32+2*8]
    ldp     x4, x5, [sp, #32+4*8]
    ldp     x6, x7, [sp, #32+6*8]
    ldp     x8, x9, [sp, #32+8*8]
    ldp     x10, x11, [sp, #32+10*8]
    ldp     x12, x13, [sp, #32+12*8]
    ldp     x14, x15, [sp, #32+14*8]
    EPILOG_RESTORE_REG_PAIR x19, x20, #16
    EPILOG_RESTORE_REG_PAIR fp, lr, #(16*8+32)!
    EPILOG_NOP br x17             ; jump (tail call) to new entryPoint

    NESTED_END

;;============================================================================================================
    END
