;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Copyright (c) ChakraCore Project Contributors. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

.global C_FUNC(_ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz)
.global C_FUNC(_ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz)

#ifdef _ENABLE_DYNAMIC_THUNKS

.global C_FUNC(_ZN2Js21InterpreterStackFrame28DelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz)
.global C_FUNC(_ZN2Js18DynamicProfileInfo29EnsureDynamicProfileInfoThunkEPNS_16RecyclableObjectENS_8CallInfoEz)

;;============================================================================================================
;; InterpreterStackFrame::DelayDynamicInterpreterThunk
;;============================================================================================================
    ;Var InterpreterStackFrame::DelayDynamicInterpreterThunk(RecyclableObject* function, CallInfo callInfo, ...)

NESTED_ENTRY _ZN2Js21InterpreterStackFrame28DelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -32   ; save parameters and volatile registers
    stp     x0, x1, [sp, #16]

    bl      C_FUNC(_ZN2Js21InterpreterStackFrame29EnsureDynamicInterpreterThunkEPNS_14ScriptFunctionE)  ; call InterpreterStackFrame::EnsureDynamicInterpreterThunk
    mov     x16, x0                  ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]        ; restore parameters and volatile registers

    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 32
    br      x16                      ; jump (tail call) to new entryPoint

NESTED_END _ZN2Js21InterpreterStackFrame28DelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

;;============================================================================================================
;; DynamicProfileInfo::EnsureDynamicProfileInfoThunk
;;============================================================================================================
    ;Var DynamicProfileInfo::EnsureDynamicProfileInfoThunk(RecyclableObject* function, CallInfo callInfo, ...)
NESTED_ENTRY _ZN2Js18DynamicProfileInfo29EnsureDynamicProfileInfoThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -32   ; save parameters and volatile registers
    stp     x0, x1, [sp, #16]

    bl      C_FUNC(_ZN2Js18DynamicProfileInfo24EnsureDynamicProfileInfoEPNS_14ScriptFunctionE) ; call DynamicProfileInfo::EnsureDynamicProfileInfo
    mov     x16, x0                  ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]        ; restore parameters and volatile registers

    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 32
    br      x16                      ; jump (tail call) to new entryPoint

NESTED_END _ZN2Js18DynamicProfileInfo29EnsureDynamicProfileInfoThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

#endif

;;============================================================================================================
;; ScriptContext::ProfileModeDeferredParsingThunk
;;============================================================================================================
    ;; Var ScriptContext::ProfileModeDeferredParsingThunk(RecyclableObject* function, CallInfo callInfo, ...)
NESTED_ENTRY _ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -32   ; save parameters and volatile registers
    stp     x0, x1, [sp, #16]

    mov     x0, sp                   ; Pass the address of the function at the saved x0 in case it need to be boxed
    add     x0, x0, #16              ; 16 is subtracted from the stack pointer when the a function is called, add it back here.
    bl      C_FUNC(_ZN2Js13ScriptContext24ProfileModeDeferredParseEPPNS_14ScriptFunctionE) ; call ScriptContext::ProfileModeDeferredParse
    mov     x16, x0                  ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]        ; restore parameters and volatile registers

    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 32
    br      x16                      ; jump (tail call) to new entryPoint

NESTED_END _ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

;;============================================================================================================
;; ScriptContext::ProfileModeDeferredDeserializeThunk
;;============================================================================================================
    ;; Var ScriptContext::ProfileModeDeferredDeserializeThunk(RecyclableObject* function, CallInfo callInfo, ...)
NESTED_ENTRY _ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -32   ; save parameters and volatile registers
    stp     x0, x1, [sp, #16]

    bl      C_FUNC(_ZN2Js13ScriptContext30ProfileModeDeferredDeserializeEPNS_14ScriptFunctionE) ; call ScriptContext::ProfileModeDeferredDeserialize
    mov     x16, x0                     ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]        ; restore parameters and volatile registers

    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 32
    br      x16                      ; jump (tail call) to new entryPoint

NESTED_END _ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT
