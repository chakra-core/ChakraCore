//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
.intel_syntax noprefix
#include "unixasmmacros.inc"


#ifdef _ENABLE_DYNAMIC_THUNKS

//============================================================================================================
// InterpreterStackFrame::DelayDynamicInterpreterThunk
//============================================================================================================

// JavascriptMethod InterpreterStackFrame::EnsureDynamicInterpreterThunk(ScriptFunction * function)
// extrn _ZN2Js21InterpreterStackFrame29EnsureDynamicInterpreterThunkEPNS_14ScriptFunctionE

// Var InterpreterStackFrame::DelayDynamicInterpreterThunk(RecyclableObject* function, CallInfo callInfo, ...)
.balign 16
NESTED_ENTRY _ZN2Js21InterpreterStackFrame28DelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp
        lea  rbp, [rsp]

        // save argument registers used by custom calling convention
        push_register rdi
        push_register rsi

        //  JavascriptMethod InterpreterStackFrame::EnsureDynamicInterpreterThunk(
        //              ScriptFunction * function)
        //
        //      RDI == function, setup by custom calling convention
        call C_FUNC(_ZN2Js21InterpreterStackFrame29EnsureDynamicInterpreterThunkEPNS_14ScriptFunctionE)

        pop_register rsi
        pop_register rdi
        pop_nonvol_reg rbp

        jmp rax
NESTED_END _ZN2Js21InterpreterStackFrame28DelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT


//============================================================================================================
// InterpreterStackFrame::AsmJsDelayDynamicInterpreterThunk
//============================================================================================================

// JavascriptMethod InterpreterStackFrame::EnsureDynamicInterpreterThunk(ScriptFunction * function)
// extrn _ZN2Js21InterpreterStackFrame29EnsureDynamicInterpreterThunkEPNS_14ScriptFunctionE

// Var InterpreterStackFrame::AsmJsDelayDynamicInterpreterThunk(RecyclableObject* function, CallInfo callInfo, ...)
.balign 16
NESTED_ENTRY _ZN2Js21InterpreterStackFrame33AsmJsDelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp             // push rbp and adjust CFA offset
        lea  rbp, [rsp]

        set_cfa_register rbp, (2*8)     // Set to compute CFA as: rbp + 16 (sizeof: [rbp] [ReturnAddress])

        // save argument registers used by custom calling convention
        push rdi
        push rsi
        push rdx
        push rcx
        push r8
        push r9

        call C_FUNC(_ZN2Js11WasmLibrary20EnsureWasmEntrypointEPNS_14ScriptFunctionE)
        test rax, rax
        jne skipThunk
        mov rdi, [rbp-0x8]
        call C_FUNC(_ZN2Js21InterpreterStackFrame29EnsureDynamicInterpreterThunkEPNS_14ScriptFunctionE)
skipThunk:

        pop r9
        pop r8
        pop rcx
        pop rdx
        pop rsi
        pop rdi

        pop_nonvol_reg rbp
        jmp rax
NESTED_END _ZN2Js21InterpreterStackFrame33AsmJsDelayDynamicInterpreterThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT


//============================================================================================================
// DynamicProfileInfo::EnsureDynamicProfileInfoThunk
//============================================================================================================
// JavascriptMethod DynamicProfileInfo::EnsureDynamicProfileInfo(ScriptFunction * function)
// extrn _ZN2Js18DynamicProfileInfo24EnsureDynamicProfileInfoEPNS_14ScriptFunctionE

// Var DynamicProfileInfo::EnsureDynamicProfileInfoThunk(RecyclableObject* function, CallInfo callInfo, ...)
.balign 16
NESTED_ENTRY _ZN2Js18DynamicProfileInfo29EnsureDynamicProfileInfoThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp
        lea  rbp, [rsp]

        // save argument registers used by custom calling convention
        push_register rdi
        push_register rsi

        call C_FUNC(_ZN2Js18DynamicProfileInfo24EnsureDynamicProfileInfoEPNS_14ScriptFunctionE)

        pop_register rsi
        pop_register rdi

        pop_nonvol_reg rbp
        jmp rax
NESTED_END _ZN2Js18DynamicProfileInfo29EnsureDynamicProfileInfoThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

#endif // _ENABLE_DYNAMIC_THUNKS


//============================================================================================================
// ScriptContext::ProfileModeDeferredParsingThunk
//============================================================================================================

// Js::JavascriptMethod ScriptContext::ProfileModeDeferredParse(ScriptFunction **function)
// extrn C_FUNC(_ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz)

// Var ScriptContext::ProfileModeDeferredParsingThunk(RecyclableObject* function, CallInfo callInfo, ...)
.balign 16
NESTED_ENTRY _ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp
        lea  rbp, [rsp]

        // save argument registers used by custom calling convention
        push_register rdi
        push_register rsi

        call C_FUNC(_ZN2Js13ScriptContext24ProfileModeDeferredParseEPPNS_14ScriptFunctionE)

        pop_register rsi
        pop_register rdi

        pop_nonvol_reg rbp
        jmp rax
NESTED_END _ZN2Js13ScriptContext31ProfileModeDeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

//============================================================================================================


//============================================================================================================
// ScriptContext::ProfileModeDeferredDeserializeThunk
//============================================================================================================

// Js::JavascriptMethod ScriptContext::ProfileModeDeferredDeserialize(ScriptFunction *function)
// extrn ?ProfileModeDeferredDeserialize@ScriptContext@Js@@SAP6APEAXPEAVRecyclableObject@2@UCallInfo@2@ZZPEAVScriptFunction@2@@Z : PROC

// Var ScriptContext::ProfileModeDeferredDeserializeThunk(RecyclableObject* function, CallInfo callInfo, ...)
.balign 16
NESTED_ENTRY _ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp
        lea  rbp, [rsp]

        // save argument registers used by custom calling convention
        push_register rdi
        push_register rsi

        call C_FUNC(_ZN2Js13ScriptContext30ProfileModeDeferredDeserializeEPNS_14ScriptFunctionE)

        pop_register rsi
        pop_register rdi

        pop_nonvol_reg rbp
        jmp rax
NESTED_END _ZN2Js13ScriptContext35ProfileModeDeferredDeserializeThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT


#ifdef _ENABLE_DYNAMIC_THUNKS

//============================================================================================================
// Js::AsmJsInterpreterThunk
//============================================================================================================

// extern _ZN2Js21InterpreterStackFrame29GetAsmJsInterpreterEntryPointEPNS_20AsmJsCallStackLayoutE

// AsmJsInterpreterThunk (AsmJsCallStackLayout *function, ...)
.balign 16
NESTED_ENTRY _ZN2Js21InterpreterStackFrame19InterpreterAsmThunkEPNS_20AsmJsCallStackLayoutE, _TEXT, NoHandler
        push_nonvol_reg rbp             // push rbp and adjust CFA offset
        lea  rbp, [rsp]

        set_cfa_register rbp, (2*8)     // Set to compute CFA as: rbp + 16 (sizeof: [rbp] [ReturnAddress])


        // save argument registers used by custom calling convention
        push rdi
        push rsi

        // get correct interpreter entrypoint
        call C_FUNC(_ZN2Js21InterpreterStackFrame29GetAsmJsInterpreterEntryPointEPNS_20AsmJsCallStackLayoutE)

        pop rsi
        pop rdi

        call rax // call appropriate template

        pop_nonvol_reg rbp
        ret
NESTED_END _ZN2Js21InterpreterStackFrame19InterpreterAsmThunkEPNS_20AsmJsCallStackLayoutE, _TEXT

//============================================================================================================
// Js::AsmJsExternalEntryPoint
//============================================================================================================

//extrn ?GetStackSizeForAsmJsUnboxing@Js@@YAHPEAVScriptFunction@1@@Z: PROC
//extrn ?UnboxAsmJsArguments@Js@@YAPEAXPEAVScriptFunction@1@PEAPEAXPEADUCallInfo@1@@Z : PROC
//extrn ?BoxAsmJsReturnValue@Js@@YAPEAXPEAVScriptFunction@1@HNM@Z : PROC
//extrn ?BoxAsmJsReturnValue@Js@@YAPEAXPEAVScriptFunction@1@HNMT__m128@@@Z : PROC

//extrn ?GetArgsSizesArray@Js@@YAPEAIPEAVScriptFunction@1@@Z : PROC

// int Js::AsmJsExternalEntryPoint(RecyclableObject* entryObject, CallInfo callInfo, ...)
.balign 16
NESTED_ENTRY _ZN2Js23AsmJsExternalEntryPointEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
        push_nonvol_reg rbp             // push rbp and adjust CFA offset
        lea  rbp, [rsp]

        set_cfa_register rbp, (2*8)     // Set to compute CFA as: rbp + 16 (sizeof: [rbp] [ReturnAddress])
        
        sub rsp, 40h

        mov [rsp + 28h], r12
        .cfi_rel_offset r12, -18h
        mov [rsp + 30h], r13
        .cfi_rel_offset r13, -10h

        mov r12, rdi // r12: entryObject
        mov r13, rsi // r13: callInfo

        // allocate stack space for unboxed values
        // int GetStackSizeForAsmJsUnboxing(ScriptFunction* func)
        call C_FUNC(_ZN2Js28GetStackSizeForAsmJsUnboxingEPNS_14ScriptFunctionE)

        mov rcx, r13 // arg4: callInfo
        mov rsi, rsp // arg2: orig stack pointer is arg for the unboxing helper
        mov r13, rsi // r13: save orig stack pointer, so that we can add it back later
        add rsi, 68h // account for the changes we have already made to rsp

        sub rsp, rax // allocate additional stack space for args
        // UnboxAsmJsArguments(func, origArgsLoc, argDst, callInfo)
        mov rdi, r12 // arg1: func
        mov rdx, rsp // arg3: argDst

        // unboxing function also does stack probe
        call C_FUNC(_ZN2Js19UnboxAsmJsArgumentsEPNS_14ScriptFunctionEPPvPcNS_8CallInfoE)
        // rax = target function address

        mov rdi, r12 // arg0: func

        mov [rsp + 00h], rdi

        // call entry point
        call rax

        // Var BoxAsmJsReturnValue(ScriptFunction* func, int intRetVal, double doubleRetVal, float floatRetVal)
        mov rdi, r12            // arg0: func
        mov rsi, rax            // arg1: intRetVal
        // movsd xmm0, xmm0     // arg2: doubleRetVal
        movss xmm1, xmm0        // arg3: floatRetVal


        // store SIMD xmm value and pointer to it as argument to box function
        sub rsp, 20h
        movups [rsp + 10h], xmm0
        lea r12, [rsp + 10h]
        mov qword ptr [rsp], r12
        call C_FUNC(_ZN2Js19BoxAsmJsReturnValueEPNS_14ScriptFunctionEldfDv4_f)

        mov rsp, r13 // restore stack pointer
    Epilogue:
        mov r12, [rsp + 28h]
        .cfi_restore r12
        mov r13, [rsp + 30h]
        .cfi_restore r13

        lea  rsp, [rbp]
        pop_nonvol_reg rbp

        ret
NESTED_END _ZN2Js23AsmJsExternalEntryPointEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

#endif // _ENABLE_DYNAMIC_THUNKS
