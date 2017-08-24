//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

#if ENABLE_NATIVE_CODEGEN
#ifdef _M_X64
#ifdef _WIN32
const BYTE InterpreterThunkEmitter::FunctionInfoOffset = 23;
const BYTE InterpreterThunkEmitter::FunctionProxyOffset = 27;
const BYTE InterpreterThunkEmitter::DynamicThunkAddressOffset = 31;
const BYTE InterpreterThunkEmitter::CallBlockStartAddrOffset = 41;
const BYTE InterpreterThunkEmitter::ThunkSizeOffset = 55;
const BYTE InterpreterThunkEmitter::ErrorOffset = 64;
const BYTE InterpreterThunkEmitter::ThunkAddressOffset = 81;

const BYTE InterpreterThunkEmitter::PrologSize = 80;
const BYTE InterpreterThunkEmitter::StackAllocSize = 0x28;

//
// Home the arguments onto the stack and pass a pointer to the base of the stack location to the inner thunk
//
// Calling convention requires that caller should allocate at least 0x20 bytes and the stack be 16 byte aligned.
// Hence, we allocate 0x28 bytes of stack space for the callee to use. The callee uses 8 bytes to push the first
// argument and the rest 0x20 ensures alignment is correct.
//
const BYTE InterpreterThunkEmitter::InterpreterThunk[] = {
    0x48, 0x89, 0x54, 0x24, 0x10,                                  // mov         qword ptr [rsp+10h],rdx
    0x48, 0x89, 0x4C, 0x24, 0x08,                                  // mov         qword ptr [rsp+8],rcx
    0x4C, 0x89, 0x44, 0x24, 0x18,                                  // mov         qword ptr [rsp+18h],r8
    0x4C, 0x89, 0x4C, 0x24, 0x20,                                  // mov         qword ptr [rsp+20h],r9
    0x48, 0x8B, 0x41, 0x00,                                        // mov         rax, qword ptr [rcx+FunctionInfoOffset]
    0x48, 0x8B, 0x48, 0x00,                                        // mov         rcx, qword ptr [rax+FunctionProxyOffset]
    0x48, 0x8B, 0x51, 0x00,                                        // mov         rdx, qword ptr [rcx+DynamicThunkAddressOffset]
                                                                   // Range Check for Valid call target
    0x48, 0x83, 0xE2, 0xF8,                                        // and         rdx, 0xFFFFFFFFFFFFFFF8h  ;Force 8 byte alignment
    0x48, 0x8b, 0xca,                                              // mov         rcx, rdx
    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    // mov         rax, CallBlockStartAddress
    0x48, 0x2b, 0xc8,                                              // sub         rcx, rax
    0x48, 0x81, 0xf9, 0x00, 0x00, 0x00, 0x00,                      // cmp         rcx, ThunkSize
    0x76, 0x09,                                                    // jbe         $safe
    0x48, 0xc7, 0xc1, 0x00, 0x00, 0x00, 0x00,                      // mov         rcx, errorcode
    0xcd, 0x29,                                                    // int         29h

    // $safe:
    0x48, 0x8D, 0x4C, 0x24, 0x08,                                  // lea         rcx, [rsp+8]                ;Load the address to stack
    0x48, 0x83, 0xEC, StackAllocSize,                              // sub         rsp,28h
    0x48, 0xB8, 0x00, 0x00, 0x00 ,0x00, 0x00, 0x00, 0x00, 0x00,    // mov         rax, <thunk>
    0xFF, 0xE2,                                                    // jmp         rdx
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC                                   // int         3                           ;for alignment to size of 8 we are adding this
};

const BYTE InterpreterThunkEmitter::Epilog[] = {
    0x48, 0x83, 0xC4, StackAllocSize,                              // add         rsp,28h
    0xC3                                                           // ret
};


#else  // Sys V AMD64
const BYTE InterpreterThunkEmitter::FunctionInfoOffset = 7;
const BYTE InterpreterThunkEmitter::FunctionProxyOffset = 11;
const BYTE InterpreterThunkEmitter::DynamicThunkAddressOffset = 15;
const BYTE InterpreterThunkEmitter::CallBlockStartAddrOffset = 25;
const BYTE InterpreterThunkEmitter::ThunkSizeOffset = 39;
const BYTE InterpreterThunkEmitter::ErrorOffset = 48;
const BYTE InterpreterThunkEmitter::ThunkAddressOffset = 61;

const BYTE InterpreterThunkEmitter::PrologSize = 60;
const BYTE InterpreterThunkEmitter::StackAllocSize = 0x0;

const BYTE InterpreterThunkEmitter::InterpreterThunk[] = {
    0x55,                                                       // push   rbp                   // Prolog - setup the stack frame
    0x48, 0x89, 0xe5,                                           // mov    rbp, rsp
    0x48, 0x8b, 0x47, 0x00,                                     // mov    rax, qword ptr [rdi + FunctionInfoOffset]
    0x48, 0x8B, 0x48, 0x00,                                     // mov    rcx, qword ptr [rax+FunctionProxyOffset]
    0x48, 0x8B, 0x51, 0x00,                                     // mov    rdx, qword ptr [rcx+DynamicThunkAddressOffset]
                                                                                                // Range Check for Valid call target
    0x48, 0x83, 0xE2, 0xF8,                                     // and    rdx, 0xfffffffffffffff8   // Force 8 byte alignment
    0x48, 0x89, 0xd1,                                           // mov    rcx, rdx
    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov    rax, CallBlockStartAddress
    0x48, 0x29, 0xc1,                                           // sub    rcx, rax
    0x48, 0x81, 0xf9, 0x00, 0x00, 0x00, 0x00,                   // cmp    rcx, ThunkSize
    0x76, 0x09,                                                 // jbe    safe
    0x48, 0xc7, 0xc1, 0x00, 0x00, 0x00, 0x00,                   // mov    rcx, errorcode
    0xcd, 0x29,                                                 // int    29h       <-- xplat TODO: just to exit

    // safe:
    0x48, 0x8d, 0x7c, 0x24, 0x10,                               // lea    rdi, [rsp+0x10]
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov    rax, <thunk>          // stack already 16-byte aligned
    0xff, 0xe2,                                                 // jmp    rdx
    0xcc                                                        // int    3                     // for alignment to size of 8
};

const BYTE InterpreterThunkEmitter::Epilog[] = {
    0x5d,                                                       // pop    rbp
    0xc3                                                        // ret
};
#endif
#elif defined(_M_ARM)
const BYTE InterpreterThunkEmitter::ThunkAddressOffset = 8;
const BYTE InterpreterThunkEmitter::FunctionInfoOffset = 18;
const BYTE InterpreterThunkEmitter::FunctionProxyOffset = 22;
const BYTE InterpreterThunkEmitter::DynamicThunkAddressOffset = 26;
const BYTE InterpreterThunkEmitter::CallBlockStartAddressInstrOffset = 42;
const BYTE InterpreterThunkEmitter::CallThunkSizeInstrOffset = 54;
const BYTE InterpreterThunkEmitter::ErrorOffset = 64;

const BYTE InterpreterThunkEmitter::InterpreterThunk[] = {
    0x0F, 0xB4,                                                      // push        {r0-r3}
    0x2D, 0xE9, 0x00, 0x48,                                          // push        {r11,lr}
    0xEB, 0x46,                                                      // mov         r11,sp
    0x00, 0x00, 0x00, 0x00,                                          // movw        r1,ThunkAddress
    0x00, 0x00, 0x00, 0x00,                                          // movt        r1,ThunkAddress
    0xD0, 0xF8, 0x00, 0x20,                                          // ldr.w       r2,[r0,#0x00]
    0xD2, 0xF8, 0x00, 0x00,                                          // ldr.w       r0,[r2,#0x00]
    0xD0, 0xF8, 0x00, 0x30,                                          // ldr.w       r3,[r0,#0x00]
    0x4F, 0xF6, 0xF9, 0x70,                                          // mov         r0,#0xFFF9
    0xCF, 0xF6, 0xFF, 0x70,                                          // movt        r0,#0xFFFF
    0x03, 0xEA, 0x00, 0x03,                                          // and         r3,r3,r0
    0x18, 0x46,                                                      // mov         r0, r3
    0x00, 0x00, 0x00, 0x00,                                          // movw        r12, CallBlockStartAddress
    0x00, 0x00, 0x00, 0x00,                                          // movt        r12, CallBlockStartAddress
    0xA0, 0xEB, 0x0C, 0x00,                                          // sub         r0, r12
    0x00, 0x00, 0x00, 0x00,                                          // mov         r12, ThunkSize
    0x60, 0x45,                                                      // cmp         r0, r12
    0x02, 0xD9,                                                      // bls         $safe
    0x4F, 0xF0, 0x00, 0x00,                                          // mov         r0, errorcode
    0xFB, 0xDE,                                                      // Equivalent to int 0x29

    //$safe:
    0x02, 0xA8,                                                      // add         r0,sp,#8
    0x18, 0x47                                                       // bx          r3
};

const BYTE InterpreterThunkEmitter::JmpOffset = 2;

const BYTE InterpreterThunkEmitter::Call[] = {
    0x88, 0x47,                                                      // blx         r1
    0x00, 0x00, 0x00, 0x00,                                          // b.w         epilog
    0xFE, 0xDE,                                                      // int         3       ;Required for alignment
};

const BYTE InterpreterThunkEmitter::Epilog[] = {
    0x5D, 0xF8, 0x04, 0xBB,                                         // pop         {r11}
    0x5D, 0xF8, 0x14, 0xFB                                          // ldr         pc,[sp],#0x14
};
#elif defined(_M_ARM64)
const BYTE InterpreterThunkEmitter::FunctionInfoOffset = 24;
const BYTE InterpreterThunkEmitter::FunctionProxyOffset = 28;
const BYTE InterpreterThunkEmitter::DynamicThunkAddressOffset = 32;
const BYTE InterpreterThunkEmitter::ThunkAddressOffset = 36;

//TODO: saravind :Implement Range Check for ARM64
const BYTE InterpreterThunkEmitter::InterpreterThunk[] = {
    0xFD, 0x7B, 0xBB, 0xA9,                                         //stp         fp, lr, [sp, #-80]!   ;Prologue
    0xFD, 0x03, 0x00, 0x91,                                         //mov         fp, sp                ;update frame pointer to the stack pointer
    0xE0, 0x07, 0x01, 0xA9,                                         //stp         x0, x1, [sp, #16]     ;Prologue again; save all registers
    0xE2, 0x0F, 0x02, 0xA9,                                         //stp         x2, x3, [sp, #32]
    0xE4, 0x17, 0x03, 0xA9,                                         //stp         x4, x5, [sp, #48]
    0xE6, 0x1F, 0x04, 0xA9,                                         //stp         x6, x7, [sp, #64]
    0x02, 0x00, 0x40, 0xF9,                                         //ldr         x2, [x0, #0x00]       ;offset will be replaced with Offset of FunctionInfo
    0x40, 0x00, 0x40, 0xF9,                                         //ldr         x0, [x2, #0x00]       ;offset will be replaced with Offset of FunctionProxy
    0x03, 0x00, 0x40, 0xF9,                                         //ldr         x3, [x0, #0x00]       ;offset will be replaced with offset of DynamicInterpreterThunk
                                                                    //Following 4 MOV Instrs are to move the 64-bit address of the InterpreterThunk address into register x1.
    0x00, 0x00, 0x00, 0x00,                                         //movz        x1, #0x00             ;This is overwritten with the actual thunk address(16 - 0 bits) move
    0x00, 0x00, 0x00, 0x00,                                         //movk        x1, #0x00, lsl #16    ;This is overwritten with the actual thunk address(32 - 16 bits) move
    0x00, 0x00, 0x00, 0x00,                                         //movk        x1, #0x00, lsl #32    ;This is overwritten with the actual thunk address(48 - 32 bits) move
    0x00, 0x00, 0x00, 0x00,                                         //movk        x1, #0x00, lsl #48    ;This is overwritten with the actual thunk address(64 - 48 bits) move
    0xE0, 0x43, 0x00, 0x91,                                         //add         x0, sp, #16
    0x60, 0x00, 0x1F, 0xD6                                          //br          x3
};

const BYTE InterpreterThunkEmitter::JmpOffset = 4;

const BYTE InterpreterThunkEmitter::Call[] = {
    0x20, 0x00, 0x3f, 0xd6,                                         // blr         x1
    0x00, 0x00, 0x00, 0x00                                          // b           epilog
};

const BYTE InterpreterThunkEmitter::Epilog[] = {
    0xfd, 0x7b, 0xc5, 0xa8,                                         // ldp         fp, lr, [sp], #80
    0xc0, 0x03, 0x5f, 0xd6                                          // ret
};
#else
const BYTE InterpreterThunkEmitter::FunctionInfoOffset = 8;
const BYTE InterpreterThunkEmitter::FunctionProxyOffset = 11;
const BYTE InterpreterThunkEmitter::DynamicThunkAddressOffset = 14;
const BYTE InterpreterThunkEmitter::CallBlockStartAddrOffset = 21;
const BYTE InterpreterThunkEmitter::ThunkSizeOffset = 26;
const BYTE InterpreterThunkEmitter::ErrorOffset = 33;
const BYTE InterpreterThunkEmitter::ThunkAddressOffset = 44;

const BYTE InterpreterThunkEmitter::InterpreterThunk[] = {
    0x55,                                                           //   push        ebp                ;Prolog - setup the stack frame
    0x8B, 0xEC,                                                     //   mov         ebp,esp
    0x8B, 0x45, 0x08,                                               //   mov         eax, dword ptr [ebp+8]
    0x8B, 0x40, 0x00,                                               //   mov         eax, dword ptr [eax+FunctionInfoOffset]
    0x8B, 0x40, 0x00,                                               //   mov         eax, dword ptr [eax+FunctionProxyOffset]
    0x8B, 0x48, 0x00,                                               //   mov         ecx, dword ptr [eax+DynamicThunkAddressOffset]
                                                                    //   Range Check for Valid call target
    0x83, 0xE1, 0xF8,                                               //   and         ecx, 0FFFFFFF8h
    0x8b, 0xc1,                                                     //   mov         eax, ecx
    0x2d, 0x00, 0x00, 0x00, 0x00,                                   //   sub         eax, CallBlockStartAddress
    0x3d, 0x00, 0x00, 0x00, 0x00,                                   //   cmp         eax, ThunkSize
    0x76, 0x07,                                                     //   jbe         SHORT $safe
    0xb9, 0x00, 0x00, 0x00, 0x00,                                   //   mov         ecx, errorcode
    0xCD, 0x29,                                                     //   int         29h

    //$safe
    0x8D, 0x45, 0x08,                                               //   lea         eax, ebp+8
    0x50,                                                           //   push        eax
    0xB8, 0x00, 0x00, 0x00, 0x00,                                   //   mov         eax, <thunk>
    0xFF, 0xE1,                                                     //   jmp         ecx
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC                              //   int 3 for 8byte alignment
};

const BYTE InterpreterThunkEmitter::Epilog[] = {
    0x5D,                                                         // pop ebp
    0xC3                                                          // ret
};
#endif

#if defined(_M_X64) || defined(_M_IX86)
const BYTE InterpreterThunkEmitter::JmpOffset = 3;

const BYTE InterpreterThunkEmitter::Call[] = {
    0xFF, 0xD0,                                                // call       rax
    0xE9, 0x00, 0x00, 0x00, 0x00,                              // jmp        [offset]
    0xCC,                                                      // int 3      ;for alignment to size of 8 we are adding this
};

#endif

const BYTE InterpreterThunkEmitter::_HeaderSize = sizeof(InterpreterThunk);
const BYTE InterpreterThunkEmitter::ThunkSize = sizeof(Call);

const BYTE InterpreterThunkEmitter::HeaderSize()
{

    return _HeaderSize;
}

InterpreterThunkEmitter::InterpreterThunkEmitter(Js::ScriptContext* context, ArenaAllocator* allocator, CustomHeap::InProcCodePageAllocators * codePageAllocators, bool isAsmInterpreterThunk) :
    emitBufferManager(allocator, codePageAllocators, /*scriptContext*/ nullptr, _u("Interpreter thunk buffer"), GetCurrentProcess()),
    scriptContext(context),
    allocator(allocator),
    thunkCount(0),
    thunkBuffer(nullptr),
    isAsmInterpreterThunk(isAsmInterpreterThunk)
{
}

SListBase<ThunkBlock>* 
InterpreterThunkEmitter::GetThunkBlocksList()
{
    return &thunkBlocks;
}

//
// Returns the next thunk. Batch allocated PageCount pages of thunks and issue them one at a time
//
BYTE* InterpreterThunkEmitter::GetNextThunk(PVOID* ppDynamicInterpreterThunk)
{
    Assert(ppDynamicInterpreterThunk);
    Assert(*ppDynamicInterpreterThunk == nullptr);

    if(thunkCount == 0)
    {
        if(!this->freeListedThunkBlocks.Empty())
        {
            return AllocateFromFreeList(ppDynamicInterpreterThunk);
        }
        if (!NewThunkBlock())
        {
#ifdef ASMJS_PLAT
            return this->isAsmInterpreterThunk ? (BYTE*)&Js::InterpreterStackFrame::StaticInterpreterAsmThunk : (BYTE*)&Js::InterpreterStackFrame::StaticInterpreterThunk;
#else
            Assert(!this->isAsmInterpreterThunk);
            return (BYTE*)&Js::InterpreterStackFrame::StaticInterpreterThunk;
#endif
        }
    }

    Assert(this->thunkBuffer != nullptr);
    BYTE* thunk = this->thunkBuffer;
#if _M_ARM
    thunk = (BYTE*)((DWORD)thunk | 0x01);
#endif
    *ppDynamicInterpreterThunk = thunk + HeaderSize() + ((--thunkCount) * ThunkSize);
#if _M_ARM
    AssertMsg(((uintptr_t)(*ppDynamicInterpreterThunk) & 0x6) == 0, "Not 8 byte aligned?");
#else
    AssertMsg(((uintptr_t)(*ppDynamicInterpreterThunk) & 0x7) == 0, "Not 8 byte aligned?");
#endif
    return thunk;
}

//
// Interpreter thunks have an entrypoint at the beginning of the page boundary. Each function has a unique thunk return address
// and this function can convert to the unique thunk return address to the beginning of the page which corresponds with the entrypoint
//
void* InterpreterThunkEmitter::ConvertToEntryPoint(PVOID dynamicInterpreterThunk)
{
    Assert(dynamicInterpreterThunk != nullptr);
    void* entryPoint = (void*)((size_t)dynamicInterpreterThunk & (~((size_t)(BlockSize) - 1)));

#if _M_ARM
    entryPoint = (BYTE*)((DWORD)entryPoint | 0x01);
#endif
    return entryPoint;
}

bool InterpreterThunkEmitter::NewThunkBlock()
{
#ifdef ENABLE_OOP_NATIVE_CODEGEN
    if (CONFIG_FLAG(ForceStaticInterpreterThunk))
    {
        return false;
    }

    if (JITManager::GetJITManager()->IsOOPJITEnabled())
    {
        return NewOOPJITThunkBlock();
    }
#endif

    Assert(this->thunkCount == 0);
    BYTE* buffer;

    EmitBufferAllocation<VirtualAllocWrapper, PreReservedVirtualAllocWrapper> * allocation = emitBufferManager.AllocateBuffer(BlockSize, &buffer);
    if (allocation == nullptr)
    {
        Js::Throw::OutOfMemory();
    }
    if (!emitBufferManager.ProtectBufferWithExecuteReadWriteForInterpreter(allocation))
    {
        Js::Throw::OutOfMemory();
    }

#if PDATA_ENABLED
    PRUNTIME_FUNCTION pdataStart = nullptr;
    intptr_t epilogEnd = 0;
#endif

    DWORD count = this->thunkCount;
    FillBuffer(
        this->scriptContext->GetThreadContext(),
        this->isAsmInterpreterThunk,
        (intptr_t)buffer,
        BlockSize,
        buffer,
#if PDATA_ENABLED
        &pdataStart,
        &epilogEnd,
#endif
        &count
    );

    if (!emitBufferManager.CommitBufferForInterpreter(allocation, buffer, BlockSize))
    {
        Js::Throw::OutOfMemory();
    }

    // Call to set VALID flag for CFG check
    ThreadContext::GetContextForCurrentThread()->SetValidCallTargetForCFG(buffer);

    // Update object state only at the end when everything has succeeded - and no exceptions can be thrown.
    auto block = this->thunkBlocks.PrependNode(allocator, buffer, count);
#if PDATA_ENABLED
    void* pdataTable;
    PDataManager::RegisterPdata((PRUNTIME_FUNCTION)pdataStart, (ULONG_PTR)buffer, (ULONG_PTR)epilogEnd, &pdataTable);
    block->SetPdata(pdataTable);
#else
    Unused(block);
#endif
    this->thunkBuffer = buffer;
    this->thunkCount = count;
    return true;
}

#ifdef ENABLE_OOP_NATIVE_CODEGEN
bool InterpreterThunkEmitter::NewOOPJITThunkBlock()
{
    PSCRIPTCONTEXT_HANDLE remoteScriptContext = this->scriptContext->GetRemoteScriptAddr();
    if (!JITManager::GetJITManager()->IsConnected())
    {
        return false;
    }
    InterpreterThunkInputIDL thunkInput;
    thunkInput.asmJsThunk = this->isAsmInterpreterThunk;

    InterpreterThunkOutputIDL thunkOutput;
    HRESULT hr = JITManager::GetJITManager()->NewInterpreterThunkBlock(remoteScriptContext, &thunkInput, &thunkOutput);
    if (!JITManager::HandleServerCallResult(hr, RemoteCallType::ThunkCreation))
    {
        return false;
    }

    BYTE* buffer = (BYTE*)thunkOutput.mappedBaseAddr;

    if (!CONFIG_FLAG(OOPCFGRegistration))
    {
        this->scriptContext->GetThreadContext()->SetValidCallTargetForCFG(buffer);
    }

    // Update object state only at the end when everything has succeeded - and no exceptions can be thrown.
    auto block = this->thunkBlocks.PrependNode(allocator, buffer, thunkOutput.thunkCount);
#if PDATA_ENABLED
    void* pdataTable;
    PDataManager::RegisterPdata((PRUNTIME_FUNCTION)thunkOutput.pdataTableStart, (ULONG_PTR)thunkOutput.mappedBaseAddr, (ULONG_PTR)thunkOutput.epilogEndAddr, &pdataTable);
    block->SetPdata(pdataTable);
#else
    Unused(block);
#endif

    this->thunkBuffer = (BYTE*)thunkOutput.mappedBaseAddr;
    this->thunkCount = thunkOutput.thunkCount;
    return true;
}
#endif

/* static */
void InterpreterThunkEmitter::FillBuffer(
    _In_ ThreadContextInfo * threadContext,
    _In_ bool asmJsThunk,
    _In_ intptr_t finalAddr,
    _In_ size_t bufferSize,
    _Out_writes_bytes_all_(BlockSize) BYTE* buffer,
#if PDATA_ENABLED
    _Out_ PRUNTIME_FUNCTION * pdataTableStart,
    _Out_ intptr_t * epilogEndAddr,
#endif
    _Out_ DWORD * thunkCount
    )
{
#ifdef _M_X64
    PrologEncoder prologEncoder;
    prologEncoder.EncodeSmallProlog(PrologSize, StackAllocSize);
    DWORD pdataSize = prologEncoder.SizeOfPData();
#elif defined(_M_ARM32_OR_ARM64)
    DWORD pdataSize = sizeof(RUNTIME_FUNCTION);
#else
    DWORD pdataSize = 0;
#endif
    DWORD bytesRemaining = BlockSize;
    DWORD bytesWritten = 0;
    DWORD thunks = 0;
    DWORD epilogSize = sizeof(Epilog);
    const BYTE *epilog = Epilog;
    const BYTE *header = InterpreterThunk;

    intptr_t interpreterThunk;

    // the static interpreter thunk invoked by the dynamic emitted thunk
#ifdef ASMJS_PLAT
    if (asmJsThunk)
    {
        interpreterThunk = ShiftAddr(threadContext, &Js::InterpreterStackFrame::InterpreterAsmThunk);
    }
    else
#endif
    {
        interpreterThunk = ShiftAddr(threadContext, &Js::InterpreterStackFrame::InterpreterThunk);
    }


    BYTE * currentBuffer = buffer;
    // Ensure there is space for PDATA at the end
    BYTE* pdataStart = currentBuffer + (BlockSize - Math::Align(pdataSize, EMIT_BUFFER_ALIGNMENT));
    BYTE* epilogStart = pdataStart - Math::Align(epilogSize, EMIT_BUFFER_ALIGNMENT);

    // Ensure there is space for PDATA at the end
    intptr_t finalPdataStart = finalAddr + (BlockSize - Math::Align(pdataSize, EMIT_BUFFER_ALIGNMENT));
    intptr_t finalEpilogStart = finalPdataStart - Math::Align(epilogSize, EMIT_BUFFER_ALIGNMENT);

    // Copy the thunk buffer and modify it.
    js_memcpy_s(currentBuffer, bytesRemaining, header, HeaderSize());
    EncodeInterpreterThunk(currentBuffer, finalAddr, HeaderSize(), finalEpilogStart, epilogSize, interpreterThunk);
    currentBuffer += HeaderSize();
    bytesRemaining -= HeaderSize();

    // Copy call buffer
    DWORD callSize = sizeof(Call);
    while (currentBuffer < epilogStart - callSize)
    {
        js_memcpy_s(currentBuffer, bytesRemaining, Call, callSize);
#if _M_ARM
        int offset = (epilogStart - (currentBuffer + JmpOffset));
        Assert(offset >= 0);
        DWORD encodedOffset = EncoderMD::BranchOffset_T2_24(offset);
        DWORD encodedBranch = /*opcode=*/ 0x9000F000 | encodedOffset;
        Emit(currentBuffer, JmpOffset, encodedBranch);
#elif _M_ARM64
        int64 offset = (epilogStart - (currentBuffer + JmpOffset));
        Assert(offset >= 0);
        DWORD encodedOffset = EncoderMD::BranchOffset_26(offset);
        DWORD encodedBranch = /*opcode=*/ 0x14000000 | encodedOffset;
        Emit(currentBuffer, JmpOffset, encodedBranch);
#else
        // jump requires an offset from the end of the jump instruction.
        int offset = (int)(epilogStart - (currentBuffer + JmpOffset + sizeof(int)));
        Assert(offset >= 0);
        Emit(currentBuffer, JmpOffset, offset);
#endif
        currentBuffer += callSize;
        bytesRemaining -= callSize;
        thunks++;
    }

    // Fill any gap till start of epilog
    bytesWritten = FillDebugBreak(currentBuffer, (DWORD)(epilogStart - currentBuffer));
    bytesRemaining -= bytesWritten;
    currentBuffer += bytesWritten;

    // Copy epilog
    bytesWritten = CopyWithAlignment(currentBuffer, bytesRemaining, epilog, epilogSize, EMIT_BUFFER_ALIGNMENT);
    currentBuffer += bytesWritten;
    bytesRemaining -= bytesWritten;

    // Generate and register PDATA
#if PDATA_ENABLED
    BYTE* epilogEnd = epilogStart + epilogSize;
    DWORD functionSize = (DWORD)(epilogEnd - buffer);
    Assert(pdataStart == currentBuffer);
#ifdef _M_X64
    Assert(bytesRemaining >= pdataSize);
    BYTE* pdata = prologEncoder.Finalize(buffer, functionSize, pdataStart);
    bytesWritten = CopyWithAlignment(pdataStart, bytesRemaining, pdata, pdataSize, EMIT_BUFFER_ALIGNMENT);
#elif defined(_M_ARM32_OR_ARM64)
    RUNTIME_FUNCTION pdata;
    GeneratePdata(buffer, functionSize, &pdata);
    bytesWritten = CopyWithAlignment(pdataStart, bytesRemaining, (const BYTE*)&pdata, pdataSize, EMIT_BUFFER_ALIGNMENT);
#endif
    *pdataTableStart = (PRUNTIME_FUNCTION)finalPdataStart;
    *epilogEndAddr = finalEpilogStart;
#endif
    *thunkCount = thunks;
}

#if _M_ARM
void InterpreterThunkEmitter::EncodeInterpreterThunk(
    __in_bcount(thunkSize) BYTE* thunkBuffer,
    __in const intptr_t thunkBufferStartAddress,
    __in const DWORD thunkSize,
    __in const intptr_t epilogStart,
    __in const DWORD epilogSize,
    __in const intptr_t interpreterThunk)
{
    _Analysis_assume_(thunkSize == HeaderSize());
    // Encode MOVW
    DWORD lowerThunkBits = (uint32)interpreterThunk & 0x0000FFFF;
    DWORD movW = EncodeMove(/*Opcode*/ 0x0000F240, /*register*/1, lowerThunkBits);
    Emit(thunkBuffer,ThunkAddressOffset, movW);

    // Encode MOVT
    DWORD higherThunkBits = ((uint32)interpreterThunk & 0xFFFF0000) >> 16;
    DWORD movT = EncodeMove(/*Opcode*/ 0x0000F2C0, /*register*/1, higherThunkBits);
    Emit(thunkBuffer, ThunkAddressOffset + sizeof(movW), movT);

    // Encode LDR - Load of function Body
    thunkBuffer[FunctionInfoOffset] = Js::JavascriptFunction::GetOffsetOfFunctionInfo();
    thunkBuffer[FunctionProxyOffset] = Js::FunctionInfo::GetOffsetOfFunctionProxy();

    // Encode LDR - Load of interpreter thunk number
    thunkBuffer[DynamicThunkAddressOffset] = Js::FunctionBody::GetOffsetOfDynamicInterpreterThunk();

    // Encode MOVW R12, CallBlockStartAddress
    uintptr_t callBlockStartAddress = (uintptr_t)thunkBufferStartAddress + HeaderSize();
    uint totalThunkSize = (uint)(epilogStart - callBlockStartAddress);

    DWORD lowerCallBlockStartAddress = callBlockStartAddress & 0x0000FFFF;
    DWORD movWblockStart = EncodeMove(/*Opcode*/ 0x0000F240, /*register*/12, lowerCallBlockStartAddress);
    Emit(thunkBuffer,CallBlockStartAddressInstrOffset, movWblockStart);

    // Encode MOVT R12, CallBlockStartAddress
    DWORD higherCallBlockStartAddress = (callBlockStartAddress & 0xFFFF0000) >> 16;
    DWORD movTblockStart = EncodeMove(/*Opcode*/ 0x0000F2C0, /*register*/12, higherCallBlockStartAddress);
    Emit(thunkBuffer, CallBlockStartAddressInstrOffset + sizeof(movWblockStart), movTblockStart);

    //Encode MOV R12, CallBlockSize
    DWORD movBlockSize = EncodeMove(/*Opcode*/ 0x0000F240, /*register*/12, (DWORD)totalThunkSize);
    Emit(thunkBuffer, CallThunkSizeInstrOffset, movBlockSize);

    Emit(thunkBuffer, ErrorOffset, (BYTE) FAST_FAIL_INVALID_ARG);
}

DWORD InterpreterThunkEmitter::EncodeMove(DWORD opCode, int reg, DWORD imm16)
{
    DWORD encodedMove = reg << 24;
    DWORD encodedImm = 0;
    EncoderMD::EncodeImmediate16(imm16, &encodedImm);
    encodedMove |= encodedImm;
    AssertMsg((encodedMove & opCode) == 0, "Any bits getting overwritten?");
    encodedMove |= opCode;
    return encodedMove;
}

void InterpreterThunkEmitter::GeneratePdata(_In_ const BYTE* entryPoint, _In_ const DWORD functionSize, _Out_ RUNTIME_FUNCTION* function)
{
    function->BeginAddress   = 0x1;                        // Since our base address is the start of the function - this is offset from the base address
    function->Flag           = 1;                          // Packed unwind data is used
    function->FunctionLength = functionSize / 2;
    function->Ret            = 0;                          // Return via Pop
    function->H              = 1;                          // Homes parameters
    function->Reg            = 7;                          // No saved registers - R11 is the frame pointer - not considered here
    function->R              = 1;                          // No registers are being saved.
    function->L              = 1;                          // Save/restore LR register
    function->C              = 1;                          // Frame pointer chain in R11 established
    function->StackAdjust    = 0;                          // Stack allocation for the function
}

#elif _M_ARM64
void InterpreterThunkEmitter::EncodeInterpreterThunk(
    __in_bcount(thunkSize) BYTE* thunkBuffer,
    __in const intptr_t thunkBufferStartAddress,
    __in const DWORD thunkSize,
    __in const intptr_t epilogStart,
    __in const DWORD epilogSize,
    __in const intptr_t interpreterThunk)
{
    int addrOffset = ThunkAddressOffset;

    _Analysis_assume_(thunkSize == HeaderSize());
    AssertMsg(thunkSize == HeaderSize(), "Mismatch in the size of the InterpreterHeaderThunk and the thunkSize used in this API (EncodeInterpreterThunk)");

    // Following 4 MOV Instrs are to move the 64-bit address of the InterpreterThunk address into register x1.

    // Encode MOVZ (movz        x1, #<interpreterThunk 16-0 bits>)
    DWORD lowerThunkBits = (uint64)interpreterThunk & 0x0000FFFF;
    DWORD movZ = EncodeMove(/*Opcode*/ 0xD2800000, /*register x1*/1, lowerThunkBits); // no shift; hw = 00
    Emit(thunkBuffer,addrOffset, movZ);
    AssertMsg(sizeof(movZ) == 4, "movZ has to be 32-bit encoded");
    addrOffset+= sizeof(movZ);

    // Encode MOVK (movk        x1, #<interpreterThunk 32-16 bits>, lsl #16)
    DWORD higherThunkBits = ((uint64)interpreterThunk & 0xFFFF0000) >> 16;
    DWORD movK = EncodeMove(/*Opcode*/ 0xF2A00000, /*register x1*/1, higherThunkBits); // left shift 16 bits; hw = 01
    Emit(thunkBuffer, addrOffset, movK);
    AssertMsg(sizeof(movK) == 4, "movK has to be 32-bit encoded");
    addrOffset+= sizeof(movK);

    // Encode MOVK (movk        x1, #<interpreterThunk 48-32 bits>, lsl #16)
    higherThunkBits = ((uint64)interpreterThunk & 0xFFFF00000000) >> 32;
    movK = EncodeMove(/*Opcode*/ 0xF2C00000, /*register x1*/1, higherThunkBits); // left shift 32 bits; hw = 02
    Emit(thunkBuffer, addrOffset, movK);
    AssertMsg(sizeof(movK) == 4, "movK has to be 32-bit encoded");
    addrOffset += sizeof(movK);

    // Encode MOVK (movk        x1, #<interpreterThunk 64-48 bits>, lsl #16)
    higherThunkBits = ((uint64)interpreterThunk & 0xFFFF000000000000) >> 48;
    movK = EncodeMove(/*Opcode*/ 0xF2E00000, /*register x1*/1, higherThunkBits); // left shift 48 bits; hw = 03
    AssertMsg(sizeof(movK) == 4, "movK has to be 32-bit encoded");
    Emit(thunkBuffer, addrOffset, movK);

    // Encode LDR - Load of function Body
    ULONG offsetOfFunctionInfo = Js::JavascriptFunction::GetOffsetOfFunctionInfo();
    AssertMsg(offsetOfFunctionInfo % 8 == 0, "Immediate offset for LDR must be 8 byte aligned");
    AssertMsg(offsetOfFunctionInfo < 0x8000, "Immediate offset for LDR must be less than 0x8000");
    *(PULONG)&thunkBuffer[FunctionInfoOffset] |= (offsetOfFunctionInfo / 8) << 10;

    ULONG offsetOfFunctionProxy = Js::FunctionInfo::GetOffsetOfFunctionProxy();
    AssertMsg(offsetOfFunctionProxy % 8 == 0, "Immediate offset for LDR must be 8 byte aligned");
    AssertMsg(offsetOfFunctionProxy < 0x8000, "Immediate offset for LDR must be less than 0x8000");
    *(PULONG)&thunkBuffer[FunctionProxyOffset] |= (offsetOfFunctionProxy / 8) << 10;

    // Encode LDR - Load of interpreter thunk number
    ULONG offsetOfDynamicInterpreterThunk = Js::FunctionBody::GetOffsetOfDynamicInterpreterThunk();
    AssertMsg(offsetOfDynamicInterpreterThunk % 8 == 0, "Immediate offset for LDR must be 8 byte aligned");
    AssertMsg(offsetOfDynamicInterpreterThunk < 0x8000, "Immediate offset for LDR must be less than 0x8000");
    *(PULONG)&thunkBuffer[DynamicThunkAddressOffset] |= (offsetOfDynamicInterpreterThunk / 8) << 10;
}

DWORD InterpreterThunkEmitter::EncodeMove(DWORD opCode, int reg, DWORD imm16)
{
    DWORD encodedMove = reg << 0;
    DWORD encodedImm = 0;
    EncoderMD::EncodeImmediate16(imm16, &encodedImm);
    encodedMove |= encodedImm;
    AssertMsg((encodedMove & opCode) == 0, "Any bits getting overwritten?");
    encodedMove |= opCode;
    return encodedMove;
}

void InterpreterThunkEmitter::GeneratePdata(_In_ const BYTE* entryPoint, _In_ const DWORD functionSize, _Out_ RUNTIME_FUNCTION* function)
{
    function->BeginAddress = 0x0;               // Since our base address is the start of the function - this is offset from the base address
    function->Flag = 1;                         // Packed unwind data is used
    function->FunctionLength = functionSize / 4;
    function->RegF = 0;                         // number of non-volatile FP registers (d8-d15) saved in the canonical stack location
    function->RegI = 0;                         // number of non-volatile INT registers (r19-r28) saved in the canonical stack location
    function->H = 1;                            // Homes parameters
    // (indicating whether the function "homes" the integer parameter registers (r0-r7) by storing them at the very start of the function)

    function->CR = 3;                           // chained function, a store/load pair instruction is used in prolog/epilog <r29,lr>
    function->FrameSize = 5;                    // the number of bytes of stack that is allocated for this function divided by 16
}
#else
void InterpreterThunkEmitter::EncodeInterpreterThunk(
    __in_bcount(thunkSize) BYTE* thunkBuffer,
    __in const intptr_t thunkBufferStartAddress,
    __in const DWORD thunkSize,
    __in const intptr_t epilogStart,
    __in const DWORD epilogSize,
    __in const intptr_t interpreterThunk)
{
    _Analysis_assume_(thunkSize == HeaderSize());


    Emit(thunkBuffer, ThunkAddressOffset, (uintptr_t)interpreterThunk);
    thunkBuffer[DynamicThunkAddressOffset] = Js::FunctionBody::GetOffsetOfDynamicInterpreterThunk();
    thunkBuffer[FunctionInfoOffset] = Js::JavascriptFunction::GetOffsetOfFunctionInfo();
    thunkBuffer[FunctionProxyOffset] = Js::FunctionInfo::GetOffsetOfFunctionProxy();
    Emit(thunkBuffer, CallBlockStartAddrOffset, (uintptr_t) thunkBufferStartAddress + HeaderSize());
    uint totalThunkSize = (uint)(epilogStart - (thunkBufferStartAddress + HeaderSize()));
    Emit(thunkBuffer, ThunkSizeOffset, totalThunkSize);
    Emit(thunkBuffer, ErrorOffset, (BYTE) FAST_FAIL_INVALID_ARG);
}
#endif

/*static*/
DWORD InterpreterThunkEmitter::FillDebugBreak(_Out_writes_bytes_all_(count) BYTE* dest, _In_ DWORD count)
{
#if defined(_M_ARM)
    Assert(count % 2 == 0);
#elif defined(_M_ARM64)
    Assert(count % 4 == 0);
#endif
    CustomHeap::FillDebugBreak(dest, count);
    return count;
}

/*static*/
DWORD InterpreterThunkEmitter::CopyWithAlignment(
    _Out_writes_bytes_all_(sizeInBytes) BYTE* dest,
    _In_ const DWORD sizeInBytes,
    _In_reads_bytes_(srcSize) const BYTE* src,
    _In_ const DWORD srcSize,
    _In_ const DWORD alignment)
{
    js_memcpy_s(dest, sizeInBytes, src, srcSize);
    dest += srcSize;

    DWORD alignPad = Math::Align(srcSize, alignment) - srcSize;
    Assert(alignPad <= (sizeInBytes - srcSize));
    if(alignPad > 0 && alignPad <= (sizeInBytes - srcSize))
    {
        FillDebugBreak(dest, alignPad);
        return srcSize + alignPad;
    }
    return srcSize;
}

#if DBG
bool
InterpreterThunkEmitter::IsInHeap(void* address)
{
#ifdef ENABLE_OOP_NATIVE_CODEGEN
    if (JITManager::GetJITManager()->IsOOPJITEnabled())
    {
        PSCRIPTCONTEXT_HANDLE remoteScript = this->scriptContext->GetRemoteScriptAddr(false);
        if (!remoteScript || !JITManager::GetJITManager()->IsConnected())
        {
            // this method is used in asserts to validate whether an entry point is valid
            // in case JIT process crashed, let's just say true to keep asserts from firing
            return true;
        }
        boolean result;
        HRESULT hr = JITManager::GetJITManager()->IsInterpreterThunkAddr(remoteScript, (intptr_t)address, this->isAsmInterpreterThunk, &result);
        if (!JITManager::HandleServerCallResult(hr, RemoteCallType::HeapQuery))
        {
            return true;
        }
        return result != FALSE;
    }
    else
#endif
    {
        return emitBufferManager.IsInHeap(address);
    }
}
#endif

// We only decommit at close because there might still be some
// code running here.
// The destructor of emitBufferManager will cause the eventual release.
void InterpreterThunkEmitter::Close()
{
#if PDATA_ENABLED
    auto unregisterPdata = ([&] (const ThunkBlock& block)
    {
        PDataManager::UnregisterPdata((PRUNTIME_FUNCTION) block.GetPdata());
    });
    thunkBlocks.Iterate(unregisterPdata);
    freeListedThunkBlocks.Iterate(unregisterPdata);
#endif

    this->thunkBlocks.Clear(allocator);
    this->freeListedThunkBlocks.Clear(allocator);

#ifdef ENABLE_OOP_NATIVE_CODEGEN
    if (JITManager::GetJITManager()->IsOOPJITEnabled())
    {
        PSCRIPTCONTEXT_HANDLE remoteScript = this->scriptContext->GetRemoteScriptAddr(false);
        if (remoteScript && JITManager::GetJITManager()->IsConnected())
        {
            JITManager::GetJITManager()->DecommitInterpreterBufferManager(remoteScript, this->isAsmInterpreterThunk);
        }
    }
    else
#endif
    {
        emitBufferManager.Decommit();
    }


    this->thunkBuffer = nullptr;
    this->thunkCount = 0;
}

void InterpreterThunkEmitter::Release(BYTE* thunkAddress, bool addtoFreeList)
{
    if(!addtoFreeList)
    {
        return;
    }

    auto predicate = ([=] (const ThunkBlock& block)
    {
        return block.Contains(thunkAddress);
    });

    ThunkBlock* block = freeListedThunkBlocks.Find(predicate);
    if(!block)
    {
        block = thunkBlocks.MoveTo(&freeListedThunkBlocks, predicate);
    }

    // if EnsureFreeList fails in an OOM scenario - we just leak the thunks
    if(block && block->EnsureFreeList(allocator))
    {
        block->Release(thunkAddress);
    }
}

BYTE* InterpreterThunkEmitter::AllocateFromFreeList(PVOID* ppDynamicInterpreterThunk )
{
    ThunkBlock& block = this->freeListedThunkBlocks.Head();
    BYTE* thunk = block.AllocateFromFreeList();
#if _M_ARM
    thunk = (BYTE*)((DWORD)thunk | 0x01);
#endif
    if(block.IsFreeListEmpty())
    {
        this->freeListedThunkBlocks.MoveHeadTo(&this->thunkBlocks);
    }
    *ppDynamicInterpreterThunk = thunk;
    BYTE* entryPoint = block.GetStart();
#if _M_ARM
    entryPoint = (BYTE*)((DWORD)entryPoint | 0x01);
#endif
    return entryPoint;
}


bool ThunkBlock::Contains(BYTE* address) const
{
    bool contains = address >= start && address < (start + InterpreterThunkEmitter::BlockSize);
    return contains;
}

void ThunkBlock::Release(BYTE* address)
{
    Assert(Contains(address));
    Assert(this->freeList);

    BVIndex index = FromThunkAddress(address);
    this->freeList->Set(index);
}

BYTE* ThunkBlock::AllocateFromFreeList()
{
    Assert(this->freeList);
    BVIndex index = this->freeList->GetNextBit(0);
    BYTE* address = ToThunkAddress(index);
    this->freeList->Clear(index);
    return address;
}

BVIndex ThunkBlock::FromThunkAddress(BYTE* address)
{
    uint index = ((uint)(address - start) - InterpreterThunkEmitter::HeaderSize()) / InterpreterThunkEmitter::ThunkSize;
    Assert(index < this->thunkCount);
    return index;
}

BYTE* ThunkBlock::ToThunkAddress(BVIndex index)
{
    Assert(index < this->thunkCount);
    BYTE* address = start + InterpreterThunkEmitter::HeaderSize() + InterpreterThunkEmitter::ThunkSize * index;
    return address;
}

bool ThunkBlock::EnsureFreeList(ArenaAllocator* allocator)
{
    if(!this->freeList)
    {
        this->freeList = BVFixed::NewNoThrow(this->thunkCount, allocator);
    }
    return this->freeList != nullptr;
}

bool ThunkBlock::IsFreeListEmpty() const
{
    Assert(this->freeList);
    return this->freeList->IsAllClear();
}

#endif
