//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

/*
* class VirtualAllocWrapper
*/

LPVOID VirtualAllocWrapper::Alloc(LPVOID lpAddress, size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation)
{
    Assert(this == nullptr);
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
    DWORD oldProtectFlags;
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
        VirtualProtect(address, dwSize, protectFlags, &oldProtectFlags);
    }
    else
#endif
    {
        address = VirtualAlloc(lpAddress, dwSize, allocationType, protectFlags);
    }

    return address;
}

BOOL VirtualAllocWrapper::Free(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType)
{
    Assert(this == nullptr);
    AnalysisAssert(dwFreeType == MEM_RELEASE || dwFreeType == MEM_DECOMMIT);
    size_t bytes = (dwFreeType == MEM_RELEASE)? 0 : dwSize;
    return VirtualFree(lpAddress, bytes, dwFreeType);
}

bool
VirtualAllocWrapper::IsPreReservedRegionPresent()
{
    return false;
}

LPVOID
VirtualAllocWrapper::GetPreReservedStartAddress()
{
    Assert(false);
    return nullptr;
}

LPVOID
VirtualAllocWrapper::GetPreReservedEndAddress()
{
    Assert(false);
    return nullptr;
}

bool
VirtualAllocWrapper::IsInRange(void * address)
{
    Assert(this == nullptr);
    return false;
}


/*
* class PreReservedVirtualAllocWrapper
*/

PreReservedVirtualAllocWrapper::PreReservedVirtualAllocWrapper() :
preReservedStartAddress(nullptr),
cs(4000)
{
    freeSegments.SetAll();
}

BOOL
PreReservedVirtualAllocWrapper::Shutdown()
{
    Assert(this);
    BOOL success = FALSE;
    if (IsPreReservedRegionPresent())
    {
        success = VirtualFree(preReservedStartAddress, 0, MEM_RELEASE);
        PreReservedHeapTrace(L"MEM_RELEASE the PreReservedSegment. Start Address: 0x%p, Size: 0x%x * 0x%x bytes", preReservedStartAddress, PreReservedAllocationSegmentCount,
            AutoSystemInfo::Data.GetAllocationGranularityPageSize());
        if (!success)
        {
            Assert(false);
        }
    }
    return success;
}

bool
PreReservedVirtualAllocWrapper::IsPreReservedRegionPresent()
{
    Assert(this);
    return preReservedStartAddress != nullptr;
}

bool
PreReservedVirtualAllocWrapper::IsInRange(void * address)
{
    if (this == nullptr)
    {
        return false;
    }
#if DBG
    //Check if the region is in MEM_COMMIT state.
    MEMORY_BASIC_INFORMATION memBasicInfo;
    size_t bytes = VirtualQuery(address, &memBasicInfo, sizeof(memBasicInfo));
    if (bytes == 0 || memBasicInfo.State != MEM_COMMIT)
    {
        AssertMsg(false, "Memory not committed? Checking for uncommitted address region?");
    }
#endif

    return IsPreReservedRegionPresent() && address >= GetPreReservedStartAddress() && address < GetPreReservedEndAddress();
}

LPVOID
PreReservedVirtualAllocWrapper::GetPreReservedStartAddress()
{
    Assert(this);
    return preReservedStartAddress;
}

LPVOID
PreReservedVirtualAllocWrapper::GetPreReservedEndAddress()
{
    Assert(this);
    return (char*)preReservedStartAddress + (PreReservedAllocationSegmentCount * AutoSystemInfo::Data.GetAllocationGranularityPageCount() * AutoSystemInfo::PageSize);
}

/*
*   LPVOID PreReservedVirtualAllocWrapper::Alloc
*   -   Reserves only one big memory region.
*   -   Returns an Allocated memory region within this preReserved region with the specified protectFlags.
*   -   Tracks the committed pages
*/

LPVOID PreReservedVirtualAllocWrapper::Alloc(LPVOID lpAddress, size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation)
{
    Assert(this);
    AssertMsg(isCustomHeapAllocation, "PreReservation used for allocations other than CustomHeap?");
    AssertMsg(AutoSystemInfo::Data.IsCFGEnabled() || PHASE_FORCE1(Js::PreReservedHeapAllocPhase), "PreReservation without CFG ?");
    Assert((allocationType & MEM_COMMIT) != 0);
    Assert(dwSize != 0);

    {
        AutoCriticalSection autocs(&this->cs);
        if (preReservedStartAddress == NULL)
        {
            //PreReserve a (bigger) segment
            size_t bytes = PreReservedAllocationSegmentCount * AutoSystemInfo::Data.GetAllocationGranularityPageSize();
#if defined(_CONTROL_FLOW_GUARD)
            if (AutoSystemInfo::Data.IsCFGEnabled())
            {
                preReservedStartAddress = VirtualAlloc(NULL, bytes, MEM_RESERVE, PAGE_READWRITE);
                PreReservedHeapTrace(L"Reserving PreReservedSegment For the first time(CFG Enabled). Address: 0x%p\n", preReservedStartAddress);
            }
            else
#endif
            if (PHASE_FORCE1(Js::PreReservedHeapAllocPhase))
            {
                //This code is used where CFG is not available, but still PreReserve optimization for CFG can be tested
                preReservedStartAddress = VirtualAlloc(NULL, bytes, MEM_RESERVE, protectFlags);
                PreReservedHeapTrace(L"Reserving PreReservedSegment For the first time(CFG Non-Enabled). Address: 0x%p\n", preReservedStartAddress);
            }
        }

        //Return nullptr, if no space to Reserve
        if (preReservedStartAddress == NULL)
        {
            PreReservedHeapTrace(L"No space to pre-reserve memory with %d pages. Returning NULL\n", PreReservedAllocationSegmentCount * AutoSystemInfo::Data.GetAllocationGranularityPageCount());
            return nullptr;
        }

        char * addressToCommit = nullptr;

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
                    PreReservedHeapTrace(L"No more space to commit in PreReserved Memory region.\n");
                    return nullptr;
                }
            } while (!freeSegments.TestRange(freeSegmentsBVIndex, static_cast<uint>(requestedNumOfSegments)));

            uint offset = freeSegmentsBVIndex * AutoSystemInfo::Data.GetAllocationGranularityPageSize();
            addressToCommit = (char*) preReservedStartAddress + offset;

            //Check if the region is not already in MEM_COMMIT state.
            MEMORY_BASIC_INFORMATION memBasicInfo;
            size_t bytes = VirtualQuery(addressToCommit, &memBasicInfo, sizeof(memBasicInfo));
            if (bytes == 0
                || memBasicInfo.RegionSize < requestedNumOfSegments * AutoSystemInfo::Data.GetAllocationGranularityPageSize()
                || memBasicInfo.State == MEM_COMMIT
                )
            {
                CustomHeap_BadPageState_fatal_error((ULONG_PTR)this);
                return nullptr;
            }
        }
        else
        {
            //Check If the lpAddress is within the range of the preReserved Memory Region
            Assert(((char*) lpAddress) >= (char*) preReservedStartAddress || ((char*) lpAddress + dwSize) < GetPreReservedEndAddress());

            addressToCommit = (char*) lpAddress;
            freeSegmentsBVIndex = (uint) ((addressToCommit - (char*) preReservedStartAddress) / AutoSystemInfo::Data.GetAllocationGranularityPageSize());
#if DBG
            uint numOfSegments = (uint)ceil((double)dwSize / (double)AutoSystemInfo::Data.GetAllocationGranularityPageSize());
            Assert(numOfSegments != 0);
            Assert(freeSegmentsBVIndex + numOfSegments - 1 < freeSegments.Length());
            Assert(!freeSegments.TestRange(freeSegmentsBVIndex, numOfSegments));
#endif
        }

        AssertMsg(freeSegmentsBVIndex < PreReservedAllocationSegmentCount, "Invalid BitVector index calculation?");
        AssertMsg(dwSize % AutoSystemInfo::PageSize == 0, "COMMIT is managed at AutoSystemInfo::PageSize granularity");

        char * committedAddress = nullptr;

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

            committedAddress = (char *)VirtualAlloc(addressToCommit, dwSize, MEM_COMMIT, allocProtectFlags);

            AssertMsg(committedAddress != nullptr, "If no space to allocate, then how did we fetch this address from the tracking bit vector?");
            VirtualProtect(committedAddress, dwSize, protectFlags, &oldProtect);
            AssertMsg(oldProtect == (PAGE_EXECUTE_READWRITE), "CFG Bitmap gets allocated and bits will be set to invalid only upon passing these flags.");
        }
        else
#endif
        {
            committedAddress = (char *) VirtualAlloc(addressToCommit, dwSize, MEM_COMMIT, protectFlags);
        }

        //Keep track of the committed pages within the preReserved Memory Region
        if (lpAddress == nullptr && committedAddress != nullptr)
        {
            Assert(committedAddress == addressToCommit);
            Assert(requestedNumOfSegments != 0);
            freeSegments.ClearRange(freeSegmentsBVIndex, static_cast<uint>(requestedNumOfSegments));
        }

        PreReservedHeapTrace(L"MEM_COMMIT: StartAddress: 0x%p of size: 0x%x * 0x%x bytes \n", committedAddress, requestedNumOfSegments, AutoSystemInfo::Data.GetAllocationGranularityPageSize());
        return committedAddress;
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
    Assert(this);
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
        BOOL success = VirtualFree(lpAddress, dwSize, MEM_DECOMMIT);
        size_t requestedNumOfSegments = dwSize / AutoSystemInfo::Data.GetAllocationGranularityPageSize();
        Assert(requestedNumOfSegments <= MAXUINT32);

        if (success)
        {
            PreReservedHeapTrace(L"MEM_DECOMMIT: Address: 0x%p of size: 0x%x bytes\n", lpAddress, dwSize);
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
            PreReservedHeapTrace(L"MEM_RELEASE: Address: 0x%p of size: 0x%x * 0x%x bytes\n", lpAddress, requestedNumOfSegments, AutoSystemInfo::Data.GetAllocationGranularityPageSize());
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

            HMODULE module = GetModuleHandleW(L"api-ms-win-core-processthreads-l1-1-3.dll");

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

