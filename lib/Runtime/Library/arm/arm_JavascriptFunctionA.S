// -------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// -------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

.syntax unified
.thumb

// .thumb
.global C_FUNC(_ZN2Js18JavascriptFunction24DeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz)
.global C_FUNC(_ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz)
// .arm
.global C_FUNC(arm_CallFunction)
.global C_FUNC(arm_CallEhFrame)
.global C_FUNC(arm_CallCatch)

    NESTED_ENTRY _ZN2Js18JavascriptFunction24DeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg {r0-r3}
        push_nonvol_reg {r11}
        push_nonvol_reg {lr}

        bl      _ZN2Js18JavascriptFunction19DeferredDeserializeEPNS_14ScriptFunctionE
        mov     r12,r0              // back up entryPoint in R12

        pop_nonvol_reg {lr}
        pop_nonvol_reg {r11}
        pop_nonvol_reg {r0-r3}
        bx      r12          // jump (tail call) to new entryPoint
    NESTED_END _ZN2Js18JavascriptFunction24DeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT


    NESTED_ENTRY _ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg {r0-r3}
        push_nonvol_reg {r11}
        push_nonvol_reg {lr}


        mov     r0, sp              // Pass the address of the function at the saved r0 in case it need to be boxed
        add     r0, r0, #8          // Add 8 to compensate for the lr,r11 offset
        bl      _ZN2Js18JavascriptFunction13DeferredParseEPPNS_14ScriptFunctionE
        mov     r12,r0              // back up entryPoint in R12

        pop_nonvol_reg {lr}
        pop_nonvol_reg {r11}
        pop_nonvol_reg {r0-r3}
        bx      r12          // jump (tail call) to new entryPoint
    NESTED_END _ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

//.arm // Specifying .arm here won't work when compiling with thumb - PC will change unexpectedly with thumb offsets and land on invalid parts of arm instructions
    NESTED_ENTRY arm_CallFunction, _TEXT, NoHandler

    // IMPORTANT: the number of pushed registers (even vs odd) must be consistent with 8-bytes align check below.
    PROLOG_PUSH {r4-r7}   // r6 is saved for stack alignment, this does: add r11,sp,#10 as well
    PROLOG_PUSH {r9}  // save register we'll be using
    PROLOG_PUSH {r12}  // save register we'll be using
    PROLOG_PUSH {lr}      // save return address
    PROLOG_PUSH {r11}     // save frame pointer (and also maintain stack alignment)
#if defined(_CONTROL_FLOW_GUARD)
    PROLOG_PUSH {r8-r9}          // extra register to save info across icall check, and one register to maintain alignment
#endif
    PROLOG_STACK_SAVE r11       // mov r11,sp -- save stack frame for restoring at the epilog.

    mov     r9,r3               // copy entryPoint to r9 so we can use r3 as a scratch variable

    // All but two values go onto the stack ... calculate the number of values on the stack.
    mov     r6,r1               // r6 = callInfo.
    bfc     r6,#24,#8           // clear high order 8 bits of r6 -- clean callInfo.Flags which shares same word as callInfo.Count.
    mov     r4,r6               // r4 = callInfo. (with callInfo.Flags == 0)
    add     r2,r2,#8            // add 8 to r2 (so it is the address of the first value to be placed on the stack).
    subs    r4,r4,#2            // subtract 2 == number of values that we can pass via registers.
    ble     LOCAL_LABEL(arg2)   // goto arg2 if we have less than (or equal) 2 args
    mov     r5,r4,lsl #2        // r5 = space needed on the stack for values.

    // Adjust sp to meet ABI requirement: stack must be 8-bytes aligned at any function boundary.
    // Each reg is 4 bytes, so as we push 4 (even) regs above, r1 == odd number of regs breaks 8-byte alignment, even is OK.
    asrs    r12,r4,#1           // r-shift r1 into carry - set carry flag if r4 is odd. Note: the value in r12 is not used.
    it cs // IT block for thumb
    addcs   r4, r4, #1            // if carry is set, add space for one more argument on stack to guarantee 8-byte alignment.
    lsl     r4, r4, #2          // convert r4 from number of arguments to stack space required

    // offset stack by the amount of space we'll need.
    sub     sp,sp,r4

    mov     r12,#0                              // offset for getting values/storing on stack.

LOCAL_LABEL(argN):
    cmp     r5,r12
    beq     LOCAL_LABEL(arg2)                   // if r5 == r12, no more values need to go onto the stack.

        ldr     r3,[r2,r12]                     // r3 = *(r2 + r12)
        str     r3,[sp,r12]                     // *(sp + r12) = r3

        add     r12,r12,#4                      // offset increases by 4.
    b   LOCAL_LABEL(argN)                       // goto argN

LOCAL_LABEL(arg2):
    // Verify that the call target is valid, and process last two arguments
#if defined(_CONTROL_FLOW_GUARD)
    mov     r5, r0    // save argument registers
    mov     r6, r1
    mov     r8, r2

    mov     r0, r4    // the target address to check
    mov32   r12, __guard_check_icall_fptr
    ldr     r12, [r12]
    blx     r12

    mov     r0, r5    // restore argument registers
    mov     r1, r6
    mov     r2, r8
#endif

    // Handle 0 and 1 values
    // Push values[1] into r3 (if any)
    cmp     r6, #1
    it      gt
    ldrgt   r3,[r2,#-4]                         // r3 = *(r2-4) = values[1]

    // Push values[0] into r2 (if any)
    cmp     r6, #0
    it      gt
    ldrgt   r2,[r2,#-8]                         // r2 = *(r2-8) = values[0]

    push    {r0-r3}                             // store args from registers on stack (EPILOG_STACK_RESTORE will clean up afterwards)
    blx     r9                                  // call r9 (== entry point)

    EPILOG_STACK_RESTORE r11
#if defined(_CONTROL_FLOW_GUARD)
    EPILOG_POP {r8-r9}
#endif
    EPILOG_POP {r11}
    EPILOG_POP {lr}
    EPILOG_POP {r12}
    EPILOG_POP {r9}
    EPILOG_POP {r4-r7}
    bx lr

    NESTED_END arm_CallFunction, _TEXT


    NESTED_ENTRY arm_CallEhFrame, _TEXT, NoHandler

    // Params:
    // r0 -- thunk target
    // r1 -- frame pointer
    // r2 -- locals pointer
    // r3 -- size of stack args area

    // Home params and save registers
    // Push r11 to create the equivalent of the stack nested function list (doesn't matter what is stored there)
    // Push r12 to create the equivalent of the arguments object slot (doesn't matter what is stored there)
    PROLOG_PUSH {r0-r3}
    PROLOG_PUSH {r11}
    PROLOG_PUSH {lr}
    PROLOG_PUSH {r4-r12}
    PROLOG_VPUSH {d8-d15}
    // Save a pointer to the saved registers
    PROLOG_STACK_SAVE r4
    PROLOG_PUSH {r4}

    // Set up the frame pointer and locals pointer
    mov r7,r2
    mov r11,r1

    // Allocate the arg out area, calling chkstk if necessary
    cmp r3,4096
    bge LOCAL_LABEL(chkstk_call)
    sub sp,r3

    // Verify that the call target is valid
#if 0 && defined(_CONTROL_FLOW_GUARD)
    // we have used the r1-r3 info to set up the fake frame
    // they aren't needed by the jitted code that we're going to thunk to
    // so we don't preserve them across the __guard_check_icall_fptr call
    mov     r5, r0

    mov32   r12, __guard_check_icall_fptr
    ldr     r12, [r12]
    blx     r12

    mov     r0, r5
#endif

    // Thunk to the jitted code (and don't return)
    bx  r0

LOCAL_LABEL(chkstk_call):
    // Call chkstk, passing a DWORD count in r4
    lsr r4,r3,#2
    // bl |__chkstk|
    // r4 is now the byte count to be allocated
    sub sp,r4

    // Thunk to the jitted code (and don't return)
    bx  r0

    NESTED_END arm_CallEhFrame, _TEXT


    NESTED_ENTRY arm_CallCatch, _TEXT, NoHandler

    // Params:
    // r0 -- thunk target
    // r1 -- frame pointer
    // r2 -- locals pointer
    // r3 -- size of stack args area
    // [sp] -- exception object

    // Home params and save registers
    // Push r11 to create the equivalent of the stack nested function list (doesn't matter what is stored there)
    // Push r12 to create the equivalent of the arguments object slot (doesn't matter what is stored there)
    PROLOG_PUSH {r0-r3}
    PROLOG_PUSH {r11}
    PROLOG_PUSH {lr}
    PROLOG_PUSH {r4-r12}
    PROLOG_VPUSH {d8-d15}
    // Save a pointer to the saved registers
    PROLOG_STACK_SAVE r4
    PROLOG_PUSH {r4}

    // Set up the frame pointer and locals pointer
    mov r7,r2
    mov r11,r1

    // Load the exception object from [sp, 16 (homed params) + 44 (saved registers) + 64 (double registers) + 4 (saved SP) == 128]
    ldr r1,[sp,#128]

    // Allocate the arg out area, calling chkstk if necessary
    cmp r3,4096
    bge LOCAL_LABEL(chkstk_call_catch)
    sub sp,r3

    // Verify that the call target is valid
#if 0 && defined(_CONTROL_FLOW_GUARD)
    // we have used the r1-r3 info to set up the fake frame
    // they aren't needed by the jitted code that we're going to thunk to
    // so we don't preserve them across the __guard_check_icall_fptr call
    mov     r5, r0

    mov32   r12, __guard_check_icall_fptr
    ldr     r12, [r12]
    blx     r12

    mov     r0, r5
#endif

    // Thunk to the jitted code (and don't return)
    bx  r0

LOCAL_LABEL(chkstk_call_catch):
    // Call chkstk, passing a DWORD count in r4
    lsr r4,r3,#2
    // bl |__chkstk|
    // r4 is now the byte count to be allocated
    sub sp,r4

    // Thunk to the jitted code (and don't return)
    bx r0

    NESTED_END arm_CallCatch, _TEXT
