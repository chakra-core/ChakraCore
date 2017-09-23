//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{

#define PreReservedHeapTrace(...) \
{ \
    OUTPUT_TRACE(Js::PreReservedHeapAllocPhase, __VA_ARGS__); \
}


class SectionAllocWrapper;
class PreReservedSectionAllocWrapper;

/*
* VirtualAllocWrapper is just a delegator class to call VirtualAlloc and VirtualFree.
*/
class VirtualAllocWrapper
{
public:
    LPVOID  Alloc(LPVOID lpAddress, DECLSPEC_GUARD_OVERFLOW size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation);
    BOOL    Free(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType);
    LPVOID  AllocLocal(LPVOID lpAddress, DECLSPEC_GUARD_OVERFLOW size_t dwSize) { return lpAddress; }
    BOOL    FreeLocal(LPVOID lpAddress) { return true; }

    static VirtualAllocWrapper Instance;  // single instance
private:
    VirtualAllocWrapper() {}
};

/*
* PreReservedVirtualAllocWrapper class takes care of Reserving a large memory region initially
* and then committing mem regions for the size requested.
* Committed pages are being tracked with a bitVector.
* Committing memory outside of the preReserved Memory region is not handled by this allocator
*/

class PreReservedVirtualAllocWrapper
{
public:
#if _M_IX86_OR_ARM32
    static const uint PreReservedAllocationSegmentCount = 256; // (256 * 64K) == 16 MB, if 64k is the AllocationGranularity
#else // _M_X64_OR_ARM64
    static const uint PreReservedAllocationSegmentCount = 4096; //(4096 * 64K) == 256MB, if 64k is the AllocationGranularity
#endif

#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    static const unsigned MaxPreReserveSegment = 6;
#endif
public:
    PreReservedVirtualAllocWrapper();
    ~PreReservedVirtualAllocWrapper();
    LPVOID      Alloc(LPVOID lpAddress, DECLSPEC_GUARD_OVERFLOW size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation);
    BOOL        Free(LPVOID lpAddress,  size_t dwSize, DWORD dwFreeType);
    LPVOID  AllocLocal(LPVOID lpAddress, DECLSPEC_GUARD_OVERFLOW size_t dwSize) { return lpAddress; }
    BOOL    FreeLocal(LPVOID lpAddress) { return true; }

    bool        IsInRange(void * address);
    static bool IsInRange(void * regionStart, void * address);
    LPVOID      EnsurePreReservedRegion();

    LPVOID      GetPreReservedEndAddress();
    static LPVOID GetPreReservedEndAddress(void * regionStart);
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    static int  NumPreReservedSegment() { return numPreReservedSegment; }
#endif

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
    bool        IsPreReservedEndAddress(LPVOID address)
    {
        return IsPreReservedRegionPresent() && address == GetPreReservedEndAddress();
    }
#endif
private:
    LPVOID      EnsurePreReservedRegionInternal();
    bool        IsPreReservedRegionPresent();
    LPVOID      GetPreReservedStartAddress();

    BVStatic<PreReservedAllocationSegmentCount>     freeSegments;
    LPVOID                                          preReservedStartAddress;
    CriticalSection                                 cs;
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
    static uint  numPreReservedSegment;
#endif
};

#if defined(ENABLE_JIT_CLAMP)

class AutoEnableDynamicCodeGen
{
public:
    AutoEnableDynamicCodeGen(bool enable = true);
    ~AutoEnableDynamicCodeGen();

private:
    bool enabled;

    typedef
    BOOL
    (WINAPI *PSET_THREAD_INFORMATION_PROC)(
        _In_ HANDLE                   hThread,
        _In_ THREAD_INFORMATION_CLASS ThreadInformationClass,
        _In_reads_bytes_(ThreadInformationSize) PVOID ThreadInformation,
        _In_ DWORD                    ThreadInformationSize
    );

    typedef
    BOOL
    (WINAPI *PGET_THREAD_INFORMATION_PROC)(
        _In_ HANDLE                   hThread,
        _In_ THREAD_INFORMATION_CLASS ThreadInformationClass,
        _Out_writes_bytes_(ThreadInformationSize) PVOID ThreadInformation,
        _In_ DWORD                    ThreadInformationSize
    );

    static PSET_THREAD_INFORMATION_PROC SetThreadInformationProc;
    static PGET_THREAD_INFORMATION_PROC GetThreadInformationProc;
    static PROCESS_MITIGATION_DYNAMIC_CODE_POLICY processPolicy;
    static CriticalSection processPolicyCS;
    static volatile bool processPolicyObtained;
};
#endif

template<typename TVirtualAlloc = VirtualAllocWrapper>
class PageSegmentBase;

template<typename TVirtualAlloc = VirtualAllocWrapper>
class SegmentBase;

template<typename TVirtualAlloc = VirtualAllocWrapper, typename TSegment = SegmentBase<TVirtualAlloc>, typename TPageSegment = PageSegmentBase<TVirtualAlloc>>
class PageAllocatorBase;

typedef PageAllocatorBase<> PageAllocator;
typedef PageSegmentBase<> PageSegment;
typedef SegmentBase<> Segment;
}
