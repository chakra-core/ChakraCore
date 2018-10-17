//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#if defined(ENABLE_NATIVE_CODEGEN) && defined(_CONTROL_FLOW_GUARD) && !defined(_M_ARM)

template class JITThunkEmitter<VirtualAllocWrapper>;

#if ENABLE_OOP_NATIVE_CODEGEN
template class JITThunkEmitter<SectionAllocWrapper>;
#endif

#if _M_IX86 || _M_X64
template <typename TAlloc>
const BYTE JITThunkEmitter<TAlloc>::DirectJmp[] = {
    0xE9, 0x00, 0x00, 0x00, 0x00, // JMP <relativeAddress>.32
    0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC
};
#endif

#if _M_X64
template <typename TAlloc>
const BYTE JITThunkEmitter<TAlloc>::IndirectJmp[] = {
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MOV RAX, <relativeAddress>.64
    0x48, 0xFF, 0xE0,                                           // JMP RAX
    0xCC, 0xCC, 0xCC
};
#endif

#if _M_ARM64
template <typename TAlloc>
const DWORD JITThunkEmitter<TAlloc>::DirectB[] = {
    0x14000000              // B <relativeAddress>.26
};

template <typename TAlloc>
const DWORD JITThunkEmitter<TAlloc>::IndirectBR[] = {
    0xd2800000 | IndirectBRTempReg,     // MOVZ x17, <absoluteAddress[16:0]>
    0xf2a00000 | IndirectBRTempReg,     // MOVK x17, <absoluteAddress[31:16]>, LSL #16
    0xf2c00000 | IndirectBRTempReg,     // MOVK x17, <absoluteAddress[47:32]>, LSL #32
    0xd61f0000 | (IndirectBRTempReg<<5) // BR x17
};
#endif

template <typename TAlloc>
JITThunkEmitter<TAlloc>::JITThunkEmitter(ThreadContextInfo * threadContext, TAlloc * codeAllocator, HANDLE processHandle) :
    processHandle(processHandle),
    codeAllocator(codeAllocator),
    threadContext(threadContext),
    baseAddress(NULL),
    firstBitToCheck(0)
{
    freeThunks.SetAll();
}

template <typename TAlloc>
JITThunkEmitter<TAlloc>::~JITThunkEmitter()
{
    if (baseAddress != NULL)
    {
        this->codeAllocator->Free((PVOID)baseAddress, TotalThunkSize, MEM_RELEASE);
    }
}

template <typename TAlloc> inline
uintptr_t
JITThunkEmitter<TAlloc>::CreateThunk(uintptr_t entryPoint)
{
    AutoCriticalSection autoCs(&this->cs);
    if(EnsureInitialized() == NULL)
    {
        return NULL;
    }

    // find available thunk
    BVIndex thunkIndex = this->freeThunks.GetNextBit(this->firstBitToCheck);
    if (thunkIndex == BVInvalidIndex)
    {
        return NULL;
    }
    uintptr_t thunkAddress = GetThunkAddressFromIndex(thunkIndex);

    uintptr_t pageStartAddress = GetThunkPageStart(thunkAddress);
    char * localPageAddress = (char *)this->codeAllocator->AllocLocal((PVOID)pageStartAddress, AutoSystemInfo::PageSize);
    if (localPageAddress == nullptr)
    {
        return NULL;
    }

    if (IsThunkPageEmpty(pageStartAddress))
    {
        if (this->codeAllocator->AllocPages((PVOID)pageStartAddress, 1, MEM_COMMIT, PAGE_EXECUTE_READ, true) == nullptr)
        {
            this->codeAllocator->FreeLocal(localPageAddress);
            return NULL;
        }
        UnprotectPage(localPageAddress);
        memset(localPageAddress, 0xCC, AutoSystemInfo::PageSize);
    }
    else
    {
        UnprotectPage(localPageAddress);
    }

    EncodeJmp(localPageAddress, thunkAddress, entryPoint);

    ProtectPage(localPageAddress);
    this->codeAllocator->FreeLocal(localPageAddress);

    if (CONFIG_FLAG(OOPCFGRegistration))
    {
        this->threadContext->SetValidCallTargetForCFG((PVOID)thunkAddress);
    }
    this->firstBitToCheck = (thunkIndex + 1 < JITThunkEmitter<TAlloc>::TotalThunkCount) ? thunkIndex + 1 : 0;
    this->freeThunks.Clear(thunkIndex);

    if (!FlushInstructionCache(this->processHandle, (PVOID)thunkAddress, ThunkSize))
    {
        return NULL;
    }

    return thunkAddress;
}

template <typename TAlloc> inline
void
JITThunkEmitter<TAlloc>::FreeThunk(uintptr_t thunkAddress)
{
    AutoCriticalSection autoCs(&this->cs);
    BVIndex thunkIndex = GetThunkIndexFromAddress(thunkAddress);
    if (thunkIndex >= this->freeThunks.Length() || this->freeThunks.TestAndSet(thunkIndex))
    {
        Assert(UNREACHED);
        this->firstBitToCheck = 0;
        return;
    }

    if (thunkIndex < firstBitToCheck)
    {
        this->firstBitToCheck = thunkIndex;
    }

    if (CONFIG_FLAG(OOPCFGRegistration))
    {
        this->threadContext->SetValidCallTargetForCFG((PVOID)thunkAddress, false);
    }

    uintptr_t pageStartAddress = GetThunkPageStart(thunkAddress);
    if (IsThunkPageEmpty(pageStartAddress))
    {
        this->codeAllocator->Free((PVOID)pageStartAddress, AutoSystemInfo::PageSize, MEM_DECOMMIT);
    }
    else
    {
        char * localAddress = (char *)this->codeAllocator->AllocLocal((PVOID)thunkAddress, ThunkSize);
        if (localAddress == nullptr)
        {
            return;
        }
        UnprotectPage(localAddress);
        memset(localAddress, 0xCC, ThunkSize);
        ProtectPage(localAddress);
        this->codeAllocator->FreeLocal(localAddress);
    }
    FlushInstructionCache(this->processHandle, (PVOID)thunkAddress, ThunkSize);
}

template <typename TAlloc> inline
uintptr_t
JITThunkEmitter<TAlloc>::EnsureInitialized()
{
    if (this->baseAddress != NULL)
    {
        return this->baseAddress;
    }

    // only take a lock if we need to initialize
    {
        AutoCriticalSection autoCs(&this->cs);
        // check again because we did the first one outside of lock
        if (this->baseAddress == NULL)
        {
            this->baseAddress = (uintptr_t)this->codeAllocator->AllocPages(nullptr, PageCount, MEM_RESERVE, PAGE_EXECUTE_READ, true);
        }
    }
    return this->baseAddress;
}

template <typename TAlloc> inline
bool
JITThunkEmitter<TAlloc>::IsInThunk(uintptr_t address) const
{
    return IsInThunk(this->baseAddress, address);
}

/* static */
template <typename TAlloc> inline
bool
JITThunkEmitter<TAlloc>::IsInThunk(uintptr_t thunkBaseAddress, uintptr_t address)
{
    bool isInThunk = address >= thunkBaseAddress && address < thunkBaseAddress + TotalThunkSize;
    Assert(!isInThunk || address % ThunkSize == 0);
    return isInThunk;
}

/* static */
template <typename TAlloc> inline
void
JITThunkEmitter<TAlloc>::EncodeJmp(char * localPageAddress, uintptr_t thunkAddress, uintptr_t targetAddress)
{
    char * localAddress = localPageAddress + thunkAddress % AutoSystemInfo::PageSize;
#if _M_IX86 || _M_X64
    ptrdiff_t relativeAddress = targetAddress - thunkAddress - DirectJmpIPAdjustment;
#if _M_X64
    if (relativeAddress > INT_MAX || relativeAddress < INT_MIN)
    {
        memcpy_s(localAddress, ThunkSize, IndirectJmp, ThunkSize);
        uintptr_t * jmpTarget = (uintptr_t*)(localAddress + IndirectJmpTargetOffset);
        *jmpTarget = targetAddress;
    }
    else
#endif
    {
        memcpy_s(localAddress, ThunkSize, DirectJmp, ThunkSize);
        uintptr_t * jmpTarget = (uintptr_t*)(localAddress + DirectJmpTargetOffset);
        *jmpTarget = relativeAddress;
    }
#elif _M_ARM64
    ptrdiff_t relativeAddress = (targetAddress - thunkAddress) / 4;
    if (relativeAddress >= (1 << 25) || relativeAddress < -(1 << 25))
    {
        Assert(targetAddress == (targetAddress & 0xffffffffffffull));
        memcpy_s(localAddress, ThunkSize, IndirectBR, ThunkSize);
        ((DWORD *)localAddress)[IndirectBRLo16Offset]  |= ((targetAddress >> 0) & 0xffff) << 5;
        ((DWORD *)localAddress)[IndirectBRMid16Offset] |= ((targetAddress >> 16) & 0xffff) << 5;
        ((DWORD *)localAddress)[IndirectBRHi16Offset]  |= ((targetAddress >> 32) & 0xffff) << 5;
    }
    else
    {
        memcpy_s(localAddress, ThunkSize, DirectB, ThunkSize);
        ((DWORD *)localAddress)[0] |= relativeAddress & 0x3ffffff;
    }
#endif
}

template <typename TAlloc> inline
bool
JITThunkEmitter<TAlloc>::IsThunkPageEmpty(uintptr_t address) const
{
    Assert(address == GetThunkPageStart(address));
    BVIndex pageStartIndex = GetThunkIndexFromAddress(address);
    Assert(pageStartIndex != BVInvalidIndex);
    BVStatic<ThunksPerPage> * pageBV = this->freeThunks.GetRange<ThunksPerPage>(pageStartIndex);
    return pageBV->IsAllSet();
}

template <> inline
void
JITThunkEmitter<VirtualAllocWrapper>::ProtectPage(void * address)
{
#if defined(ENABLE_JIT_CLAMP)
    AutoEnableDynamicCodeGen enableCodeGen(true);
#endif
    DWORD oldProtect;
    BOOL result = VirtualProtectEx(this->processHandle, address, ThunkSize, PAGE_EXECUTE_READ, &oldProtect);
    AssertOrFailFast(result && oldProtect == PAGE_EXECUTE_READWRITE);
}

template <> inline
void
JITThunkEmitter<VirtualAllocWrapper>::UnprotectPage(void * address)
{
#if defined(ENABLE_JIT_CLAMP)
    AutoEnableDynamicCodeGen enableCodeGen(true);
#endif
    DWORD oldProtect;
    BOOL result = VirtualProtectEx(this->processHandle, address, ThunkSize, PAGE_EXECUTE_READWRITE, &oldProtect);
    AssertOrFailFast(result && oldProtect == PAGE_EXECUTE_READ);
}

#if ENABLE_OOP_NATIVE_CODEGEN
template <> inline
void
JITThunkEmitter<SectionAllocWrapper>::ProtectPage(void * address)
{
}

template <> inline
void
JITThunkEmitter<SectionAllocWrapper>::UnprotectPage(void * address)
{
}
#endif

template <typename TAlloc> inline
uintptr_t
JITThunkEmitter<TAlloc>::GetThunkAddressFromIndex(BVIndex index) const
{
    return this->baseAddress + index * ThunkSize;
}

template <typename TAlloc> inline
BVIndex
JITThunkEmitter<TAlloc>::GetThunkIndexFromAddress(uintptr_t thunkAddress) const
{
    uintptr_t thunkIndex = (thunkAddress - this->baseAddress) / ThunkSize;
#if TARGET_64
    if (thunkIndex > BVInvalidIndex)
    {
        thunkIndex = BVInvalidIndex;
    }
#endif
    return (BVIndex)thunkIndex;
}

/* static */
template <typename TAlloc> inline
uintptr_t
JITThunkEmitter<TAlloc>::GetThunkPageStart(uintptr_t address)
{
    return address - address % AutoSystemInfo::PageSize;
}
#endif
