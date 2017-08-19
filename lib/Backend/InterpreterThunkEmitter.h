//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_NATIVE_CODEGEN
class ThunkBlock
{
private:
#if PDATA_ENABLED
    void* registeredPdataTable;
#endif
    BYTE*    start;
    BVFixed* freeList;
    DWORD    thunkCount;

public:
    ThunkBlock(BYTE* start, DWORD thunkCount) :
        start(start),
        thunkCount(thunkCount),
        freeList(NULL)
#if PDATA_ENABLED
        , registeredPdataTable(NULL)
#endif
    {}

    bool Contains(BYTE* address) const;
    void Release(BYTE* address);
    bool EnsureFreeList(ArenaAllocator* allocator);
    BYTE* AllocateFromFreeList();
    bool IsFreeListEmpty() const;
    BYTE* GetStart() const { return start; }

#if PDATA_ENABLED
    void* GetPdata() const { return registeredPdataTable; }
    void SetPdata(void* pdata) { Assert(!this->registeredPdataTable); this->registeredPdataTable = pdata; }
#endif

private:
    BVIndex FromThunkAddress(BYTE* address);
    BYTE* ToThunkAddress(BVIndex index);
};

//
// Emits interpreter thunks such that each javascript function in the interpreter gets a unique return address on call.
// This unique address can be used to identify the function when a stackwalk is
// performed using ETW.
//
// On every call, we generate 1 thunk (size ~ 1 page) that is implemented as a jump table.
// The shape of the thunk is as follows:
//         1. Function prolog
//         2. Determine the thunk number from the JavascriptFunction object passed as first argument.
//         3. Calculate the address of the correct calling point and jump to it.
//         4. Make the call and jump to the epilog.
//         5. Function epilog.
//
class InterpreterThunkEmitter
{
private:
    /* ------- instance methods --------*/
    InProcEmitBufferManager emitBufferManager;
    SListBase<ThunkBlock> thunkBlocks;
    SListBase<ThunkBlock> freeListedThunkBlocks;
    bool isAsmInterpreterThunk; // To emit address of InterpreterAsmThunk or InterpreterThunk
    BYTE*                thunkBuffer;
    ArenaAllocator*      allocator;
    Js::ScriptContext *  scriptContext;
    DWORD thunkCount;                      // Count of thunks available in the current thunk block

    /* -------static constants ----------*/
    // Interpreter thunk buffer includes function prolog, setting up of arguments, jumping to the appropriate calling point.
    static const BYTE ThunkAddressOffset;
    static const BYTE FunctionInfoOffset;
    static const BYTE FunctionProxyOffset;
    static const BYTE DynamicThunkAddressOffset;
    static const BYTE CallBlockStartAddrOffset;
    static const BYTE ThunkSizeOffset;
    static const BYTE ErrorOffset;
#if defined(_M_ARM)
    static const BYTE CallBlockStartAddressInstrOffset;
    static const BYTE CallThunkSizeInstrOffset;
#endif
    static const BYTE InterpreterThunk[];

    // Call buffer includes a call to the inner interpreter thunk and eventual jump to the epilog
    static const BYTE JmpOffset;
    static const BYTE Call[];

    static const BYTE Epilog[];


    static const BYTE PageCount = 1;
#if defined(_M_X64)
    static const BYTE PrologSize;
    static const BYTE StackAllocSize;
#endif

    /* ------private helpers -----------*/
    bool NewThunkBlock();

#ifdef ENABLE_OOP_NATIVE_CODEGEN
    bool NewOOPJITThunkBlock();
#endif

    static void EncodeInterpreterThunk(
        __in_bcount(thunkSize) BYTE* thunkBuffer,
        __in const intptr_t thunkBufferStartAddress,
        __in const DWORD thunkSize,
        __in const intptr_t epilogStart,
        __in const DWORD epilogSize,
        __in const intptr_t interpreterThunk);
#if defined(_M_ARM32_OR_ARM64)
    static DWORD EncodeMove(DWORD opCode, int reg, DWORD imm16);
    static void GeneratePdata(_In_ const BYTE* entryPoint, _In_ const DWORD functionSize, _Out_ RUNTIME_FUNCTION* function);
#endif

    /*-------static helpers ---------*/
    inline static DWORD FillDebugBreak(_Out_writes_bytes_all_(count) BYTE* dest, _In_ DWORD count);
    inline static DWORD CopyWithAlignment(_Out_writes_bytes_all_(sizeInBytes) BYTE* dest, _In_ const DWORD sizeInBytes, _In_reads_bytes_(srcSize) const BYTE* src, _In_ const DWORD srcSize, _In_ const DWORD alignment);
    template<class T>
    inline static void Emit(__in_bcount(sizeof(T) + offset) BYTE* dest, __in const DWORD offset, __in const T value)
    {
        AssertMsg(*(T*) (dest + offset) == 0, "Overwriting an already existing opcode?");
        *(T*)(dest + offset) = value;
    };

    BYTE* AllocateFromFreeList(PVOID* ppDynamicInterpreterThunk);
    static const BYTE _HeaderSize;
public:
    static const BYTE HeaderSize();
    static const BYTE ThunkSize;
    static const uint BlockSize= AutoSystemInfo::PageSize * PageCount;
    static void* ConvertToEntryPoint(PVOID dynamicInterpreterThunk);

    InterpreterThunkEmitter(Js::ScriptContext * context, ArenaAllocator* allocator, CustomHeap::InProcCodePageAllocators * codePageAllocators, bool isAsmInterpreterThunk = false);
    BYTE* GetNextThunk(PVOID* ppDynamicInterpreterThunk);
    SListBase<ThunkBlock>* GetThunkBlocksList();

    void Close();
    void Release(BYTE* thunkAddress, bool addtoFreeList);
    // Returns true if the argument falls within the range managed by this buffer.
#if DBG
    bool IsInHeap(void* address);
#endif
    const InProcEmitBufferManager* GetEmitBufferManager() const
    {
        return &emitBufferManager;
    }

    static void FillBuffer(
        _In_ ThreadContextInfo * context,
        _In_ bool asmJsThunk,
        _In_ intptr_t finalAddr,
        _In_ size_t bufferSize,
        _Out_writes_bytes_all_(BlockSize) BYTE* buffer,
#if PDATA_ENABLED
        _Out_ PRUNTIME_FUNCTION * pdataTableStart,
        _Out_ intptr_t * epilogEndAddr,
#endif
        _Out_ DWORD * thunkCount
    );

};
#endif
