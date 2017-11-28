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

    PROLOG_SAVE_REG_PAIR fp, lr, #-(2*8+16*8)!          ; establish stack frame
    stp     d0, d1, [sp, #16+0*8]
    stp     d2, d3, [sp, #16+2*8]
    stp     d4, d5, [sp, #16+4*8]
    stp     d6, d7, [sp, #16+6*8]
    stp     x0, x1, [sp, #16+8*8]
    stp     x2, x3, [sp, #16+10*8]
    stp     x4, x5, [sp, #16+12*8]
    stp     x6, x7, [sp, #16+14*8]

    bl      |?CheckCodeGen@NativeCodeGenerator@@SAP6APEAXPEAVRecyclableObject@Js@@UCallInfo@3@ZZPEAVScriptFunction@3@@Z|  ; call  NativeCodeGenerator::CheckCodeGen
    mov     x15, x0               ; move entry point to x15

#if defined(_CONTROL_FLOW_GUARD)
    adrp    x17, __guard_check_icall_fptr
    ldr     x17, [x17, __guard_check_icall_fptr]
    blr     x17
#endif

    ldp     d0, d1, [sp, #16+0*8]
    ldp     d2, d3, [sp, #16+2*8]
    ldp     d4, d5, [sp, #16+4*8]
    ldp     d6, d7, [sp, #16+6*8]
    ldp     x0, x1, [sp, #16+8*8]
    ldp     x2, x3, [sp, #16+10*8]
    ldp     x4, x5, [sp, #16+12*8]
    ldp     x6, x7, [sp, #16+14*8]
    EPILOG_RESTORE_REG_PAIR fp, lr, #(2*8+16*8)!
    EPILOG_NOP br x15             ; jump (tail call) to new entryPoint

    NESTED_END

;;============================================================================================================
    END
