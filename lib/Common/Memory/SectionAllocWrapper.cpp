//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

#if _WIN32
#include "../Core/DelayLoadLibrary.h"

#ifdef NTDDI_WIN10_RS2
#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)
#define USEFILEMAP2 1
#endif
#endif

namespace Memory
{

void CloseSectionHandle(HANDLE handle)
{
#if USEFILEMAP2
    CloseHandle(handle);
#else
    NtdllLibrary::Instance->Close(handle);
#endif
}

HANDLE CreateSection(size_t sectionSize, bool commit)
{
    const ULONG allocAttributes = commit ? SEC_COMMIT : SEC_RESERVE;
#if USEFILEMAP2
#if TARGET_32
    DWORD sizeHigh = 0;
#elif TARGET_64
    DWORD sizeHigh = (DWORD)(sectionSize >> 32);
#endif
    HANDLE handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE | allocAttributes, sizeHigh, (DWORD)sectionSize, NULL);
    if (handle == nullptr)
    {
        return nullptr;
    }
#else
    NtdllLibrary::OBJECT_ATTRIBUTES attr;
    NtdllLibrary::Instance->InitializeObjectAttributes(&attr, NULL, NtdllLibrary::OBJ_KERNEL_HANDLE, NULL, NULL);
    LARGE_INTEGER size = { 0 };
#if TARGET_32
    size.LowPart = sectionSize;
#elif TARGET_64
    size.QuadPart = sectionSize;
#endif
    HANDLE handle = nullptr;
    int status = NtdllLibrary::Instance->CreateSection(&handle, SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_QUERY | SECTION_MAP_EXECUTE, &attr, &size, PAGE_EXECUTE_READWRITE, allocAttributes, NULL);
    if (status != 0)
    {
        return nullptr;
    }
#endif
    return handle;
}

void UnmapView(HANDLE process, PVOID address)
{
#if USEFILEMAP2
    UnmapViewOfFile2(process, address, 0);
#else
    NtdllLibrary::Instance->UnmapViewOfSection(process, address);
#endif
}

PVOID MapView(HANDLE process, HANDLE sectionHandle, size_t size, size_t offset, bool local)
{
    PVOID address = nullptr;
    DWORD flags = 0;
    if (local)
    {
        if (process != GetCurrentProcess())
        {
            return nullptr;
        }
        flags = PAGE_READWRITE;
    }
    else
    {
        if (process == GetCurrentProcess())
        {
            return nullptr;
        }
        flags = AutoSystemInfo::Data.IsCFGEnabled() ? PAGE_EXECUTE_RO_TARGETS_INVALID : PAGE_EXECUTE_READ;
    }

#if USEFILEMAP2
    address = MapViewOfFile2(sectionHandle, process, offset, nullptr, size, NULL, flags);
    if (local && address != nullptr)
    {
        address = VirtualAlloc(address, size, MEM_COMMIT, flags);
    }
#else
    LARGE_INTEGER mapOffset = { 0 };
#if TARGET_32
    mapOffset.LowPart = offset;
#elif TARGET_64
    mapOffset.QuadPart = offset;
#else
    CompileAssert(UNREACHED);
#endif
    SIZE_T viewSize = size;
    int status = NtdllLibrary::Instance->MapViewOfSection(sectionHandle, process, &address, NULL, viewSize, &mapOffset, &viewSize, NtdllLibrary::ViewUnmap, NULL, flags);
    if (status != 0)
    {
        return nullptr;
    }
#endif
    return address;
}

#if defined(_M_X64_OR_ARM64)
SectionMap32::SectionMap32(__in char * startAddress) :
    startAddress(startAddress),
#else
SectionMap32::SectionMap32() :
#endif
    count(0)
{
    memset(map, 0, sizeof(map));

#if defined(_M_X64_OR_ARM64)
    Assert(((size_t)startAddress) % TotalSize == 0);
#endif
}

SectionMap32::~SectionMap32()
{
    for (uint i = 0; i < _countof(map); i++)
    {
        L2MapChunk * chunk = map[i];
        if (chunk)
        {
            HeapDelete(chunk);
        }
    }
}

SectionInfo *
SectionMap32::GetSection(void* address)
{

    uint id1 = GetLevel1Id(address);

    L2MapChunk * l2map = map[id1];
    if (l2map == nullptr)
    {
        return nullptr;
    }

    return l2map->Get(address);
}

void
SectionMap32::Cleanup()
{
    for (uint id1 = 0; id1 < L1Count; id1++)
    {
        L2MapChunk * l2map = map[id1];
        if (l2map != nullptr && l2map->IsEmpty())
        {
            map[id1] = nullptr;
            HeapDelete(l2map);
            Assert(count > 0);
            count--;
        }
    }
}

bool
SectionMap32::EnsureSection(void * address, uint pageCount)
{
    uint id1 = GetLevel1Id(address);
    uint id2 = GetLevel2Id(address);

    uint currentPageCount = min(pageCount, L2Count - id2);

    while (true)
    {
        if (map[id1] == nullptr)
        {
            L2MapChunk * newChunk = HeapNewNoThrowZ(L2MapChunk);
            if (newChunk == nullptr)
            {
                // remove any previously allocated L2 maps
                Cleanup();

                return false;
            }

            map[id1] = newChunk;
            count++;
        }

        pageCount -= currentPageCount;
        if (pageCount == 0)
        {
            break;
        }

        id2 = 0;
        id1++;
        currentPageCount = min(pageCount, L2Count);
    }

    return true;
}

void
SectionMap32::ClearSection(void * address, uint pageCount)
{
    uint id1 = GetLevel1Id(address);
    uint id2 = GetLevel2Id(address);

    uint currentPageCount = min(pageCount, L2Count - id2);

    while (true)
    {
        Assert(map[id1] != nullptr);

        map[id1]->Clear(id2, currentPageCount);

        pageCount -= currentPageCount;
        if (pageCount == 0)
        {
            return;
        }

        id2 = 0;
        id1++;
        currentPageCount = min(pageCount, L2Count);
    }
}

void
SectionMap32::SetSectionNoCheck(void * address, uint pageCount, SectionInfo * section)
{
    uint id1 = GetLevel1Id(address);
    uint id2 = GetLevel2Id(address);

    uint currentPageCount = min(pageCount, L2Count - id2);

    while (true)
    {
        Assert(map[id1] != nullptr);

        map[id1]->Set(id2, currentPageCount, section);

        pageCount -= currentPageCount;
        if (pageCount == 0)
        {
            return;
        }

        id2 = 0;
        id1++;
        currentPageCount = min(pageCount, L2Count);
    }
}

bool
SectionMap32::SetSection(void * address, uint pageCount, SectionInfo * section)
{
    if (!EnsureSection(address, pageCount))
    {
        return false;
    }

    SetSectionNoCheck(address, pageCount, section);
    return true;
}

SectionMap32::L2MapChunk::~L2MapChunk()
{
    for (uint i = 0; i < L2Count; ++i)
    {
        if (map[i] != nullptr)
        {
            // in case runtime process has abnormal termination, map may not be empty
            CloseSectionHandle(map[i]->handle);
            HeapDelete(map[i]);
        }
    }
}

bool
SectionMap32::L2MapChunk::IsEmpty() const
{
    for (uint i = 0; i < L2Count; i++)
    {
        if (this->map[i] != nullptr)
        {
            return false;
        }
    }

    return true;
}

void
SectionMap32::L2MapChunk::Clear(uint id2, uint pageCount)
{
    uint id2End = id2 + pageCount;

    Assert(id2 < L2Count);
    Assert(id2End <= L2Count);

    for (uint i = id2; i < id2End; i++)
    {
        __analysis_assume(i < L2Count);
        Assert(map[i] != nullptr);
        map[i] = nullptr;
    }
}

void
SectionMap32::L2MapChunk::Set(uint id2, uint pageCount, SectionInfo * section)
{
    uint id2End = id2 + pageCount;

    Assert(id2 < L2Count);
    Assert(id2End <= L2Count);

    for (uint i = id2; i < id2End; i++)
    {
        __analysis_assume(i < L2Count);
        Assert(map[i] == nullptr);
        map[i] = section;
    }
}

SectionInfo *
SectionMap32::L2MapChunk::Get(void * address)
{
    uint id2 = GetLevel2Id(address);
    Assert(id2 < L2Count);
    __analysis_assume(id2 < L2Count);
    return map[id2];
}

#if TARGET_64

SectionMap64::SectionMap64() : list(nullptr)
{
}

SectionMap64::~SectionMap64()
{
    Node * node = list;
    list = nullptr;

    while (node != nullptr)
    {
        Node * next = node->next;
        HeapDelete(node);
        node = next;
    }
}

bool
SectionMap64::EnsureSection(void * address, size_t pageCount)
{
    uint lowerBitsAddress = ::Math::PointerCastToIntegralTruncate<uint>(address);
    size_t pageCountLeft = pageCount;
    uint nodePages = PagesPer4GB - lowerBitsAddress / AutoSystemInfo::PageSize;
    if (pageCountLeft < nodePages)
    {
        nodePages = (uint)pageCountLeft;
    }

    do
    {
        Node * node = FindOrInsertNode(address);
        if (node == nullptr || !node->map.EnsureSection(address, nodePages))
        {
            return false;
        }

        pageCountLeft -= nodePages;
        if (pageCountLeft == 0)
        {
            return true;
        }
        address = (void *)((size_t)address + (nodePages * AutoSystemInfo::PageSize));
        nodePages = PagesPer4GB;
        if (pageCountLeft < PagesPer4GB)
        {
            nodePages = (uint)pageCountLeft;
        }
    } while (true);
}

void
SectionMap64::SetSectionNoCheck(void * address, size_t pageCount, SectionInfo * section)
{
    ForEachNodeInAddressRange(address, pageCount, [&](Node * node, void * address, uint nodePages)
    {
        Assert(node != nullptr);
        node->map.SetSectionNoCheck(address, nodePages, section);
    });
}

bool
SectionMap64::SetSection(void * address, uint pageCount, SectionInfo * section)
{
    if (!EnsureSection(address, pageCount))
    {
        return false;
    }

    SetSectionNoCheck(address, pageCount, section);

    return true;
}

SectionInfo *
SectionMap64::GetSection(void * address)
{
    Node * node = FindNode(address);
    if (node == nullptr)
    {
        return nullptr;
    }
    return node->map.GetSection(address);
}

void
SectionMap64::ClearSection(void * address, uint pageCount)
{
    ForEachNodeInAddressRange(address, pageCount, [&](Node* node, void* address, uint nodePages)
    {
        Assert(node != nullptr);
        node->map.ClearSection(address, nodePages);
    });
}

template <class Fn>
void SectionMap64::ForEachNodeInAddressRange(void * address, size_t pageCount, Fn fn)
{
    uint lowerBitsAddress = ::Math::PointerCastToIntegralTruncate<uint>(address);
    uint nodePages = SectionMap64::PagesPer4GB - lowerBitsAddress / AutoSystemInfo::PageSize;
    if (pageCount < nodePages)
    {
        nodePages = (uint)pageCount;
    }

    do
    {
        Node * node = FindNode(address);

        fn(node, address, nodePages);

        pageCount -= nodePages;
        if (pageCount == 0)
        {
            break;
        }
        address = (void *)((size_t)address + (nodePages * AutoSystemInfo::PageSize));
        nodePages = SectionMap64::PagesPer4GB;
        if (pageCount < SectionMap64::PagesPer4GB)
        {
            nodePages = (uint)pageCount;
        }
    } while (true);
}

SectionMap64::Node *
SectionMap64::FindOrInsertNode(void * address)
{
    Node * node = FindNode(address);

    if (node == nullptr)
    {
        node = HeapNewNoThrowZ(Node, GetNodeStartAddress(address));
        if (node != nullptr)
        {
            node->nodeIndex = GetNodeIndex(address);
            node->next = list;
            list = node;
        }
    }

    return node;
}

SectionMap64::Node *
SectionMap64::FindNode(void * address) const
{
    uint index = GetNodeIndex(address);

    Node * node = list;
    while (node != nullptr)
    {
        if (node->nodeIndex == index)
        {
            return node;
        }

        node = node->next;
    }

    return nullptr;
}

#endif //TARGET_64

static const uint SectionAlignment = 65536;

PVOID
AllocLocalView(HANDLE sectionHandle, LPVOID remoteBaseAddr, LPVOID remoteRequestAddress, size_t requestSize)
{
    const size_t offset = (uintptr_t)remoteRequestAddress - (uintptr_t)remoteBaseAddr;
    const size_t offsetAlignment = offset % SectionAlignment;
    const size_t alignedOffset = offset - offsetAlignment;
    const size_t viewSize = requestSize + offsetAlignment;
    
    PVOID address = MapView(GetCurrentProcess(), sectionHandle, viewSize, alignedOffset, true);
    if (address == nullptr)
    {
        return nullptr;
    }
    return (PVOID)((uintptr_t)address + offsetAlignment);
}

BOOL
FreeLocalView(LPVOID lpAddress)
{
    const size_t alignment = (uintptr_t)lpAddress % SectionAlignment;
    UnmapView(GetCurrentProcess(), (LPVOID)((uintptr_t)lpAddress - alignment));
    return TRUE;
}

SectionAllocWrapper::SectionAllocWrapper(HANDLE process) :
    process(process)
{
    Assert(process != GetCurrentProcess()); // only use sections when OOP
}

LPVOID
SectionAllocWrapper::Alloc(LPVOID requestAddress, size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation)
{
    Assert(isCustomHeapAllocation);

    LPVOID address = nullptr;

#if defined(ENABLE_JIT_CLAMP)
    // REVIEW: is this needed?
    AutoEnableDynamicCodeGen enableCodeGen(true);
#endif
    HANDLE sectionHandle = nullptr;
    SectionInfo * section = nullptr;

    // for new allocations, create new section and fully map it (reserved) into runtime process
    if (requestAddress == nullptr)
    {
        sectionHandle = CreateSection(dwSize, ((allocationType & MEM_COMMIT) == MEM_COMMIT));
        if (sectionHandle == nullptr)
        {
            goto FailureCleanup;
        }
        address = MapView(this->process, sectionHandle, 0, 0, false);
        if(address == nullptr)
        {
            goto FailureCleanup;
        }

        section = HeapNewNoThrowStruct(SectionInfo);
        if (section == nullptr)
        {
            goto FailureCleanup;
        }
        section->handle = sectionHandle;
        section->runtimeBaseAddress = address;
        if (!sections.SetSection(address, (uint)(dwSize / AutoSystemInfo::PageSize), section))
        {
            goto FailureCleanup;
        }
    }
    else
    {
        if (!sections.GetSection(requestAddress))
        {
            return nullptr;
        }
        address = requestAddress;

        if ((allocationType & MEM_COMMIT) == MEM_COMMIT)
        {
            const DWORD allocProtectFlags = AutoSystemInfo::Data.IsCFGEnabled() ? PAGE_EXECUTE_RO_TARGETS_INVALID : PAGE_EXECUTE_READ;
            address = VirtualAllocEx(this->process, address, dwSize, MEM_COMMIT, allocProtectFlags);
        }
    }

    return address;

FailureCleanup:
    // if section allocation failed, free whatever we started to allocate
    if (sectionHandle != nullptr)
    {
        CloseSectionHandle(sectionHandle);
    }
    if (address != nullptr)
    {
        UnmapView(this->process, address);
    }
    if (section != nullptr)
    {
        HeapDelete(section);
    }
    return nullptr;
}

LPVOID
SectionAllocWrapper::AllocLocal(LPVOID requestAddress, size_t dwSize)
{
    SectionInfo * section = sections.GetSection(requestAddress);
    Assert(section);
    return AllocLocalView(section->handle, section->runtimeBaseAddress, requestAddress, dwSize);
}

BOOL SectionAllocWrapper::FreeLocal(LPVOID lpAddress)
{
    return FreeLocalView(lpAddress);
}

BOOL SectionAllocWrapper::Free(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType)
{
    Assert(dwSize % AutoSystemInfo::PageSize == 0);
    Assert(this->process != GetCurrentProcess()); // only use sections when OOP

    if ((dwFreeType & MEM_RELEASE) == MEM_RELEASE)
    {
        SectionInfo * section = sections.GetSection(lpAddress);
        Assert(section);
        Assert(section->runtimeBaseAddress == lpAddress);
        sections.ClearSection(lpAddress, (uint)(dwSize / AutoSystemInfo::PageSize));

        UnmapView(this->process, lpAddress);
        CloseSectionHandle(section->handle);
    }
    else
    {
        Assert((dwFreeType & MEM_DECOMMIT) == MEM_DECOMMIT);
        for (size_t i = 0; i < dwSize / AutoSystemInfo::PageSize; ++i)
        {
            LPVOID localAddr = AllocLocal((char*)lpAddress + i * AutoSystemInfo::PageSize, AutoSystemInfo::PageSize);
            if (localAddr == nullptr)
            {
                return FALSE;
            }
            ZeroMemory(localAddr, AutoSystemInfo::PageSize);
            FreeLocal(localAddr);
        }
        DWORD oldFlags = NULL;
        if (!VirtualProtectEx(this->process, lpAddress, dwSize, PAGE_NOACCESS, &oldFlags))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*
* class PreReservedVirtualAllocWrapper
*/
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
// TODO: this should be on runtime process
uint PreReservedSectionAllocWrapper::numPreReservedSegment = 0;
#endif

PreReservedSectionAllocWrapper::PreReservedSectionAllocWrapper(HANDLE process) :
    preReservedStartAddress(nullptr),
    process(process),
    cs(4000),
    section(nullptr)
{
    Assert(process != GetCurrentProcess()); // only use sections when OOP
    freeSegments.SetAll();
}

PreReservedSectionAllocWrapper::~PreReservedSectionAllocWrapper()
{
    if (IsPreReservedRegionPresent())
    {
        UnmapView(this->process, this->preReservedStartAddress);
        CloseSectionHandle(this->section);
        PreReservedHeapTrace(_u("MEM_RELEASE the PreReservedSegment. Start Address: 0x%p, Size: 0x%x * 0x%x bytes"), this->preReservedStartAddress, PreReservedAllocationSegmentCount,
            AutoSystemInfo::Data.GetAllocationGranularityPageSize());
#if !_M_X64_OR_ARM64 && _CONTROL_FLOW_GUARD
        Assert(numPreReservedSegment > 0);
        InterlockedDecrement(&PreReservedSectionAllocWrapper::numPreReservedSegment);
#endif
    }
}

bool
PreReservedSectionAllocWrapper::IsPreReservedRegionPresent()
{
    return this->preReservedStartAddress != nullptr;
}

bool
PreReservedSectionAllocWrapper::IsInRange(void * address)
{
    if (!this->IsPreReservedRegionPresent())
    {
        return false;
    }
    bool isInRange = IsInRange(GetPreReservedStartAddress(), address);
#if DBG
    if (isInRange)
    {
        MEMORY_BASIC_INFORMATION memBasicInfo;
        size_t bytes = VirtualQueryEx(this->process, address, &memBasicInfo, sizeof(memBasicInfo));
        Assert(bytes == 0 || (memBasicInfo.State == MEM_COMMIT && memBasicInfo.AllocationProtect == PAGE_EXECUTE_READ));
    }
#endif
    return isInRange;
}

/* static */
bool
PreReservedSectionAllocWrapper::IsInRange(void * regionStart, void * address)
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
PreReservedSectionAllocWrapper::GetPreReservedStartAddress()
{
    return this->preReservedStartAddress;
}

LPVOID
PreReservedSectionAllocWrapper::GetPreReservedEndAddress()
{
    Assert(IsPreReservedRegionPresent());
    return GetPreReservedEndAddress(this->preReservedStartAddress);
}

/* static */
LPVOID
PreReservedSectionAllocWrapper::GetPreReservedEndAddress(void * regionStart)
{
    return (char*)regionStart + (PreReservedAllocationSegmentCount * AutoSystemInfo::Data.GetAllocationGranularityPageCount() * AutoSystemInfo::PageSize);
}


LPVOID PreReservedSectionAllocWrapper::EnsurePreReservedRegion()
{
    LPVOID startAddress = this->preReservedStartAddress;
    if (startAddress != nullptr)
    {
        return startAddress;
    }

    {
        AutoCriticalSection autocs(&this->cs);
        return EnsurePreReservedRegionInternal();
    }
}

LPVOID PreReservedSectionAllocWrapper::EnsurePreReservedRegionInternal()
{
    LPVOID startAddress = this->preReservedStartAddress;
    if (startAddress != nullptr)
    {
        return startAddress;
    }

    size_t bytes = PreReservedAllocationSegmentCount * AutoSystemInfo::Data.GetAllocationGranularityPageSize();
    if (PHASE_FORCE1(Js::PreReservedHeapAllocPhase))
    {
        HANDLE sectionHandle = CreateSection(bytes, false);
        if (sectionHandle == nullptr)
        {
            return nullptr;
        }
        startAddress = MapView(this->process, sectionHandle, 0, 0, false);
        if (startAddress == nullptr)
        {
            CloseSectionHandle(sectionHandle);
            return nullptr;
        }
        PreReservedHeapTrace(_u("Reserving PreReservedSegment For the first time(CFG Non-Enabled). Address: 0x%p\n"), this->preReservedStartAddress);
        this->preReservedStartAddress = startAddress;
        this->section = sectionHandle;
        return startAddress;
    }

#if defined(_CONTROL_FLOW_GUARD)
    bool supportPreReservedRegion = true;
#if TARGET_32
#if _M_IX86
    // We want to restrict the number of prereserved segment for 32-bit process so that we don't use up the address space

    // Note: numPreReservedSegment is for the whole process, and access and update to it is not protected by a global lock.
    // So we may allocate more than the maximum some of the time if multiple thread check it simutaniously and allocate pass the limit.
    // It doesn't affect functionally, and it should be OK if we exceed.

    if (PreReservedSectionAllocWrapper::numPreReservedSegment > PreReservedSectionAllocWrapper::MaxPreReserveSegment)
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
        HANDLE sectionHandle = CreateSection(bytes, false);
        if (sectionHandle == nullptr)
        {
            return nullptr;
        }
        startAddress = MapView(this->process, sectionHandle, 0, 0, false);
        if (startAddress == nullptr)
        {
            CloseSectionHandle(sectionHandle);
            return nullptr;
        }
        PreReservedHeapTrace(_u("Reserving PreReservedSegment For the first time(CFG Enabled). Address: 0x%p\n"), this->preReservedStartAddress);
        this->preReservedStartAddress = startAddress;
        this->section = sectionHandle;

#if TARGET_32
        if (startAddress)
        {
            InterlockedIncrement(&PreReservedSectionAllocWrapper::numPreReservedSegment);
        }
#endif
    }
#endif


    return startAddress;
}

LPVOID PreReservedSectionAllocWrapper::Alloc(LPVOID lpAddress, size_t dwSize, DWORD allocationType, DWORD protectFlags, bool isCustomHeapAllocation)
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
            addressToReserve = (char*)this->preReservedStartAddress + offset;
        }
        else
        {
            //Check If the lpAddress is within the range of the preReserved Memory Region
            Assert(((char*)lpAddress) >= (char*)this->preReservedStartAddress || ((char*)lpAddress + dwSize) < GetPreReservedEndAddress());

            addressToReserve = (char*)lpAddress;
            freeSegmentsBVIndex = (uint)((addressToReserve - (char*)this->preReservedStartAddress) / AutoSystemInfo::Data.GetAllocationGranularityPageSize());
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

        if ((allocationType & MEM_COMMIT) != 0)
        {
#if defined(ENABLE_JIT_CLAMP)
            AutoEnableDynamicCodeGen enableCodeGen;
#endif

            const DWORD allocProtectFlags = AutoSystemInfo::Data.IsCFGEnabled() ? PAGE_EXECUTE_RO_TARGETS_INVALID : PAGE_EXECUTE_READ;
            allocatedAddress = (char *)VirtualAllocEx(this->process, addressToReserve, dwSize, MEM_COMMIT, allocProtectFlags);
            if (allocatedAddress == nullptr)
            {
                MemoryOperationLastError::RecordLastError();
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
        return allocatedAddress;
    }
}

BOOL
PreReservedSectionAllocWrapper::Free(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType)
{
    AutoCriticalSection autocs(&this->cs);

    if (dwSize == 0)
    {
        Assert(false);
        return FALSE;
    }

    if (this->preReservedStartAddress == nullptr)
    {
        Assert(false);
        return FALSE;
    }

    Assert(dwSize % AutoSystemInfo::PageSize == 0);

    // zero one page at a time to minimize working set impact while zeroing
    for (size_t i = 0; i < dwSize / AutoSystemInfo::PageSize; ++i)
    {
        LPVOID localAddr = AllocLocal((char*)lpAddress + i * AutoSystemInfo::PageSize, AutoSystemInfo::PageSize);
        if (localAddr == nullptr)
        {
            return FALSE;
        }
        ZeroMemory(localAddr, AutoSystemInfo::PageSize);
        FreeLocal(localAddr);
    }

    DWORD oldFlags = NULL;
    if(!VirtualProtectEx(this->process, lpAddress, dwSize, PAGE_NOACCESS, &oldFlags))
    {
        return FALSE;
    }

    size_t requestedNumOfSegments = dwSize / AutoSystemInfo::Data.GetAllocationGranularityPageSize();
    Assert(requestedNumOfSegments <= MAXUINT32);

    PreReservedHeapTrace(_u("MEM_DECOMMIT: Address: 0x%p of size: 0x%x bytes\n"), lpAddress, dwSize);

    if ((dwFreeType & MEM_RELEASE) == MEM_RELEASE)
    {
        Assert((uintptr_t)lpAddress >= (uintptr_t)this->preReservedStartAddress);
        AssertMsg(((uintptr_t)lpAddress & (AutoSystemInfo::Data.GetAllocationGranularityPageCount() - 1)) == 0, "Not aligned with Allocation Granularity?");
        AssertMsg(dwSize % AutoSystemInfo::Data.GetAllocationGranularityPageSize() == 0, "Release size should match the allocation granularity size");
        Assert(requestedNumOfSegments != 0);

        BVIndex freeSegmentsBVIndex = (BVIndex)(((uintptr_t)lpAddress - (uintptr_t)this->preReservedStartAddress) / AutoSystemInfo::Data.GetAllocationGranularityPageSize());
        AssertMsg(freeSegmentsBVIndex < PreReservedAllocationSegmentCount, "Invalid Index ?");
        freeSegments.SetRange(freeSegmentsBVIndex, static_cast<uint>(requestedNumOfSegments));
        PreReservedHeapTrace(_u("MEM_RELEASE: Address: 0x%p of size: 0x%x * 0x%x bytes\n"), lpAddress, requestedNumOfSegments, AutoSystemInfo::Data.GetAllocationGranularityPageSize());
    }

    return TRUE;
}


LPVOID
PreReservedSectionAllocWrapper::AllocLocal(LPVOID requestAddress, size_t dwSize)
{
    return AllocLocalView(this->section, this->preReservedStartAddress, requestAddress, dwSize);
}

BOOL
PreReservedSectionAllocWrapper::FreeLocal(LPVOID lpAddress)
{
    return FreeLocalView(lpAddress);
}

} // namespace Memory
#endif

