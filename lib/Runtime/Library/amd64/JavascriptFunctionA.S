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

        cmp r10, 0x1000  // If the space is large, make sure the stack is committed
        jl  LOCAL_LABEL(allocate_stack)
        // xplat-todo: Figure out if we need to implement __chkstk
        // call __chkstk

LOCAL_LABEL(allocate_stack):
        sub rsp, r10    // Allocate the stack space
        mov qword ptr [rsp], rdi        // function
        mov qword ptr [rsp + 0x8], rsi   // callInfo
        cmp rcx, 0
        je LOCAL_LABEL(args_setup_done)

        // Copy all args (r8) to rsp[2]. rcx has argc.
LOCAL_LABEL(copy_args_to_stack):
        lea rdi, [rsp + 0x10]            // &rsp[2]
        mov rsi, r8                     // argv
        rep movsq
        mov rdi, qword ptr [rsp]        // restore rdi
        mov rsi, qword ptr [rsp + 0x8]   // restore rsi

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
NESTED_ENTRY _ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh, _TEXT, NoHandler
        push_nonvol_reg rbp
        mov rbp, rsp

        // Set to compute CFA as: rbp + 2 words (RA, rbp)
        set_cfa_register rbp, (2*8)

        // rdi: function, rsi: entryPoint, rdx: argv, rcx: argSizes, r8: reg

        and rsp, -16 // Make sure the stack is 16 bytes aligned
        // add 16 bytes to argsSize to account for ScriptFunction and stay 16 bytes aligned
        add rcx, 16
        // Stack allocation
        sub rsp, rcx

        // Set entrypoint to r11
        mov r11, rsi

        // Make sure ScriptFunction* is first argument
        mov qword ptr [rdx], rdi

        shr rcx, 3 // rcx = rcx / 8 for qword size mov
        mov rsi, rdx // rsi = argv
        mov rdi, rsp // rdi = arguments destination
        rep movsq

        mov rdi, qword ptr [rsp]

        xor rax, rax // Zero out rax in case r11 expects varags
        call r11

// done:
        // restore stack
        mov rsp, rbp
        pop_nonvol_reg rbp
        ret
NESTED_END _ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh, _TEXT


// int64 CallAsmJsFunction<int64>(RecyclableObject *function, JavascriptMethod entryPoint, uint argc, Var *argv);
.balign 16
LEAF_ENTRY _ZN2Js18JavascriptFunction17CallAsmJsFunctionIlEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh, _TEXT
    jmp C_FUNC(_ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh)
LEAF_END _ZN2Js18JavascriptFunction17CallAsmJsFunctionIlEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh, _TEXT

// float CallAsmJsFunction<float>(RecyclableObject *function, JavascriptMethod entryPoint, uint argc, Var *argv);
.balign 16
LEAF_ENTRY _ZN2Js18JavascriptFunction17CallAsmJsFunctionIfEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh, _TEXT
    jmp C_FUNC(_ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh)
LEAF_END _ZN2Js18JavascriptFunction17CallAsmJsFunctionIfEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh, _TEXT

// double CallAsmJsFunction<double>(RecyclableObject *function, JavascriptMethod entryPoint, uint argc, Var *argv);
.balign 16
LEAF_ENTRY _ZN2Js18JavascriptFunction17CallAsmJsFunctionIdEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh, _TEXT
    jmp C_FUNC(_ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh)
LEAF_END _ZN2Js18JavascriptFunction17CallAsmJsFunctionIdEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh, _TEXT

// AsmJsSIMDValue CallAsmJsFunction<AsmJsSIMDValue>(RecyclableObject *function, JavascriptMethod entryPoint, uint argc, Var *argv);
.balign 16
LEAF_ENTRY _ZN2Js18JavascriptFunction17CallAsmJsFunctionIDv4_fEET_PNS_16RecyclableObjectEPFPvS5_NS_8CallInfoEzEjPS6_, _TEXT
    int 3  //TODO: Verify this code path when enabling WASM.SIMD for xplat
    jmp C_FUNC(_ZN2Js18JavascriptFunction17CallAsmJsFunctionIiEET_PNS_16RecyclableObjectEPFPvS4_NS_8CallInfoEzEPS5_jPh)
LEAF_END _ZN2Js18JavascriptFunction17CallAsmJsFunctionIDv4_fEET_PNS_16RecyclableObjectEPFPvS5_NS_8CallInfoEzEjPS6_, _TEXT

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
        lea rdi, [rbp + 0x10]    // &function, setup by custom calling convention
        call C_FUNC(_ZN2Js18JavascriptFunction13DeferredParseEPPNS_14ScriptFunctionE)

        pop_register rsi
        pop_register rdi

        mov rdi, qword ptr [rbp + 0x10]  // re-load function, might have been changed by DeferredParse.
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

.balign 16
NESTED_ENTRY BreakSpeculation, _TEXT, NoHandler
        cmp rdi, rdi
        cmove rax, rdi
        ret
NESTED_END BreakSpeculation, _TEXT
