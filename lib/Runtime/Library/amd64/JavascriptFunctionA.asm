;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------
include ksamd64.inc

        _TEXT SEGMENT

extrn __chkstk: PROC
ifdef _CONTROL_FLOW_GUARD
extrn __guard_check_icall_fptr:QWORD
extrn __guard_dispatch_icall_fptr:QWORD

        subttl  "Control Flow Guard ICall Check Stub"
;++
;
; Routine Description:
;
;   This routine is a stub that is called for the CFG icall check.  Its
;   function is to create a new stack frame for (*__guard_check_icall_fptr)
;   which performs the actual indirect call check.
;
;   N.B. A new stack frame is required since amd64_CallFunction requires the
;         parameter home area of the icall check subroutine to be preserved.
;         (The saved non-volatiles in amd64_CallFunction would overlap with
;          the *guard_check_icall_fptr home parameter region.)
;
;   N.B. The (*guard_check_icall_fptr) call is guaranteed to preserve rcx.
;        This stub preserves that behavior, allowing callers to assume rcx
;        is preserved across the call.
;
; Arguments:
;
;   ICallTarget (rcx) - Supplies a pointer to a function to check.
;
; Implicit Arguments:
;
;   (rsp+08h - rsp+30h)  - Supplies the preserved home area.
;
; Return Value:
;
;   None.  Should the indirect call check fail, a fast fail event is raised.
;
;--

IcFrame struct
        P1Home  dq ?                    ; child function parameter home addresses
        P2Home  dq ?                    ;
        P3Home  dq ?                    ;
        P4Home  dq ?                    ;
        Fill    dq ?                    ;
IcFrame ends

        NESTED_ENTRY amd64_CheckICall, _TEXT$00

        alloc_stack (sizeof IcFrame)    ; allocate stack frame

        END_PROLOGUE

        call    [__guard_check_icall_fptr] ; verify that the call target is valid

        add     rsp, (sizeof IcFrame)   ; deallocate stack frame

        ret                             ; return to dispatch invoke

        NESTED_END amd64_CheckICall, _TEXT$00

endif

align 16
amd64_CallFunction PROC FRAME

        ;;
        ;;  Stack layout: A, B, C indicate what the stack looks like at specific
        ;;  points in code.
        ;;
        ;;            ----------------------------
        ;;             argv
        ;;  rbp + 48h ----------------------------
        ;;             argc       [r9]             -\
        ;;  rbp + 40h ----------------------------  |
        ;;             callInfo   [r8]              |
        ;;  rbp + 38h ----------------------------  -- argument register spill
        ;;             entryPoint [rdx]             |
        ;;  rbp + 30h ----------------------------  |
        ;;             function   [rcx]            -/
        ;;  rbp + 28h ----------------------------
        ;;             return address
        ;;  rbp + 20h ---------------------------- <-- (A) function entry
        ;;             rbx                         -\
        ;;  rbp + 18h ----------------------------  |
        ;;             rsi                          |
        ;;  rbp + 10h ----------------------------  -- saved non-volatile registers
        ;;             rdi                          |
        ;;  rbp + 08h ----------------------------  |
        ;;             rbp                         -/
        ;;  rbp       ----------------------------  <-- (B) frame pointer established
        ;;             padding
        ;;            ----------------------------
        ;;            ~                          ~
        ;;            ~ (argc&1)?(argc+1):argc   ~
        ;;            ~ QWORDS                   ~ <-- argv[2] ... argv[N - 1] + padding
        ;;            ~                          ~
        ;;            ----------------------------
        ;;             argv[1]    [r9]             -\
        ;;            ----------------------------  |
        ;;             argv[0]    [r8]              |
        ;;            ----------------------------  -- argument register spill
        ;;             callInfo   [rdx]             |
        ;;            ----------------------------  |
        ;;             function   [rcx]            -/
        ;;            ---------------------------- <-- (C) callsite
        ;;

        ;; (A) function entry

        push rbx
        .pushreg rbx
        push rsi
        .pushreg rsi
        push rdi
        .pushreg rdi
        push rbp
        .pushreg rbp
        lea rbp, [rsp]
        .setframe rbp, 0
        .endprolog

        ;; (B) frame pointer established

        ;; The first 4 QWORD args are passed in rcx, rdx, r8 and r9. rcx = function *,
        ;; rdx = CallInfo.
        ;; upon entry rcx contains function *.
        sub rsp, 8h

        ;; rbx = argc
        mov rbx, r9

        ;; save entry point (rdx) and move CallInfo (r8) into rdx.
        mov rax, rdx
        mov rdx, r8

        mov r10, 0

        ;; rsi = argv
        mov rsi, qword ptr [rsp + 50h]

        ;; If argc > 2 then r8 = argv[0] and r9 = argv[1]. The rest are copied onto
        ;; the stack.
        cmp rbx, 2h
        jg  setup_stack_and_reg_args
        je  setup_reg_args_2
        cmp rbx, 1h
        je  setup_reg_args_1
        jmp setup_args_done

        ;; *args labels handle copying the script args (argv) into either registers or the stack.
setup_stack_and_reg_args:
        ;; calculate the number of args to be copied onto the stack. Adjust the allocation
        ;; size so that the stack pointer is a multiple of 10h at the callsite.
        mov r10, rbx
        and r10, -2

        ;; Calculate the size of stack to be allocated in bytes and allocate.
        push rax
        mov  rax, r10
        shl  rax, 3h

        ;; Call __chkstk to ensure the stack is extended properly. It expects size in rax.
        cmp  rax, 1000h
        jl   stack_alloc
        call __chkstk

stack_alloc:
        mov  r10, rax
        pop  rax
        sub  rsp, r10

        ;; (rsp[0]..rsp[N - 3]) = (argv[2]..argv[N - 1])
        mov r10, rbx            ; r10 = N - 1 i.e. (argc - 2) - 1
        sub r10, 3h
        shl r10, 3h
        lea r11, [rsi + 10h]
copy_stack_args:
        mov rdi, qword ptr [r11 + r10]
        mov qword ptr [rsp + r10], rdi
        sub r10, 8h
        cmp r10, 0
        jge copy_stack_args

        ;; The first two script args are passed in r8 and r9.
setup_reg_args_2:
        mov r9, qword ptr [rsi + 8h]
setup_reg_args_1:
        mov r8, qword ptr [rsi]

setup_args_done:
        ;; allocate args register spill
        sub rsp, 20h

ifdef _CONTROL_FLOW_GUARD
        call    [__guard_dispatch_icall_fptr]
else
        ;; (C) callsite
        call rax
endif
done:
        mov rsp, rbp
        pop rbp
        pop rdi
        pop rsi
        pop rbx
        ret

amd64_CallFunction ENDP

ifdef _ENABLE_DYNAMIC_THUNKS

extrn ?GetStackSizeForAsmJsUnboxing@Js@@YAHPEAVScriptFunction@1@@Z: PROC
extrn ?GetArgsSizesArray@Js@@YAPEAIPEAVScriptFunction@1@@Z : PROC

; int64 JavascriptFunction::CallAsmJsFunction<int64>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
align 16
??$CallAsmJsFunction@_J@JavascriptFunction@Js@@SA_JPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z PROC FRAME
    .setframe rbp, 0
    .endprolog
    rex_jmp_reg ??$CallAsmJsFunction@H@JavascriptFunction@Js@@SAHPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z
??$CallAsmJsFunction@_J@JavascriptFunction@Js@@SA_JPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z ENDP

; float JavascriptFunction::CallAsmJsFunction<float>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
align 16
??$CallAsmJsFunction@N@JavascriptFunction@Js@@SANPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z PROC FRAME
    .setframe rbp, 0
    .endprolog
    rex_jmp_reg ??$CallAsmJsFunction@H@JavascriptFunction@Js@@SAHPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z
??$CallAsmJsFunction@N@JavascriptFunction@Js@@SANPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z ENDP

; double JavascriptFunction::CallAsmJsFunction<double>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
align 16
??$CallAsmJsFunction@M@JavascriptFunction@Js@@SAMPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z PROC FRAME
    .setframe rbp, 0
    .endprolog
    rex_jmp_reg ??$CallAsmJsFunction@H@JavascriptFunction@Js@@SAHPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z
??$CallAsmJsFunction@M@JavascriptFunction@Js@@SAMPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z ENDP

; __m128 JavascriptFunction::CallAsmJsFunction<__m128>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
align 16
??$CallAsmJsFunction@T__m128@@@JavascriptFunction@Js@@SA?AT__m128@@PEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z PROC FRAME
    .setframe rbp, 0
    .endprolog
    rex_jmp_reg ??$CallAsmJsFunction@H@JavascriptFunction@Js@@SAHPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z
??$CallAsmJsFunction@T__m128@@@JavascriptFunction@Js@@SA?AT__m128@@PEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z ENDP

; int JavascriptFunction::CallAsmJsFunction<int>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
align 16
??$CallAsmJsFunction@H@JavascriptFunction@Js@@SAHPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z PROC FRAME
        ; save arguments to stack for interpreter
        mov qword ptr [rsp + 8h],  rcx ;; function
        mov qword ptr [rsp + 10h], rdx ;; entrypoint
        mov qword ptr [rsp + 18h], r8 ;; argv
        mov qword ptr [rsp + 20h], r9 ;; argsSize
        ;; reg is at [rsp + 28h]

        ; push rbx unused
        ; .pushreg rbx
        push rsi
        .pushreg rsi
        push rdi
        .pushreg rdi
        ; push r12 unused
        ; .pushreg r12
        ; push r13 unused
        ; .pushreg r13
        push rbp
        .pushreg rbp
        mov rbp, rsp
        .setframe rbp, 0
        .endprolog

        and rsp, -16 ; Make sure the stack is 16 bytes aligned
        lea rax, [r9 + 16] ; add 16 bytes to argsSize to account for ScriptFunction and stay 16 bytes aligned

        ; Check if we need to commit more stack
        cmp rax, 2000h ; x64 has 2 guard pages
        jl stack_alloc
        call __chkstk
stack_alloc:
        sub rsp, rax

        ;; Make sure ScriptFunction* is first argument
        mov qword ptr [r8], rcx

        ;; copy all args to the new stack frame.
        ;; Move argSize in rcx for rep movs
        mov rcx, rax
        shr rcx, 3 ;; rcx = rcx / 8 for qword size mov
        mov rsi, r8 ;; rsi = argv
        mov rdi, rsp ;; rdi = arguments destination
        rep movsq

        ;; Move entrypoint in rax
        mov rax, rdx

        ;; Load 4 first arguments in registers
        ;; First argument (aka ScriptFunction*)
        mov rcx, qword ptr [rsp]
        mov r10, [rbp + 40h] ;; r10 = byte* reg
        ;; Second argument
        mov rdx, qword ptr [r10]
        movaps xmm1, xmmword ptr [r10]
        ;; Third argument
        mov r8, qword ptr [r10 + 10h]
        movaps xmm2, xmmword ptr [r10 + 10h]
        ;; Fourth argument
        mov r9, qword ptr [r10 + 20h]
        movaps xmm3, xmmword ptr [r10 + 20h]

ifdef _CONTROL_FLOW_GUARD
        call    [__guard_dispatch_icall_fptr]
else
        call rax
endif

        lea rsp, [rbp]
        pop rbp
        ; pop r13
        ; pop r12
        pop rdi
        pop rsi
        ; pop rbx
        ret

??$CallAsmJsFunction@H@JavascriptFunction@Js@@SAHPEAVRecyclableObject@1@P6APEAX0UCallInfo@1@ZZPEAPEAXIPEAE@Z ENDP

endif ;; _ENABLE_DYNAMIC_THUNKS

extrn ?DeferredParse@JavascriptFunction@Js@@SAP6APEAXPEAVRecyclableObject@2@UCallInfo@2@ZZPEAPEAVScriptFunction@2@@Z : PROC
align 16
?DeferredParsingThunk@JavascriptFunction@Js@@SAPEAXPEAVRecyclableObject@2@UCallInfo@2@ZZ PROC FRAME
        ;; save volatile registers
        mov qword ptr [rsp + 8h],  rcx
        mov qword ptr [rsp + 10h], rdx
        mov qword ptr [rsp + 18h], r8
        mov qword ptr [rsp + 20h], r9

        push rbp
        .pushreg rbp
        lea  rbp, [rsp]
        .setframe rbp, 0
        .endprolog

        sub rsp, 20h
        lea rcx, [rsp + 30h]
        call ?DeferredParse@JavascriptFunction@Js@@SAP6APEAXPEAVRecyclableObject@2@UCallInfo@2@ZZPEAPEAVScriptFunction@2@@Z

ifdef _CONTROL_FLOW_GUARD
        mov rcx, rax                            ; __guard_check_icall_fptr requires the call target in rcx.
        call [__guard_check_icall_fptr]         ; verify that the call target is valid
        mov rax, rcx                            ;restore call target
endif

        add rsp, 20h

        lea rsp, [rbp]
        pop rbp

        ;; restore volatile registers
        mov rcx, qword ptr [rsp + 8h]
        mov rdx, qword ptr [rsp + 10h]
        mov r8,  qword ptr [rsp + 18h]
        mov r9,  qword ptr [rsp + 20h]

        rex_jmp_reg rax
?DeferredParsingThunk@JavascriptFunction@Js@@SAPEAXPEAVRecyclableObject@2@UCallInfo@2@ZZ ENDP

extrn ?DeferredDeserialize@JavascriptFunction@Js@@SAP6APEAXPEAVRecyclableObject@2@UCallInfo@2@ZZPEAVScriptFunction@2@@Z : PROC
align 16
?DeferredDeserializeThunk@JavascriptFunction@Js@@SAPEAXPEAVRecyclableObject@2@UCallInfo@2@ZZ PROC FRAME
        ;; save volatile registers
        mov qword ptr [rsp + 8h],  rcx
        mov qword ptr [rsp + 10h], rdx
        mov qword ptr [rsp + 18h], r8
        mov qword ptr [rsp + 20h], r9

        push rbp
        .pushreg rbp
        lea  rbp, [rsp]
        .setframe rbp, 0
        .endprolog

        sub rsp, 20h
        call ?DeferredDeserialize@JavascriptFunction@Js@@SAP6APEAXPEAVRecyclableObject@2@UCallInfo@2@ZZPEAVScriptFunction@2@@Z

ifdef _CONTROL_FLOW_GUARD
        mov rcx, rax                            ; __guard_check_icall_fptr requires the call target in rcx.
        call [__guard_check_icall_fptr]         ; verify that the call target is valid
        mov rax, rcx                            ;restore call target
endif

        add rsp, 20h

        lea rsp, [rbp]
        pop rbp

        ;; restore volatile registers
        mov rcx, qword ptr [rsp + 8h]
        mov rdx, qword ptr [rsp + 10h]
        mov r8,  qword ptr [rsp + 18h]
        mov r9,  qword ptr [rsp + 20h]

        rex_jmp_reg rax
?DeferredDeserializeThunk@JavascriptFunction@Js@@SAPEAXPEAVRecyclableObject@2@UCallInfo@2@ZZ ENDP

align 16
BreakSpeculation PROC
        cmp rcx, rcx
        cmove rax, rcx
        ret
BreakSpeculation ENDP

        _TEXT ENDS
        end
