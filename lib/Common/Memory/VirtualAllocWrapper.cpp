//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

/*
* class VirtualAllocWrapper
*/

VirtualAllocWrapper VirtualAllocWrapper::Instance;  // single instance

LPVOID VirtualAllocWrapper::Alloc(LPVOID lpAddress, size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation)
{
    LPVOID address = nullptr;

#if defined(ENABLE_JIT_CLAMP)
    bool makeExecutable;

    if ((isCustomHeapAllocation) ||
        (protectFlags & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
    {
        makeExecutable = true;
    }
    else
    {
        makeExecutable = false;
    }

    AutoEnableDynamicCodeGen enableCodeGen(makeExecutable);
#endif

#if defined(_CONTROL_FLOW_GUARD)
    DWORD oldProtectFlags = 0;
    if (AutoSystemInfo::Data.IsCFGEnabled() && isCustomHeapAllocation)
    {
        //We do the allocation in two steps - CFG Bitmap in kernel will be created only on allocation with EXECUTE flag.
        //We again call VirtualProtect to set to the requested protectFlags.
        DWORD allocProtectFlags = 0;
        if (AutoSystemInfo::Data.IsCFGEnabled())
        {
            allocProtectFlags = PAGE_EXECUTE_RW_TARGETS_INVALID;
        }
        else
        {
            allocProtectFlags = PAGE_EXECUTE_READWRITE;
        }

        address = VirtualAlloc(lpAddress, dwSize, allocationType, allocProtectFlags);
        if (address == nullptr)
        {
            MemoryOperationLastError::RecordLastError();
            return nullptr;
        }
        else if ((allocationType & MEM_COMMIT) == MEM_COMMIT) // The access protection value can be set only on committed pages.
        {
            BOOL result = VirtualProtect(address, dwSize, protectFlags, &oldProtectFlags);
            if (result == FALSE)
            {
                CustomHeap_BadPageState_fatal_error((ULONG_PTR)this);
            }
        }
    }
    else
#endif
    {
        address = VirtualAlloc(lpAddress, dwSize, allocationType, protectFlags);
        if (address == nullptr)
        {
            MemoryOperationLastError::RecordLastError();
            return nullptr;
        }
    }

    return address;
}

BOOL VirtualAllocWrapper::Free(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType)
{
    AnalysisAssert(dwFreeType == MEM_RELEASE || dwFreeType == MEM_DECOMMIT);
    size_t bytes = (dwFreeType == MEM_RELEASE)? 0 : dwSize;
#pragma warning(suppress: 28160) // Calling VirtualFreeEx without the MEM_RELEASE flag frees memory but not address descriptors (VADs)
    BOOL ret = VirtualFree(lpAddress, bytes, dwFreeType);
    return ret;
}

/*
* class PreReservedVirtualAllocWrapper
*/
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
uint PreReservedVirtualAllocWrapper::numPreReservedSegment = 0;
#endif

PreReservedVirtualAllocWrapper::PreReservedVirtualAllocWrapper() :
    preReservedStartAddress(nullptr),
    cs(4000)
{
    freeSegments.SetAll();
}

PreReservedVirtualAllocWrapper::~PreReservedVirtualAllocWrapper()
{
    if (IsPreReservedRegionPresent())
    {
        BOOL success = VirtualFree(preReservedStartAddress, 0, MEM_RELEASE);
        PreReservedHeapTrace(_u("MEM_RELEASE the PreReservedSegment. Start Address: 0x%p, Size: 0x%x * 0x%x bytes"), preReservedStartAddress, PreReservedAllocationSegmentCount,
            AutoSystemInfo::Data.GetAllocationGranularityPageSize());
        if (!success)
        {
            // OOP JIT TODO: check if we need to cleanup the context related to this content process
        }

#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
        Assert(numPreReservedSegment > 0);
        InterlockedDecrement(&PreReservedVirtualAllocWrapper::numPreReservedSegment);
#endif
    }
}

bool
PreReservedVirtualAllocWrapper::IsPreReservedRegionPresent()
{
    return preReservedStartAddress != nullptr;
}

bool
PreReservedVirtualAllocWrapper::IsInRange(void * address)
{
    if (!this->IsPreReservedRegionPresent())
    {
        return false;
    }
    bool isInRange = IsInRange(GetPreReservedStartAddress(), address);
#if DBG
    if (isInRange)
    {
        //Check if the region is in MEM_COMMIT state.
        MEMORY_BASIC_INFORMATION memBasicInfo;
        size_t bytes = VirtualQuery(address, &memBasicInfo, sizeof(memBasicInfo));
        if (bytes == 0)
        {
            return false;
        }
        AssertMsg(memBasicInfo.State == MEM_COMMIT, "Memory not committed? Checking for uncommitted address region?");
    }
#endif
    return isInRange;
}

/* static */
bool
PreReservedVirtualAllocWrapper::IsInRange(void * regionStart, void * address)
{
    if (!regionStart)
    {
        return false;
    }
    if (address >= regionStart && address < GetPreReservedEndAddress(regionStart))
    {
        return true;
    }

    return false;

}

LPVOID
PreReservedVirtualAllocWrapper::GetPreReservedStartAddress()
{
    return preReservedStartAddress;
}

LPVOID
PreReservedVirtualAllocWrapper::GetPreReservedEndAddress()
{
    Assert(IsPreReservedRegionPresent());
    return GetPreReservedEndAddress(preReservedStartAddress);
}

/* static */
LPVOID
PreReservedVirtualAllocWrapper::GetPreReservedEndAddress(void * regionStart)
{
    return (char*)regionStart + (PreReservedAllocationSegmentCount * AutoSystemInfo::Data.GetAllocationGranularityPageCount() * AutoSystemInfo::PageSize);
}


LPVOID PreReservedVirtualAllocWrapper::EnsurePreReservedRegion()
{
    LPVOID startAddress = preReservedStartAddress;
    if (startAddress != nullptr)
    {
        return startAddress;
    }

    {
        AutoCriticalSection autocs(&this->cs);
        return EnsurePreReservedRegionInternal();
    }
}

LPVOID PreReservedVirtualAllocWrapper::EnsurePreReservedRegionInternal()
{
    LPVOID startAddress = preReservedStartAddress;
    if (startAddress != nullptr)
    {
        return startAddress;
    }

    //PreReserve a (bigger) segment
    size_t bytes = PreReservedAllocationSegmentCount * AutoSystemInfo::Data.GetAllocationGranularityPageSize();
    if (PHASE_FORCE1(Js::PreReservedHeapAllocPhase))
    {
        //This code is used where CFG is not available, but still PreReserve optimization for CFG can be tested
        startAddress = VirtualAlloc(NULL, bytes, MEM_RESERVE, PAGE_READWRITE);
        PreReservedHeapTrace(_u("Reserving PreReservedSegment For the first time(CFG Non-Enabled). Address: 0x%p\n"), preReservedStartAddress);
        preReservedStartAddress = startAddress;
        return startAddress;
    }

#if defined(_CONTROL_FLOW_GUARD)
    bool supportPreReservedRegion = true;
#if !_M_X64_OR_ARM64
#if _M_IX86
    // We want to restrict the number of prereserved segment for 32-bit process so that we don't use up the address space

    // Note: numPreReservedSegment is for the whole process, and access and update to it is not protected by a global lock.
    // So we may allocate more than the maximum some of the time if multiple thread check it simutaniously and allocate pass the limit.
    // It doesn't affect functionally, and it should be OK if we exceed.

    if (PreReservedVirtualAllocWrapper::numPreReservedSegment > PreReservedVirtualAllocWrapper::MaxPreReserveSegment)
    {
        supportPreReservedRegion = false;
    }
#else
    // TODO: fast check for prereserved segment is not implementated in ARM yet, so it is only enabled for x86
    supportPreReservedRegion = false;
#endif // _M_IX86
#endif

    if (AutoSystemInfo::Data.IsCFGEnabled() && supportPreReservedRegion)
    {
        startAddress = VirtualAlloc(NULL, bytes, MEM_RESERVE, PAGE_READWRITE);
        PreReservedHeapTrace(_u("Reserving PreReservedSegment For the first time(CFG Enabled). Address: 0x%p\n"), preReservedStartAddress);
        preReservedStartAddress = startAddress;

#if !_M_X64_OR_ARM64
        if (startAddress)
        {
            InterlockedIncrement(&PreReservedVirtualAllocWrapper::numPreReservedSegment);
        }
#endif
    }
#endif


    return startAddress;
}

/*
*   LPVOID PreReservedVirtualAllocWrapper::Alloc
*   -   Reserves only one big memory region.
*   -   Returns an Allocated memory region within this preReserved region with the specified protectFlags.
*   -   Tracks the committed pages
*/

LPVOID PreReservedVirtualAllocWrapper::Alloc(LPVOID lpAddress, size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation)
{
    AssertMsg(isCustomHeapAllocation, "PreReservation used for allocations other than CustomHeap?");
    AssertMsg(AutoSystemInfo::Data.IsCFGEnabled() || PHASE_FORCE1(Js::PreReservedHeapAllocPhase), "PreReservation without CFG ?");
    Assert(dwSize != 0);

    {
        AutoCriticalSection autocs(&this->cs);
        //Return nullptr, if no space to Reserve
        if (EnsurePreReservedRegionInternal() == nullptr)
        {
            PreReservedHeapTrace(_u("No space to pre-reserve memory with %d pages. Returning NULL\n"), PreReservedAllocationSegmentCount * AutoSystemInfo::Data.GetAllocationGranularityPageCount());
            return nullptr;
        }

        char * addressToReserve = nullptr;

        uint freeSegmentsBVIndex = BVInvalidIndex;
        size_t requestedNumOfSegments = dwSize / (AutoSystemInfo::Data.GetAllocationGranularityPageSize());
        Assert(requestedNumOfSegments <= MAXUINT32);

        if (lpAddress == nullptr)
        {
            Assert(requestedNumOfSegments != 0);
            AssertMsg(dwSize % AutoSystemInfo::Data.GetAllocationGranularityPageSize() == 0, "dwSize should be aligned with Allocation Granularity");

            do
            {
                freeSegmentsBVIndex = freeSegments.GetNextBit(freeSegmentsBVIndex + 1);
                //Return nullptr, if we don't have free/decommit pages to allocate
                if ((freeSegments.Length() - freeSegmentsBVIndex < requestedNumOfSegments) ||
                    freeSegmentsBVIndex == BVInvalidIndex)
                {
                    PreReservedHeapTrace(_u("No more space to commit in PreReserved Memory region.\n"));
                    return nullptr;
                }
            } while (!freeSegments.TestRange(freeSegmentsBVIndex, static_cast<uint>(requestedNumOfSegments)));

            uint offset = freeSegmentsBVIndex * AutoSystemInfo::Data.GetAllocationGranularityPageSize();
            addressToReserve = (char*) preReservedStartAddress + offset;

            //Check if the region is not already in MEM_COMMIT state.
            MEMORY_BASIC_INFORMATION memBasicInfo;
            size_t bytes = VirtualQuery(addressToReserve, &memBasicInfo, sizeof(memBasicInfo));
            if (bytes == 0) 
            {
                MemoryOperationLastError::RecordLastError();
            }
            if (bytes == 0
                || memBasicInfo.RegionSize < requestedNumOfSegments * AutoSystemInfo::Data.GetAllocationGranularityPageSize()
                || memBasicInfo.State == MEM_COMMIT)
            {
                CustomHeap_BadPageState_fatal_error((ULONG_PTR)this);
            }
        }
        else
        {
            //Check If the lpAddress is within the range of the preReserved Memory Region
            Assert(((char*) lpAddress) >= (char*) preReservedStartAddress || ((char*) lpAddress + dwSize) < GetPreReservedEndAddress());

            addressToReserve = (char*) lpAddress;
            freeSegmentsBVIndex = (uint) ((addressToReserve - (char*) preReservedStartAddress) / AutoSystemInfo::Data.GetAllocationGranularityPageSize());
#if DBG
            uint numOfSegments = (uint)ceil((double)dwSize / (double)AutoSystemInfo::Data.GetAllocationGranularityPageSize());
            Assert(numOfSegments != 0);
            Assert(freeSegmentsBVIndex + numOfSegments - 1 < freeSegments.Length());
            Assert(!freeSegments.TestRange(freeSegmentsBVIndex, numOfSegments));
#endif
        }

        AssertMsg(freeSegmentsBVIndex < PreReservedAllocationSegmentCount, "Invalid BitVector index calculation?");
        AssertMsg(dwSize % AutoSystemInfo::PageSize == 0, "COMMIT is managed at AutoSystemInfo::PageSize granularity");

        char * allocatedAddress = nullptr;
        bool failedToProtectPages = false;

        if ((allocationType & MEM_COMMIT) != 0)
        {
#if defined(ENABLE_JIT_CLAMP)
            AutoEnableDynamicCodeGen enableCodeGen;
#endif

#if defined(_CONTROL_FLOW_GUARD)
            if (AutoSystemInfo::Data.IsCFGEnabled())
            {
                DWORD oldProtect = 0;
                DWORD allocProtectFlags = 0;

                if (AutoSystemInfo::Data.IsCFGEnabled())
                {
                    allocProtectFlags = PAGE_EXECUTE_RW_TARGETS_INVALID;
                }
                else
                {
                    allocProtectFlags = PAGE_EXECUTE_READWRITE;
                }

                allocatedAddress = (char *)VirtualAlloc(addressToReserve, dwSize, MEM_COMMIT, allocProtectFlags);
                if (allocatedAddress != nullptr)
                {
                    BOOL result = VirtualProtect(allocatedAddress, dwSize, protectFlags, &oldProtect);
                    if (result == FALSE)
                    {
                        CustomHeap_BadPageState_fatal_error((ULONG_PTR)this);
                    }
                    AssertMsg(oldProtect == (PAGE_EXECUTE_READWRITE), "CFG Bitmap gets allocated and bits will be set to invalid only upon passing these flags.");
                }
                else
                {
                    MemoryOperationLastError::RecordLastError();
                }
            }
            else
#endif
            {
                allocatedAddress = (char *)VirtualAlloc(addressToReserve, dwSize, MEM_COMMIT, protectFlags);
                if (allocatedAddress == nullptr)
                {
                    MemoryOperationLastError::RecordLastError();
                }
            }
        }
        else
        {
            // Just return the uncommitted address if we didn't ask to commit it.
            allocatedAddress = addressToReserve;
        }

        // Keep track of the committed pages within the preReserved Memory Region
        if (lpAddress == nullptr && allocatedAddress != nullptr)
        {
            Assert(allocatedAddress == addressToReserve);
            Assert(requestedNumOfSegments != 0);
            freeSegments.ClearRange(freeSegmentsBVIndex, static_cast<uint>(requestedNumOfSegments));
        }

        PreReservedHeapTrace(_u("MEM_COMMIT: StartAddress: 0x%p of size: 0x%x * 0x%x bytes \n"), allocatedAddress, requestedNumOfSegments, AutoSystemInfo::Data.GetAllocationGranularityPageSize());
        if (failedToProtectPages)
        {
            return nullptr;
        }
        return allocatedAddress;
    }
}

/*
*   PreReservedVirtualAllocWrapper::Free
*   -   Doesn't actually release the pages to the CPU.
*   -   It Decommits the page (memory region within the preReserved Region)
*   -   Update the tracking of the committed pages.
*/

BOOL
PreReservedVirtualAllocWrapper::Free(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType)
{
    {
        AutoCriticalSection autocs(&this->cs);

        if (dwSize == 0)
        {
            Assert(false);
            return FALSE;
        }

        if (preReservedStartAddress == nullptr)
        {
            Assert(false);
            return FALSE;
        }

        Assert(dwSize % AutoSystemInfo::PageSize == 0);
#pragma warning(suppress: 6250)
#pragma warning(suppress: 28160) // Calling VirtualFreeEx without the MEM_RELEASE flag frees memory but not address descriptors (VADs)
        BOOL success = VirtualFree(lpAddress, dwSize, MEM_DECOMMIT);
        size_t requestedNumOfSegments = dwSize / AutoSystemInfo::Data.GetAllocationGranularityPageSize();
        Assert(requestedNumOfSegments <= MAXUINT32);

        if (success)
        {
            PreReservedHeapTrace(_u("MEM_DECOMMIT: Address: 0x%p of size: 0x%x bytes\n"), lpAddress, dwSize);
        }

        if (success && (dwFreeType & MEM_RELEASE) != 0)
        {
            Assert((uintptr_t) lpAddress >= (uintptr_t) preReservedStartAddress);
            AssertMsg(((uintptr_t)lpAddress & (AutoSystemInfo::Data.GetAllocationGranularityPageCount() - 1)) == 0, "Not aligned with Allocation Granularity?");
            AssertMsg(dwSize % AutoSystemInfo::Data.GetAllocationGranularityPageSize() == 0, "Release size should match the allocation granularity size");
            Assert(requestedNumOfSegments != 0);

            BVIndex freeSegmentsBVIndex = (BVIndex) (((uintptr_t) lpAddress - (uintptr_t) preReservedStartAddress) / AutoSystemInfo::Data.GetAllocationGranularityPageSize());
            AssertMsg(freeSegmentsBVIndex < PreReservedAllocationSegmentCount, "Invalid Index ?");
            freeSegments.SetRange(freeSegmentsBVIndex, static_cast<uint>(requestedNumOfSegments));
            PreReservedHeapTrace(_u("MEM_RELEASE: Address: 0x%p of size: 0x%x * 0x%x bytes\n"), lpAddress, requestedNumOfSegments, AutoSystemInfo::Data.GetAllocationGranularityPageSize());
        }

        return success;
    }
}

#if defined(ENABLE_JIT_CLAMP)
/*
* class AutoEnableDynamicCodeGen
*/

typedef
BOOL
(WINAPI *PGET_PROCESS_MITIGATION_POLICY_PROC)(
    _In_  HANDLE                    hProcess,
    _In_  PROCESS_MITIGATION_POLICY MitigationPolicy,
    _Out_ PVOID                     lpBuffer,
    _In_  SIZE_T                    dwLength
);

AutoEnableDynamicCodeGen::PSET_THREAD_INFORMATION_PROC AutoEnableDynamicCodeGen::SetThreadInformationProc = nullptr;
AutoEnableDynamicCodeGen::PGET_THREAD_INFORMATION_PROC AutoEnableDynamicCodeGen::GetThreadInformationProc = nullptr;
PROCESS_MITIGATION_DYNAMIC_CODE_POLICY AutoEnableDynamicCodeGen::processPolicy;
CriticalSection AutoEnableDynamicCodeGen::processPolicyCS;
volatile bool AutoEnableDynamicCodeGen::processPolicyObtained = false;

AutoEnableDynamicCodeGen::AutoEnableDynamicCodeGen(bool enable) : enabled(false)
{
    if (enable == false)
    {
        return;
    }

    //
    // Snap the dynamic code generation policy for this process so that we
    // don't need to resolve APIs and query it each time. We expect the policy
    // to have been established upfront.
    //

    if (processPolicyObtained == false)
    {
        AutoCriticalSection autocs(&processPolicyCS);

        if (processPolicyObtained == false)
        {
            PGET_PROCESS_MITIGATION_POLICY_PROC GetProcessMitigationPolicyProc = nullptr;

            HMODULE module = GetModuleHandleW(_u("api-ms-win-core-processthreads-l1-1-3.dll"));

            if (module != nullptr)
            {
                GetProcessMitigationPolicyProc = (PGET_PROCESS_MITIGATION_POLICY_PROC) GetProcAddress(module, "GetProcessMitigationPolicy");
                SetThreadInformationProc = (PSET_THREAD_INFORMATION_PROC) GetProcAddress(module, "SetThreadInformation");
                GetThreadInformationProc = (PGET_THREAD_INFORMATION_PROC) GetProcAddress(module, "GetThreadInformation");
            }

            if ((GetProcessMitigationPolicyProc == nullptr) ||
                (!GetProcessMitigationPolicyProc(GetCurrentProcess(), ProcessDynamicCodePolicy, (PPROCESS_MITIGATION_DYNAMIC_CODE_POLICY) &processPolicy, sizeof(processPolicy))))
            {
                processPolicy.ProhibitDynamicCode = 0;
            }

            processPolicyObtained = true;
        }
    }

    //
    // The process is not prohibiting dynamic code or does not allow threads
    // to opt out.  In either case, return to the caller.
    //
    // N.B. It is OK that this policy is mutable at runtime. If a process
    //      really does not allow thread opt-out, then the call below will fail
    //      benignly.
    //

    if ((processPolicy.ProhibitDynamicCode == 0) || (processPolicy.AllowThreadOptOut == 0))
    {
        return;
    }

    if (SetThreadInformationProc == nullptr || GetThreadInformationProc == nullptr)
    {
        return;
    }

    //
    // If dynamic code is already allowed for this thread, then don't attempt to allow it again.
    //

    DWORD threadPolicy;

    if ((GetThreadInformationProc(GetCurrentThread(), ThreadDynamicCodePolicy, &threadPolicy, sizeof(DWORD))) &&
        (threadPolicy == THREAD_DYNAMIC_CODE_ALLOW))
    {
        return;
    }

    threadPolicy = THREAD_DYNAMIC_CODE_ALLOW;

    BOOL result = SetThreadInformationProc(GetCurrentThread(), ThreadDynamicCodePolicy, &threadPolicy, sizeof(DWORD));
    Assert(result);

    enabled = true;
}

AutoEnableDynamicCodeGen::~AutoEnableDynamicCodeGen()
{
    if (enabled)
    {
        DWORD threadPolicy = 0;

        BOOL result = SetThreadInformationProc(GetCurrentThread(), ThreadDynamicCodePolicy, &threadPolicy, sizeof(DWORD));
        Assert(result);

        enabled = false;
    }
}

#endif // defined(ENABLE_JIT_CLAMP)

