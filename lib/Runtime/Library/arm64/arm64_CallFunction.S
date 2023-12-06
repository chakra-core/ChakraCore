;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Copyright (c) ChakraCore Project Contributors. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

;Var arm64_CallFunction(JavascriptFunction* function, CallInfo info, uint argCount, Var* values, JavascriptMethod entryPoint)
;
;   This method should be called as follows
;       varResult = arm64_CallFunction((JavascriptFunction*)function, args.Info, argCount, args.Values, entryPoint);
;
;   and makes the following call
;           return entryPoint(function, info, function, info, values[0], values[1], ..., values[n-2], values[n-1]);
;   where n = info.Count
;
;   ARM64 on macOS puts upto 6 named parameters into x0-x5 and all other parameters (including ...) on the stack
;   For the stack walker to work need function and info on stack as well as in x0 and x1
;   Below logic copies them and all other params to stack
;

#include "unixasmmacros.inc"

NESTED_ENTRY arm64_CallFunction, _TEXT, NoHandler

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -16    ; save FP/LR registers, implicitly stores FP in SP

    add     x5, x2, #3                          ; add 3 to param count
    lsr     x5, x5, #1                          ; divide by 2 
    sub     sp, sp, x5, lsl #4                  ; then allocate the space
    mov     x6, sp                              ; use x6 = dest
    str     x0, [x6], 8                         ; store function pointer
    str     x1, [x6], 8                         ; store info pointer

    cmp     x2, #0                              ; check for 0 params
    beq     LOCAL_LABEL(CallEntryPoint)

    LOCAL_LABEL(CopyLoop):
    subs    x2, x2, #1                          ; decrement param count by 1
    ldr     x7, [x3], #8                        ; read param from source
    str     x7, [x6], #8                        ; store param to dest
    bne     LOCAL_LABEL(CopyLoop)               ; loop until all copied

    LOCAL_LABEL(CallEntryPoint):
    blr     x4                                  ; call entry point

    mov     sp, fp                              ; explicitly restore sp
    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, 16  ; restore FP/LR
    ret                                         ; return

NESTED_END arm64_CallFunction

NESTED_ENTRY BreakSpeculation, _TEXT, NoHandler
    cmp x0, x0
    csel x0, x0, x0, eq
    ret
NESTED_END BreakSpeculation
