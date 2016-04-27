//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
.intel_syntax noprefix

.extern _ZN2Js18JavascriptFunction13DeferredParseEPPNS_14ScriptFunctionE

.global amd64_CallFunction
.global	_ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz

.type amd64_CallFunction, @function
.type _ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz, @function
        
.text
amd64_CallFunction:     
        // Var amd64_CallFunction(RecyclableObject *function,
        //	JavascriptMethod entryPoint, CallInfo callInfo, uint argc, Var *argv) ;

        // Save existing rbp
        push rbp

        // Save the callee-save registers
        // This should leave the stack 16-byte aligned
        push rbx
        push r15
        push r14
        push r13
        push r12

        // 16 byte align the stack since it was 8-byte aligned when we
        // came into the function (and since then we pushed 48 bytes onto
        // the stack)
        sub  rsp, 8

        mov rbp, rsp
        
        // We need to call the target function with the following:
        //  rdi = function
        //  rsi = CallInfo
        //  rdx = argv[0]
        //  rcx = argv[1]
        //  r8  = argv[2]
        //  r9  = argv[3]
        //  Remaining args on stack
        //
        // Coming into the function, rdi = function, rsi = entrypoint, 
        // rdx = callInfo, rcx = argc, r8 = argv
        //
        // Lets store the entry point in r11 and set up its first two
        // arguments
        // (Note that since rdi is already set up, it doesn't need to
        // change)
        
        mov r11, rsi // Store entry point in r11
        mov rsi, rdx // Store CallInfo in rs1
        mov r12, r8  // Store argv in r12

        // Now check how many arguments we have
        cmp rcx, 5
        jge copy_args_to_stack
        cmp rcx, 4
        je store_arg4
        cmp rcx, 3
        je store_arg3
        cmp rcx, 2
        je store_arg2
        cmp rcx, 1
        je store_arg1
        jmp args_setup_done

        // At this point, r8 has argv, rcx has argc
copy_args_to_stack:
        mov r10, rcx   // Store argc in r10
        sub r10, 4     // Adjust for args that are passed through the registers
        and r10, -2    // Mask off the lower bit to 16 byte align the stack
        shl r10, 3     // Calculate space for remaining args (* sizeof(Var*))

        cmp r10, 1000h // If the space is large, make sure the stack is committed
        jl  allocate_stack
        // xplat-todo: Figure out if we need to implement __chkstk
        // call __chkstk

allocate_stack:
        sub rsp, r10   // Allocate the stack space
        sub r10, 8     // r10 = ((argc - 4) - 1) * 8
        lea rax, [r8 + 20h] // rax = &argv[4]

        // Copying from rax+r10 to rsp+r10
        // Can use rcx as temporary since argc is not needed anymore
copy_args:
        mov r12, qword ptr [rax + r10]
        mov qword ptr [rsp + r10], r12
        sub r10, 8h
        cmp r10, 0
        jge copy_args           

        // Restore argv back to r12
        mov r12, r8

store_arg4:
        mov r9,  qword ptr [r12+18h]
        
store_arg3:
        mov r8,  qword ptr [r12+10h]
        
store_arg2:
        mov rcx, qword ptr [r12+8h]
        
store_arg1:
        mov rdx, qword ptr [r12]

args_setup_done:
        xor rax, rax // Zero out rax in case r11 expects varags
        call r11

function_done:
        // rbp was established before the callee saves were saved
        // So just restore the stack pointer to rbp for cleaning up
        mov rsp, rbp
        add rsp, 8

        pop r12
        pop r13
        pop r14
        pop r15
        pop rbx

        pop rbp
        ret

// extrn ?DeferredParse@JavascriptFunction@Js@@SAP6APEAXPEAVRecyclableObject@2@UCallInfo@2@ZZPEAPEAVScriptFunction@2@@Z : PROC
.align 16
_ZN2Js18JavascriptFunction20DeferredParsingThunkEPNS_16RecyclableObjectENS_8CallInfoEz: 
        push rbp
        lea  rbp, [rsp]

        call _ZN2Js18JavascriptFunction13DeferredParseEPPNS_14ScriptFunctionE@plt
        lea rsp, [rbp]
        pop rbp
        
        jmp rax


