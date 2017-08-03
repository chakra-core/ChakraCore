//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if defined(ENABLE_NATIVE_CODEGEN) && defined(_CONTROL_FLOW_GUARD) && (_M_IX86 || _M_X64)
template <typename TAlloc>
class JITThunkEmitter
{
private:
    static const BYTE DirectJmp[];
    static const uint DirectJmpTargetOffset = 1;
    static const uint DirectJmpIPAdjustment = 5;
#if _M_AMD64
    static const BYTE IndirectJmp[];
    static const uint IndirectJmpTargetOffset = 2;
#endif
    static const uint PageCount = 100;
    static const size_t ThunkSize = 16;
    static const size_t ThunksPerPage = AutoSystemInfo::PageSize / ThunkSize;

public:
    static const uintptr_t ThunkAlignmentMask = ~(ThunkSize-1);
    static const uint TotalThunkSize = AutoSystemInfo::PageSize * PageCount;
    static const size_t TotalThunkCount = TotalThunkSize / ThunkSize;

private:
    CriticalSection cs;
    TAlloc * codeAllocator;
    uintptr_t baseAddress;
    HANDLE processHandle;
    BVStatic<TotalThunkCount> freeThunks;
    ThreadContextInfo * threadContext;
    BVIndex firstBitToCheck;

public:
    JITThunkEmitter(ThreadContextInfo * threadContext, TAlloc * codeAllocator, HANDLE processHandle);
    ~JITThunkEmitter();

    uintptr_t  CreateThunk(uintptr_t entryPoint);
    void FreeThunk(uintptr_t thunkAddress);
    uintptr_t EnsureInitialized();

    bool IsInThunk(uintptr_t address) const;
    static bool IsInThunk(uintptr_t thunkBaseAddress, uintptr_t address);
private:
    uintptr_t GetThunkAddressFromIndex(BVIndex index) const;
    BVIndex GetThunkIndexFromAddress(uintptr_t index) const;
    void ProtectPage(void * address);
    void UnprotectPage(void * address);
    bool IsThunkPageEmpty(uintptr_t address) const;

    static void EncodeJmp(char * localPageAddress, uintptr_t thunkAddress, uintptr_t targetAddress);
    static uintptr_t GetThunkPageStart(uintptr_t address);
};

#if ENABLE_OOP_NATIVE_CODEGEN
typedef JITThunkEmitter<SectionAllocWrapper> OOPJITThunkEmitter;
#endif
typedef JITThunkEmitter<VirtualAllocWrapper> InProcJITThunkEmitter;
#endif
