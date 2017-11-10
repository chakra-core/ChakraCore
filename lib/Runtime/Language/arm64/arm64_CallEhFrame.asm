;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

;
;   arm64_CallEhFrame() and arm64_CallCatch() both thunk into jitted code at the
;   start of an EH region. The purpose is to restore the frame pointer (fp)
;   and locals pointer (x28) to the appropriate values for executing the parent
;   function and to create a local frame that can be unwound using the parent
;   function's pdata. The parent's frame looks like this:
;
;-------------------
;   {x0-x7}     -- homed parameters
;   lr          -- address from which parent was called
;   fp          -- saved frame pointer, pointed to by current fp
;   arg obj
;   {x19-x28}   -- non-volatile registers: all of them are saved
;   {q8-q15}    -- non-volatile double registers: all of them are saved
;   locals area -- pointed to by x28
;   pointer to non-volatile register area above
;   stack args
;-------------------
;
;   The reason for the "pointer to non-volatile register area" is to allow the
;   unwinder to deallocate the locals area regardless of its size. So this thunk can skip
;   the allocation of the locals area altogether, and unwinding still works.
;   The unwind pseudo-codes for the above prolog look like:
;
;   1. Deallocate stack args (sp now points to "pointer to non-volatile register area")
;   2. Restore rN (rN now points to first saved register)
;   3. Copy rN to sp (sp now points to first saved register)
;   4. Restore {q8-q15} (non-volatile double registers restored)
;   5. Restore {x19-x28} (non-volatile registers restored, sp points to saved r11)
;   6. Restore fp
;   7. Load lr into pc and deallocate remaining stack.
;
;   The prologs for the assembly thunks allocate a frame that can be unwound by executing
;   the above steps, although we don't allocate a locals area and don't know the size of the
;   stack args. The caller doesn't return to this thunk; it executes its own epilog and
;   returns to the caller of the thunk (one of the runtime try helpers).


    ; Windows version

    OPT 2       ; disable listing

#include "ksarm64.h"

    OPT 1       ; re-enable listing

    TTL Lib\Common\arm\arm64_CallEhFrame.asm

    IMPORT  __chkstk
    EXPORT  arm64_CallEhFrame
    EXPORT  arm64_CallCatch

    MACRO
    STANDARD_PROLOG

    ;
    ; Generate a prolog that will match the original function's, with all
    ; parameters homed and all non-volatile registers saved:
    ;
    ;    Size  Offset
    ;    ----  ------ 
    ;     64    176    Homed parameters
    ;     16    160    Saved FP/LR
    ;     16    144    ArgOut / stack function list
    ;     80     64    Saved x19-x28
    ;     64      0    Saved d8-d15
    ;  = 240 total
    ;
    ; The try/catch/finally blocks will jump to the epilog code skipping
    ; the instruction that deallocates the locals, in order to allow these
    ; thunks to skip re-allocating locals space.
    ;

    ; Params:
    ; x0 -- thunk target
    ; x1 -- frame pointer
    ; x2 -- locals pointer
    ; x3 -- size of stack args area
    ; x4 -- exception object (for arm64_CallCatch only)

    PROLOG_SAVE_REG_PAIR d8, d9, #-240!
    PROLOG_SAVE_REG_PAIR d10, d11, #16
    PROLOG_SAVE_REG_PAIR d12, d13, #32
    PROLOG_SAVE_REG_PAIR d14, d15, #48
    PROLOG_SAVE_REG_PAIR x19, x20, #64
    PROLOG_SAVE_REG_PAIR x21, x22, #80
    PROLOG_SAVE_REG_PAIR x23, x24, #96
    PROLOG_SAVE_REG_PAIR x25, x26, #112
    PROLOG_SAVE_REG_PAIR x27, x28, #128
    PROLOG_SAVE_REG_PAIR_NO_FP fp, lr, #160

    sub     x15, x1, x2         ; x15 = frame pointer minus locals pointer
    sub     x15, x15, #160      ; x15 -= space we already allocated
    add     x15, x15, x3        ; x15 += argout area = same stack allocation as original function
    lsr     x15, x15, #4        ; x15 /= 16
    bl      __chkstk            ; verify the allocation is ok
    sub     sp, sp, x15, lsl #4 ; allocate the stack
    
    MEND

    TEXTAREA

    NESTED_ENTRY arm64_CallEhFrame

    STANDARD_PROLOG

    ; Set up the locals pointer and frame pointer
    mov     x28, x2
    mov     fp, x1

    ; Thunk to the jitted code (and don't return)
    br      x0

    NESTED_END arm64_CallEhFrame

    ; arm64_CallCatch() is similar to arm64_CallEhFrame() except that we also pass the catch object to the jitted code

    EXPORT  arm64_CallCatch

    TEXTAREA

    NESTED_ENTRY arm64_CallCatch

    ; Params:
    ; x0 -- thunk target
    ; x1 -- frame pointer
    ; x2 -- locals pointer
    ; x3 -- size of stack args area
    ; x4 -- exception object

    STANDARD_PROLOG

    ; Set up the locals pointer and frame pointer and catch object handler
    mov     x28, x2
    mov     fp, x1
    mov     x1, x4

    ; Thunk to the jitted code (and don't return)
    br      x0

    NESTED_END arm64_CallCatch

    END
