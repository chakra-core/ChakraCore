;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Copyright (c) ChakraCore Project Contributors. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

;Var Js:JavascriptFunction::DeferredParsingThunk(function, info, values[0], values[1], ..., values[n-2], values[n-1])
;
;   This method should be called as follows
;       varResult = JavascriptFunction::DeferredParsingThunk(function, info, values[0], values[1], ..., values[n-2], values[n-1]);
;
;   and does the following:
;           entryPoint = JavascriptFunction::DeferredParse(&function);
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
;  Save and restore x0 and x1 to ensure they are the same after JavascriptFunction::DeferredParse
;  Move the stack pointer on so values on the stack remain untouched - move it back after JavascriptFunction::DeferredParse
;  Take the address of function to pass as parameter to JavascriptFunction::DeferredParse
;

#include "unixasmmacros.inc"

NESTED_ENTRY _ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -32   ; save parameter registers
    stp     x0, x1, [sp, #16]

    ; Pass the address of the function at the saved x0
    mov     x0, sp
    add     x0, x0, #16                 ; 16 is subtracted from the stack pointer when the a function is called, add it back here.
    bl      C_FUNC(_ZN2Js18JavascriptFunction13DeferredParseEPPNS_14ScriptFunctionE) ; retrieve entrypoint
    mov     x16, x0                     ; back up entryPoint in x16

    ldp     x0, x1, [sp, #16]
    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 32
    br      x16                         ; tail call to new entryPoint

NESTED_END _ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz

