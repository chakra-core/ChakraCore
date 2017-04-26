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

        sub rsp, 40h

        // spill potential floating point arguments to stack
        movaps xmmword ptr [rsp + 00h], xmm0
        movaps xmmword ptr [rsp + 10h], xmm1
        movaps xmmword ptr [rsp + 20h], xmm2
        movaps xmmword ptr [rsp + 30h], xmm3
        call C_FUNC(_ZN2Js21InterpreterStackFrame29EnsureDynamicInterpreterThunkEPNS_14ScriptFunctionE)
        // restore potential floating point arguments from stack
        movaps xmm0, xmmword ptr [rsp + 00h]
        movaps xmm1, xmmword ptr [rsp + 10h]
        movaps xmm2, xmmword ptr [rsp + 20h]
        movaps xmm3, xmmword ptr [rsp + 30h]

        add rsp, 40h

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

        sub rsp, 40h

        // spill potential floating point arguments to stack
        movaps xmmword ptr [rsp + 00h], xmm0
        movaps xmmword ptr [rsp + 10h], xmm1
        movaps xmmword ptr [rsp + 20h], xmm2
        movaps xmmword ptr [rsp + 30h], xmm3

        // save argument registers used by custom calling convention
        push rdi
        push rsi

        // get correct interpreter entrypoint
        call C_FUNC(_ZN2Js21InterpreterStackFrame29GetAsmJsInterpreterEntryPointEPNS_20AsmJsCallStackLayoutE)

        pop rsi
        pop rdi

        call rax // call appropriate template

        add rsp, 40h
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
        mov [rsp + 30h], r13

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

        // move first 4 arguments into registers.
        // don't know types other than arg0 (which is ScriptFunction *), so put in both xmm and general purpose registers
        mov rdi, r12 // arg0: func

        // int GetArgsSizesArray(ScriptFunction* func)
        // get args sizes of target asmjs function
        // rdi has ScriptFunction*
        push r13
        push rax
        push rdi
        sub rsp, 8h
        call C_FUNC(_ZN2Js17GetArgsSizesArrayEPNS_14ScriptFunctionE)
        mov r13, rax                    // r13: arg size
        add rsp, 8h
        pop rdi
        pop rax

        // NOTE: Below xmm usage is non-standard.

        // Move 3 args to regs per convention. rdi already has first arg: ScriptFunction*
        push r12
        // r12->unboxed args
        lea r12, [rsp + 18h] // rsp + size of(r12 + r13 + ScriptFunction*)

        // r13 is arg size
        cmp dword ptr [r13], 10h
        je SIMDArg2
        mov rsi, [r12]                  // arg1
        movq xmm1, qword ptr [r12]      // arg1
        add r12, 8h
        jmp Arg3
    SIMDArg2:
        movups xmm1, xmmword ptr[r12]
        add r12, 10h
    Arg3:
        cmp dword ptr [r13 + 4h], 10h
        je SIMDArg3
        mov rdx, [r12]                  // arg2
        movq xmm2, qword ptr [r12]      // arg2
        add r12, 8h
        jmp Arg4
    SIMDArg3:
        movups xmm2, xmmword ptr[r12]
        add r12, 10h
    Arg4:
        cmp dword ptr [r13 + 8h], 10h
        je SIMDArg4
        mov rcx, [r12]                  // arg3
        movq xmm3, qword ptr [r12]      // arg3
        jmp ArgsDone
   SIMDArg4:
        movups xmm3, xmmword ptr [r12]

   ArgsDone:
        pop r12         // r12: func
        pop r13         // r13: orig stack pointer

        // "home" arg0. other args were read from stack and already homed.
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
        mov r13, [rsp + 30h]

        lea  rsp, [rbp]
        pop_nonvol_reg rbp

        ret
NESTED_END _ZN2Js23AsmJsExternalEntryPointEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT


//============================================================================================================
// WasmLibrary::WasmDeferredParseExternalThunk
//============================================================================================================

// Var WasmLibrary::WasmDeferredParseExternalThunk(RecyclableObject* function, CallInfo callInfo, ...)
.balign 16
NESTED_ENTRY _ZN2Js11WasmLibrary30WasmDeferredParseExternalThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
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

        lea rdi, [rbp + 10h]
        mov rsi, 0
        call C_FUNC(_ZN2Js11WasmLibrary27WasmDeferredParseEntryPointEPPNS_19AsmJsScriptFunctionEi)

        pop r9
        pop r8
        pop rcx
        pop rdx
        pop rsi
        pop rdi

        pop_nonvol_reg rbp
        jmp rax
NESTED_END _ZN2Js11WasmLibrary30WasmDeferredParseExternalThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

//============================================================================================================
// WasmLibrary::WasmDeferredParseInternalThunk
//============================================================================================================

// Var WasmLibrary::WasmDeferredParseInternalThunk(RecyclableObject* function, CallInfo callInfo, ...)
.balign 16
NESTED_ENTRY _ZN2Js11WasmLibrary30WasmDeferredParseInternalThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT, NoHandler
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


        lea rdi, [rbp + 10h]
        mov rsi, 1
        call C_FUNC(_ZN2Js11WasmLibrary27WasmDeferredParseEntryPointEPPNS_19AsmJsScriptFunctionEi)


        pop r9
        pop r8
        pop rcx
        pop rdx
        pop rsi
        pop rdi

        pop_nonvol_reg rbp
        jmp rax
NESTED_END _ZN2Js11WasmLibrary30WasmDeferredParseInternalThunkEPNS_16RecyclableObjectENS_8CallInfoEz, _TEXT

#endif // _ENABLE_DYNAMIC_THUNKS
