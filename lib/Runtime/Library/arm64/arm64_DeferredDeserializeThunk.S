;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Copyright (c) ChakraCore Project Contributors. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

;Var Js:JavascriptFunction::DeferredDeserializeThunk(function, info, values[0], values[1], ..., values[n-2], values[n-1])
;
;   This method should be called as follows
;       varResult = JavascriptFunction::DeferredDeserializeThunk(function, info, values[0], values[1], ..., values[n-2], values[n-1]);
;   
;   and does the following:
;           entryPoint = JavascriptFunction::DeferredDeserialize(function);
;           return entryPoint(function, info, values[0], values[1], ..., values[n-2], values[n-1]);
;   where n = info.Count
;
;   ARM64 calling convention (on macOS) is: //TODO verify if same on Linux
;       x0        parameter 1 = function
;       x1        parameter 2 = info
;       [sp+0]    values[0]
;       [sp+8]    values[1]
;       ...
;
;  Save and restore x0 and x1 to ensure they are the same after JavascriptFunction::DeferredDeserialize
;  Move the stack pointer on so values on the stack remain untouched - move it back after JavascriptFunction::DeferredDeserialize

#include "unixasmmacros.inc"

NESTED_ENTRY _ZN2Js18JavascriptFunction24DeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -32   ; save parameter registers
    stp     x0, x1, [sp, #16]

    bl      C_FUNC(_ZN2Js18JavascriptFunction19DeferredDeserializeEPNS_14ScriptFunctionE) ; retrieve entrypoint
    mov     x16, x0                    ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]
    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 32
    br x16                             ; tail call to new entryPoint

NESTED_END _ZN2Js18JavascriptFunction24DeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz
