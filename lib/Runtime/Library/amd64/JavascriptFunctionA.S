//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

.intel_syntax noprefix
#include "unixasmmacros.inc"

.global C_FUNC(_ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz)
.global C_FUNC(_ZN2Js18JavascriptFunction24DeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz)

#ifndef __APPLE__
.extern _ZN2Js18JavascriptFunction13DeferredParseEPPNS_14ScriptFunctionE
.extern _ZN2Js18JavascriptFunction19DeferredDeserializeEPNS_14ScriptFunctionE

.type _ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, @function
.type _ZN2Js18JavascriptFunction24DeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, @function
#endif

//------------------------------------------------------------------------------
// Invoke
//
//      JavascriptMethod(RecyclableObject* function, CallInfo callInfo, ...)
//
// with a custom calling convention in order to support JavascriptStackWalker
// and RUNTIME_ARGUMENTS. JavascriptMethod entry stack layout:
//
//      [Return Address]
//      [function]              == RDI
//      [callInfo]              == RSI
//      [arg0]
//      [arg1]
//      ...
//------------------------------------------------------------------------------
NESTED_ENTRY amd64_CallFunction, _TEXT, NoHandler
        // Var amd64_CallFunction(
        //      RecyclableObject *function,             (rdi)
        //      JavascriptMethod entryPoint,            (rsi)
        //      CallInfo callInfo,                      (rdx)
        //      uint argc,                              (rcx)
        //      Var *argv);                             (r8)

        // push rbp and adjust CFA offset
        //   CFA refers to Canonical Frame Address.
        //   See section 6.4 of DWARF spec (http://dwarfstd.org/doc/DWARF4.pdf)
        push_nonvol_reg rbp
        mov rbp, rsp

        // Set to compute CFA as: rbp + 16
        //      (16 == sizeof: [rbp] [ReturnAddress])
        // Since rbp remains unchanged, we don't need to adjust CFA offset
        // in this function.
        set_cfa_register rbp, (2*8)

        // Not using other callee-save registers: rbx/r12-r15.
        //
        // The stack is now 16 byte aligned. It was 8-byte aligned when we
        // came into the function, and since then we pushed 8 bytes onto
        // the stack.

        // We need to call the target function with the following:
        //  rdi = function
        //  rsi = CallInfo
        //  function, callInfo, and all args on stack
        //
        // Lets store the entry point in r11 and set up its first two arguments.
        // Note that since rdi is already set up, it doesn't need to change.

        mov r11, rsi    // Store entry point in r11
        mov rsi, rdx    // Store CallInfo in rsi


        mov r10, rcx    // Store argc in r10
        add r10, 3      // function, callInfo, ..., + 1 for alignment
        and r10, -2     // Mask off the lower bit to 16 byte align the stack
        shl r10, 3      // Calculate space for remaining args (* sizeof(Var*))

        cmp r10, 1000h  // If the space is large, make sure the stack is committed
        jl  LOCAL_LABEL(allocate_stack)
        // xplat-todo: Figure out if we need to implement __chkstk
        // call __chkstk

LOCAL_LABEL(allocate_stack):
        sub rsp, r10    // Allocate the stack space
        mov qword ptr [rsp], rdi        // function
        mov qword ptr [rsp + 8h], rsi   // callInfo
        cmp rcx, 0
        je LOCAL_LABEL(args_setup_done)

        // Copy all args (r8) to rsp[2]. rcx has argc.
LOCAL_LABEL(copy_args_to_stack):
        lea rdi, [rsp + 10h]            // &rsp[2]
        mov rsi, r8                     // argv
        rep movsq
        mov rdi, qword ptr [rsp]        // restore rdi
        mov rsi, qword ptr [rsp + 8h]   // restore rsi

LOCAL_LABEL(args_setup_done):
        xor rax, rax    // Zero out rax in case r11 expects varags
        call r11

LOCAL_LABEL(function_done):
        lea rsp, [rbp]
        pop_nonvol_reg rbp
        ret

NESTED_END amd64_CallFunction, _TEXT


//------------------------------------------------------------------------------
#ifdef _ENABLE_DYNAMIC_THUNKS

//extrn ?GetStackSizeForAsmJsUnboxing@Js@@YAHPEAVScriptFunction@1@@Z: PROC
//extrn ?GetArgsSizesArray@Js@@YAPEAIPEAVScriptFunction@1@@Z : PROC

// int CallAsmJsFunction<int>(RecyclableObject *function, JavascriptMethod entryPoint, uint argc, Var *argv);
.balign 16
NESTED_ENTRY _ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_, _TEXT, NoHandler
        push_nonvol_reg rbp
        mov rbp, rsp

        // Set to compute CFA as: rbp + 2 words (RA, rbp)
        set_cfa_register rbp, (2*8)

        push rbx
        push r12
        push r13
        push r15

        // rdi: function

        // rax: entry point
        mov rax, rsi

        // rbx: argc
        mov rbx, rdx

        // r15: argv
        mov r15, rcx
        add r15, 8h

        // int GetArgsSizesArray(ScriptFunction* func)
        push rax
        push rdi
        call C_FUNC(_ZN2Js17GetArgsSizesArrayEPNS_14ScriptFunctionE)
        mov r12, rax
        pop rdi
        pop rax

        // int GetStackSizeForAsmJsUnboxing(ScriptFunction* func)
        // This will return 0x20 bytes if size is below minimum. Includes space for function*.
        push rax
        push rdi
        call C_FUNC(_ZN2Js28GetStackSizeForAsmJsUnboxingEPNS_14ScriptFunctionE)
        mov r13, rax
        pop rdi
        pop rax

LOCAL_LABEL(setup_stack_and_reg_args):

        // OP_CallAsmInternal checks stack space

LOCAL_LABEL(stack_alloc):
        sub  rsp, r13

        // copy all args to the new stack frame.
        lea r11, [r15]
        lea r10, [rsp + 8]      // copy after ScriptFunction*
        push r13                // save r13
LOCAL_LABEL(copy_stack_args):
        mov rsi, qword ptr [r11]
        mov qword ptr [r10], rsi
        add r11, 8
        add r10, 8
        sub r13, 8
        cmp r13, 8  // skipped 1
        jg LOCAL_LABEL(copy_stack_args)
        pop r13                 // restore r13

        // r12 points to arg size map
LOCAL_LABEL(setup_reg_args_1):
        lea r11, [r15]
        // argc < 1 ?
        cmp rbx, 1h
        jl LOCAL_LABEL(setup_args_done)
        cmp dword ptr[r12], 10h
        je LOCAL_LABEL(SIMDArg1)
        mov rsi, qword ptr [r11]
        movq xmm1, qword ptr [r11]
        add r11, 8h
        jmp LOCAL_LABEL(setup_reg_args_2)
LOCAL_LABEL(SIMDArg1):
        movups xmm1, xmmword ptr [r11]
        add r11, 10h

LOCAL_LABEL(setup_reg_args_2):
        // argc < 2 ?
        cmp rbx, 2h
        jl LOCAL_LABEL(setup_args_done)

        add r12, 4
        cmp dword ptr[r12], 10h
        je LOCAL_LABEL(SIMDArg2)
        mov rdx, qword ptr [r11]
        movq xmm2, qword ptr [r11]
        add r11, 8h
        jmp LOCAL_LABEL(setup_reg_args_3)
LOCAL_LABEL(SIMDArg2):
        movups xmm2, xmmword ptr [r11]
        add r11, 10h

LOCAL_LABEL(setup_reg_args_3):
        // argc < 3 ?
        cmp rbx, 3h
        jl LOCAL_LABEL(setup_args_done)
        add r12, 4
        cmp dword ptr[r12], 10h
        je LOCAL_LABEL(SIMDArg3)
        mov rcx, qword ptr [r11]
        movq xmm3, qword ptr [r11]
LOCAL_LABEL(SIMDArg3):
        movups xmm3, xmmword ptr [r11]

LOCAL_LABEL(setup_args_done):
        // "home" arg0. other args already copied.
        mov [rsp], rdi
        call rax
        add rsp, r13            // restore stack

// done:
        pop r15
        pop r13
        pop r12
        pop rbx
        pop_nonvol_reg rbp
        ret
NESTED_END _ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_, _TEXT


// int64 CallAsmJsFunction<int64>(RecyclableObject *function, JavascriptMethod entryPoint, uint argc, Var *argv);
.balign 16
LEAF_ENTRY _ZN2Js18JavascriptFunction17CallAsmJsFunctionIlEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_, _TEXT
    jmp C_FUNC(_ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_)
LEAF_END _ZN2Js18JavascriptFunction17CallAsmJsFunctionIlEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_, _TEXT

// float CallAsmJsFunction<float>(RecyclableObject *function, JavascriptMethod entryPoint, uint argc, Var *argv);
.balign 16
LEAF_ENTRY _ZN2Js18JavascriptFunction17CallAsmJsFunctionIfEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_, _TEXT
    jmp C_FUNC(_ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_)
LEAF_END _ZN2Js18JavascriptFunction17CallAsmJsFunctionIfEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_, _TEXT

// double CallAsmJsFunction<double>(RecyclableObject *function, JavascriptMethod entryPoint, uint argc, Var *argv);
.balign 16
LEAF_ENTRY _ZN2Js18JavascriptFunction17CallAsmJsFunctionIdEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_, _TEXT
    jmp C_FUNC(_ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_)
LEAF_END _ZN2Js18JavascriptFunction17CallAsmJsFunctionIdEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEjPS5_, _TEXT

#endif // _ENABLE_DYNAMIC_THUNKS
//------------------------------------------------------------------------------
.balign 16
NESTED_ENTRY _ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp
        lea  rbp, [rsp]

        // save argument registers used by custom calling convention
        push_register rdi
        push_register rsi

        // Call
        //  JavascriptMethod JavascriptFunction::DeferredParse(ScriptFunction**)
        //
        lea rdi, [rbp + 10h]    // &function, setup by custom calling convention
        call C_FUNC(_ZN2Js18JavascriptFunction13DeferredParseEPPNS_14ScriptFunctionE)

        pop_register rsi
        pop_register rdi

        mov rdi, qword ptr [rbp + 10h]  // re-load function, might have been changed by DeferredParse.
                                        // e.g. StackScriptFunction is Boxed
                                        // previous push/pop rdi is for stack alignment

        pop_nonvol_reg rbp
        jmp rax

NESTED_END _ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT


//------------------------------------------------------------------------------
// Var JavascriptFunction::DeferredDeserializeThunk(
//              RecyclableObject* function, CallInfo callInfo, ...)
.balign 16
NESTED_ENTRY _ZN2Js18JavascriptFunction24DeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp
        lea  rbp, [rsp]

        // save argument registers used by custom calling convention
        push_register rdi
        push_register rsi

        // Call
        //  Js::JavascriptMethod JavascriptFunction::DeferredDeserialize(
        //                              ScriptFunction* function)
        //
        //      RDI == function, setup by custom calling convention
        call C_FUNC(_ZN2Js18JavascriptFunction19DeferredDeserializeEPNS_14ScriptFunctionE)

        pop_register rsi
        pop_register rdi

        pop_nonvol_reg rbp
        jmp rax

NESTED_END _ZN2Js18JavascriptFunction24DeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT
