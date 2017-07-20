//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

#define UpdateMinimum(dst, src) if (dst > src) { dst = src; }

#if ENABLE_OOP_NATIVE_CODEGEN
THREAD_LOCAL HRESULT MemoryOperationLastError::MemOpLastError = 0;
#endif

//=============================================================================================================
// Segment
//=============================================================================================================

SegmentBaseCommon::SegmentBaseCommon(PageAllocatorBaseCommon* allocator)
    : allocator(allocator)
{
}

bool SegmentBaseCommon::IsInPreReservedHeapPageAllocator() const
{
    return allocator->GetAllocatorType() == PageAllocatorBaseCommon::AllocatorType::PreReservedVirtualAlloc
#if ENABLE_OOP_NATIVE_CODEGEN
        || allocator->GetAllocatorType() == PageAllocatorBaseCommon::AllocatorType::PreReservedSectionAlloc
#endif
        ;
}

template<typename T>
SegmentBase<T>::SegmentBase(PageAllocatorBase<T> * allocator, size_t pageCount, bool enableWriteBarrier) :
    SegmentBaseCommon(allocator),
    address(nullptr),
    trailingGuardPageCount(0),
    leadingGuardPageCount(0),
    secondaryAllocPageCount(allocator->secondaryAllocPageCount),
    secondaryAllocator(nullptr)
#if defined(_M_X64_OR_ARM64) && defined(RECYCLER_WRITE_BARRIER)
    , isWriteBarrierAllowed(false)
    , isWriteBarrierEnabled(enableWriteBarrier)
#endif
{
    this->segmentPageCount = pageCount + secondaryAllocPageCount;
}

template<typename T>
SegmentBase<T>::~SegmentBase()
{
    Assert(this->allocator != nullptr);

    // Cleanup secondaryAllocator before releasing pages so the destructor
    // still has access to segment memory.
    if(this->secondaryAllocator)
    {
        this->secondaryAllocator->Delete();
        this->secondaryAllocator = nullptr;
    }

    if (this->address)
    {
        char* originalAddress = this->address - (leadingGuardPageCount * AutoSystemInfo::PageSize);
        GetAllocator()->GetVirtualAllocator()->Free(originalAddress, GetPageCount() * AutoSystemInfo::PageSize, MEM_RELEASE);
        GetAllocator()->ReportFree(this->segmentPageCount * AutoSystemInfo::PageSize); //Note: We reported the guard pages free when we decommitted them during segment initialization
#if defined(_M_X64_OR_ARM64) && defined(RECYCLER_WRITE_BARRIER_BYTE)
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (CONFIG_FLAG(StrictWriteBarrierCheck) && this->isWriteBarrierEnabled)
        {
            RecyclerWriteBarrierManager::ToggleBarrier(this->address, this->segmentPageCount * AutoSystemInfo::PageSize, false);
        }
#endif
        RecyclerWriteBarrierManager::OnSegmentFree(this->address, this->segmentPageCount);
#endif
    }
}

template<typename T>
bool
SegmentBase<T>::Initialize(DWORD allocFlags, bool excludeGuardPages)
{
    Assert(this->address == nullptr);
    char* originalAddress = nullptr;
    bool addGuardPages = false;
    if (!excludeGuardPages)
    {
        addGuardPages = (this->segmentPageCount * AutoSystemInfo::PageSize) > VirtualAllocThreshold;
#if _M_IX86_OR_ARM32
        unsigned int randomNumber2 = static_cast<unsigned int>(Math::Rand());
        addGuardPages = addGuardPages && (randomNumber2 % 4 == 1);
#endif
#if DEBUG
        addGuardPages = addGuardPages || Js::Configuration::Global.flags.ForceGuardPages;
#endif
        if (addGuardPages)
        {
            unsigned int randomNumber = static_cast<unsigned int>(Math::Rand());
            this->leadingGuardPageCount = randomNumber % maxGuardPages + minGuardPages;
            this->trailingGuardPageCount = minGuardPages;
        }
    }

    // We can only allocate with this granularity using VirtualAlloc
    size_t totalPages = Math::Align<size_t>(this->segmentPageCount + leadingGuardPageCount + trailingGuardPageCount, AutoSystemInfo::Data.GetAllocationGranularityPageCount());
    this->segmentPageCount = totalPages - (leadingGuardPageCount + trailingGuardPageCount);

#ifdef FAULT_INJECTION
    if (Js::FaultInjection::Global.ShouldInjectFault(Js::FaultInjection::Global.NoThrow))
    {
        this->address = nullptr;
        return false;
    }
#endif

    if (!this->GetAllocator()->RequestAlloc(totalPages * AutoSystemInfo::PageSize))
    {
        return false;
    }

    this->address = (char *)GetAllocator()->GetVirtualAllocator()->Alloc(NULL, totalPages * AutoSystemInfo::PageSize, MEM_RESERVE | allocFlags, PAGE_READWRITE, this->IsInCustomHeapAllocator());

    if (this->address == nullptr)
    {
        this->GetAllocator()->ReportFailure(totalPages * AutoSystemInfo::PageSize);
        return false;
    }

    Assert( ((ULONG_PTR)this->address % (64 * 1024)) == 0 );

    originalAddress = this->address;
    bool committed = (allocFlags & MEM_COMMIT) != 0;
    if (addGuardPages)
    {
#if DBG_DUMP
        GUARD_PAGE_TRACE(_u("Number of Leading Guard Pages: %d\n"), leadingGuardPageCount);
        GUARD_PAGE_TRACE(_u("Starting address of Leading Guard Pages: 0x%p\n"), address);
        GUARD_PAGE_TRACE(_u("Offset of Segment Start address: 0x%p\n"), this->address + (leadingGuardPageCount*AutoSystemInfo::PageSize));
        GUARD_PAGE_TRACE(_u("Starting address of Trailing Guard Pages: 0x%p\n"), address + ((leadingGuardPageCount + this->segmentPageCount)*AutoSystemInfo::PageSize));
#endif
        if (committed)
        {
            GetAllocator()->GetVirtualAllocator()->Free(address,
              leadingGuardPageCount * AutoSystemInfo::PageSize, MEM_DECOMMIT);

            GetAllocator()->GetVirtualAllocator()->Free(address +
              ((leadingGuardPageCount + this->segmentPageCount) * AutoSystemInfo::PageSize),
              trailingGuardPageCount * AutoSystemInfo::PageSize, MEM_DECOMMIT);
        }
        this->GetAllocator()->ReportFree((leadingGuardPageCount + trailingGuardPageCount) * AutoSystemInfo::PageSize);

        this->address = this->address + (leadingGuardPageCount*AutoSystemInfo::PageSize);
    }

    if (!GetAllocator()->CreateSecondaryAllocator(this, committed, &this->secondaryAllocator))
    {
        GetAllocator()->GetVirtualAllocator()->Free(originalAddress,
          GetPageCount() * AutoSystemInfo::PageSize, MEM_RELEASE);
        this->GetAllocator()->ReportFailure(GetPageCount() * AutoSystemInfo::PageSize);
        this->address = nullptr;
        return false;
    }

#ifdef RECYCLER_WRITE_BARRIER
#if defined(_M_X64_OR_ARM64) && defined(RECYCLER_WRITE_BARRIER_BYTE)
    bool registerBarrierResult = true;
#if ENABLE_DEBUG_CONFIG_OPTIONS
    if (CONFIG_FLAG(StrictWriteBarrierCheck))
    {
        if (this->isWriteBarrierEnabled)
        {
            // only commit card table for write barrier pages for strict check
            // we can do this in free build if all write barrier annotated struct
            // only allocate with write barrier pages
            registerBarrierResult = RecyclerWriteBarrierManager::OnSegmentAlloc(this->address, this->segmentPageCount);
        }
    }
    else
#endif
    {
        registerBarrierResult = RecyclerWriteBarrierManager::OnSegmentAlloc(this->address, this->segmentPageCount);
    }

    if (!registerBarrierResult)
    {
        GetAllocator()->GetVirtualAllocator()->Free(originalAddress,
          GetPageCount() * AutoSystemInfo::PageSize, MEM_RELEASE);
        this->GetAllocator()->ReportFailure(GetPageCount() * AutoSystemInfo::PageSize);
        this->address = nullptr;
        return false;
    }
#endif

    this->isWriteBarrierAllowed = true;
#if DBG

    if (this->isWriteBarrierEnabled)
    {
        RecyclerWriteBarrierManager::ToggleBarrier(this->address,
          this->segmentPageCount * AutoSystemInfo::PageSize, true);
    }
#endif
#endif

    return true;
}

//=============================================================================================================
// PageSegment
//=============================================================================================================

template<typename T>
PageSegmentBase<T>::PageSegmentBase(PageAllocatorBase<T> * allocator, bool committed, bool allocated, bool enableWriteBarrier) :
    SegmentBase<T>(allocator, allocator->maxAllocPageCount, enableWriteBarrier), decommitPageCount(0)
{
    Assert(this->segmentPageCount == allocator->maxAllocPageCount + allocator->secondaryAllocPageCount);

    uint maxPageCount = GetMaxPageCount();

    if (committed)
    {
        Assert(!allocated);
        this->freePageCount = this->GetAvailablePageCount();
        this->SetRangeInFreePagesBitVector(0, this->freePageCount);
        if (this->freePageCount != maxPageCount)
        {
            this->ClearRangeInFreePagesBitVector(this->freePageCount, (maxPageCount - this->freePageCount));
        }

        Assert(this->GetCountOfFreePages() == this->freePageCount);
    }
    else
    {
        this->freePageCount = 0;
        this->ClearAllInFreePagesBitVector();
        if (!allocated)
        {
            this->decommitPageCount = this->GetAvailablePageCount();
            this->SetRangeInDecommitPagesBitVector(0, this->decommitPageCount);

            if (this->decommitPageCount != maxPageCount)
            {
                this->ClearRangeInDecommitPagesBitVector(this->decommitPageCount, (maxPageCount - this->decommitPageCount));
            }
        }
    }
}

template<typename T>
PageSegmentBase<T>::PageSegmentBase(PageAllocatorBase<T> * allocator, void* address, uint pageCount, uint committedCount, bool enableWriteBarrier) :
    SegmentBase<T>(allocator, allocator->maxAllocPageCount, enableWriteBarrier), decommitPageCount(0), freePageCount(0)
{
    this->address = (char*)address;
    this->segmentPageCount = pageCount;
}

#ifdef PAGEALLOCATOR_PROTECT_FREEPAGE
template<typename T>
bool
PageSegmentBase<T>::Initialize(DWORD allocFlags, bool excludeGuardPages)
{
    Assert(freePageCount + this->GetAllocator()->secondaryAllocPageCount == this->segmentPageCount || freePageCount == 0);
    if (__super::Initialize(allocFlags, excludeGuardPages))
    {
        if (freePageCount != 0)
        {
            if (this->GetAllocator()->processHandle == GetCurrentProcess())
            {
                DWORD oldProtect;
                BOOL vpresult = VirtualProtect(this->address, this->GetAvailablePageCount() * AutoSystemInfo::PageSize, PAGE_NOACCESS, &oldProtect);
                if(vpresult == FALSE)
                {
                    Assert(UNREACHED);
                    return false;
                }
                Assert(oldProtect == PAGE_READWRITE);
            }
        }
        return true;
    }
    return false;
}
#endif

template<typename T>
void
PageSegmentBase<T>::Prime()
{
#ifndef PAGEALLOCATOR_PROTECT_FREEPAGE
    for (uint i = 0; i < this->GetAvailablePageCount(); i++)
    {
        this->address[i * AutoSystemInfo::PageSize] = 0;
    }
#endif
}

template<typename T>
bool
PageSegmentBase<T>::IsAllocationPageAligned(__in char* address, size_t pageCount, uint *nextIndex)
{
    // Require that allocations are aligned at a boundary
    // corresponding to the page count
    // REVIEW: This might actually lead to additional address space fragmentation
    // because of the leading guard pages feature in the page allocator
    // We can restrict the guard pages to be an even number to improve the chances
    // of having the first allocation be aligned but that reduces the effectiveness
    // of having a random number of guard pages
    uintptr_t mask = (pageCount * AutoSystemInfo::PageSize) - 1;
    if ((reinterpret_cast<uintptr_t>(address)& mask) == 0)
    {
        return true;
    }

    if (nextIndex != nullptr)
    {
        *nextIndex = (uint) ((reinterpret_cast<uintptr_t>(address) % (mask + 1)) / AutoSystemInfo::PageSize);
    }

    return false;
}

template<typename T>
template <bool notPageAligned>
char *
PageSegmentBase<T>::AllocPages(uint pageCount)
{
    Assert(freePageCount != 0);
    Assert(freePageCount == (uint)this->GetCountOfFreePages());
    if (freePageCount < pageCount)
    {
        return nullptr;
    }
    Assert(!IsFull());

#pragma prefast(push)
#pragma prefast(suppress:__WARNING_LOOP_INDEX_UNDERFLOW, "Prefast about overflow when multiplying index.")
    uint index = this->GetNextBitInFreePagesBitVector(0);
    while (index != -1)
    {
        Assert(index < this->GetAllocator()->GetMaxAllocPageCount());

        if (GetAvailablePageCount() - index < pageCount)
        {
            break;
        }
        if (pageCount == 1 || this->TestRangeInFreePagesBitVector(index, pageCount))
        {
            char * allocAddress = this->address + index * AutoSystemInfo::PageSize;

            if (pageCount > 1 && !notPageAligned)
            {
                uint nextIndex = 0;
                if (!IsAllocationPageAligned(allocAddress, pageCount, &nextIndex))
                {
                    if (index + nextIndex >= this->GetAllocator()->GetMaxAllocPageCount())
                    {
                        return nullptr;
                    }
                    index = this->freePages.GetNextBit(index + nextIndex);
                    continue;
                }
            }

            this->ClearRangeInFreePagesBitVector(index, pageCount);
            freePageCount -= pageCount;
            Assert(freePageCount == (uint)this->GetCountOfFreePages());

#ifdef PAGEALLOCATOR_PROTECT_FREEPAGE
            if (this->GetAllocator()->processHandle == GetCurrentProcess())
            {
                DWORD oldProtect;
                BOOL vpresult = VirtualProtect(allocAddress, pageCount * AutoSystemInfo::PageSize, PAGE_READWRITE, &oldProtect);
                if (vpresult == FALSE)
                {
                    Assert(UNREACHED);
                    return nullptr;
                }
                Assert(oldProtect == PAGE_NOACCESS);
            }
#endif
            return allocAddress;
        }
        index = this->GetNextBitInFreePagesBitVector(index + 1);
    }
#pragma prefast(pop)
    return nullptr;
}

#pragma prefast(push)
#pragma prefast(suppress:__WARNING_LOOP_INDEX_UNDERFLOW, "Prefast about overflow when multiplying index.")
template<typename TVirtualAlloc>
template<typename T, bool notPageAligned>
char *
PageSegmentBase<TVirtualAlloc>::AllocDecommitPages(uint pageCount, T freePages, T decommitPages)
{
    Assert(freePageCount == (uint)this->GetCountOfFreePages());
    Assert(decommitPageCount ==  (uint)this->GetCountOfDecommitPages());
    Assert(decommitPageCount != 0);
    if (freePageCount + decommitPageCount < pageCount)
    {
        return nullptr;
    }
    Assert(this->secondaryAllocator == nullptr || this->secondaryAllocator->CanAllocate());

    T freeAndDecommitPages = freePages;

    freeAndDecommitPages.Or(&decommitPages);

    uint oldFreePageCount = freePageCount;
    uint index = freeAndDecommitPages.GetNextBit(0);
    while (index != -1)
    {
        Assert(index < this->GetAllocator()->GetMaxAllocPageCount());

        if (GetAvailablePageCount() - index < pageCount)
        {
            break;
        }
        if (pageCount == 1 || freeAndDecommitPages.TestRange(index, pageCount))
        {
            char * pages = this->address + index * AutoSystemInfo::PageSize;

            if (!notPageAligned)
            {
                uint nextIndex = 0;
                if (!IsAllocationPageAligned(pages, pageCount, &nextIndex))
                {
                    if (index + nextIndex >= this->GetAllocator()->GetMaxAllocPageCount())
                    {
                        return nullptr;
                    }
                    index = freeAndDecommitPages.GetNextBit(index + nextIndex);
                    continue;
                }
            }

            void * ret = this->GetAllocator()->GetVirtualAllocator()->Alloc(pages, pageCount * AutoSystemInfo::PageSize, MEM_COMMIT, PAGE_READWRITE, this->IsInCustomHeapAllocator());
            if (ret != nullptr)
            {
                Assert(ret == pages);

                this->ClearRangeInFreePagesBitVector(index, pageCount);
                this->ClearRangeInDecommitPagesBitVector(index, pageCount);

                uint newFreePageCount = this->GetCountOfFreePages();
                freePageCount = freePageCount - oldFreePageCount + newFreePageCount;
                decommitPageCount -= pageCount - (oldFreePageCount - newFreePageCount);

                Assert(freePageCount == (uint)this->GetCountOfFreePages());
                Assert(decommitPageCount == (uint)this->GetCountOfDecommitPages());

                return pages;
            }
            else if (pageCount == 1)
            {
                // if we failed to commit one page, we should just give up.
                return nullptr;
            }
        }
        index = freeAndDecommitPages.GetNextBit(index + 1);
    }

    return nullptr;
}
#pragma prefast(pop)

template<typename T>
void
PageSegmentBase<T>::ReleasePages(__in void * address, uint pageCount)
{
    Assert(address >= this->address);
    Assert(pageCount <= this->GetAllocator()->maxAllocPageCount);
    Assert(((uint)(((char *)address) - this->address)) <= (this->GetAllocator()->maxAllocPageCount - pageCount) *  AutoSystemInfo::PageSize);
    Assert(!IsFreeOrDecommitted(address, pageCount));

    uint base = this->GetBitRangeBase(address);
    this->SetRangeInFreePagesBitVector(base, pageCount);
    this->freePageCount += pageCount;
    Assert(freePageCount == (uint)this->GetCountOfFreePages());

#ifdef PAGEALLOCATOR_PROTECT_FREEPAGE
    if (this->GetAllocator()->processHandle == GetCurrentProcess())
    {
        DWORD oldProtect;
        BOOL vpresult = VirtualProtect(address, pageCount * AutoSystemInfo::PageSize, PAGE_NOACCESS, &oldProtect);
        Assert(vpresult != FALSE);
        Assert(oldProtect == PAGE_READWRITE);
    }
#endif

}

template<typename T>
void
PageSegmentBase<T>::ChangeSegmentProtection(DWORD protectFlags, DWORD expectedOldProtectFlags)
{
    // TODO: There is a discrepancy in PageSegmentBase
    // The segment page count is initialized in PageSegmentBase::Initialize. It takes into account
    // the guard pages + any additional pages for alignment.
    // However, the free page count is calculated for the segment before initialize is called.
    // In practice, what happens is the following. The initial segment page count is 256. This
    // ends up being the free page count too. When initialize is called, we allocate the guard
    // pages and the alignment pages, which causes the total page count to be 272. The segment
    // page count is then calculated as total - guard, which means 256 <= segmentPageCount < totalPageCount
    // The code in PageSegment's constructor will mark the pages between 256 and 272 as in use,
    // which is why it generally works. However, it breaks in the case where we want to know the end
    // address of the page. It should really be address + 256 * 4k but this->GetEndAddress will return
    // a value greater than that. Need to do a pass through the counts and make sure that it's rational.
    // For now, simply calculate the end address from the allocator's page count
    Assert(this->GetAllocator()->processHandle == GetCurrentProcess());

    char* segmentEndAddress = this->address + (this->GetAllocator()->GetMaxAllocPageCount() * AutoSystemInfo::PageSize);

    for (char* address = this->address; address < segmentEndAddress; address += AutoSystemInfo::PageSize)
    {
        if (!IsFreeOrDecommitted(address))
        {
            char* endAddress = address;
            do
            {
                endAddress += AutoSystemInfo::PageSize;
            } while (endAddress < segmentEndAddress && !IsFreeOrDecommitted(endAddress));

            Assert(((uintptr_t)(endAddress - address)) < UINT_MAX);
            DWORD regionSize = (DWORD) (endAddress - address);
            DWORD oldProtect = 0;

#if DBG
            MEMORY_BASIC_INFORMATION info = { 0 };
            VirtualQuery(address, &info, sizeof(MEMORY_BASIC_INFORMATION));
            Assert(info.Protect == expectedOldProtectFlags);
#endif

            BOOL fSuccess = VirtualProtect(address, regionSize, protectFlags, &oldProtect);
            Assert(fSuccess == TRUE);
            Assert(oldProtect == expectedOldProtectFlags);

            address = endAddress;
        }
    }
}

template<typename T>
template <bool onlyUpdateState>
void
PageSegmentBase<T>::DecommitPages(__in void * address, uint pageCount)
{
    Assert(address >= this->address);
    Assert(pageCount <= this->GetAllocator()->maxAllocPageCount);
    Assert(((uint)(((char *)address) - this->address))
      <= (this->GetAllocator()->maxAllocPageCount - pageCount) * AutoSystemInfo::PageSize);

    Assert(!IsFreeOrDecommitted(address, pageCount));
    uint base = this->GetBitRangeBase(address);

    this->SetRangeInDecommitPagesBitVector(base, pageCount);
    this->decommitPageCount += pageCount;

    if (!onlyUpdateState)
    {
#pragma warning(suppress: 6250)
        this->GetAllocator()->GetVirtualAllocator()->Free(address,
          pageCount * AutoSystemInfo::PageSize, MEM_DECOMMIT);
    }

    Assert(decommitPageCount == (uint)this->GetCountOfDecommitPages());
}

template<typename T>
void
PageSegmentBase<T>::DecommitFreePagesInternal(uint index, uint pageCount)
{
    Assert(pageCount > 0 && (index + pageCount) <= this->GetAvailablePageCount());
    this->ClearRangeInFreePagesBitVector(index, pageCount);
    this->SetRangeInDecommitPagesBitVector(index, pageCount);

    char * currentAddress = this->address + (index * AutoSystemInfo::PageSize);
#pragma warning(suppress: 6250)
    this->GetAllocator()->GetVirtualAllocator()->Free(currentAddress,
      pageCount * AutoSystemInfo::PageSize, MEM_DECOMMIT);
}

template<typename T>
size_t
PageSegmentBase<T>::DecommitFreePages(size_t pageToDecommit)
{
    Assert(pageToDecommit != 0 && this->GetAvailablePageCount() > 0);

    uint startIndex = 0, index = 0, decommitCount = 0;
    do
    {
        if (!this->TestInFreePagesBitVector(index))
        {
            if (startIndex < index)
            {
                uint pageCount = index - startIndex;
                this->DecommitFreePagesInternal(startIndex, pageCount);
            }
            startIndex = index + 1;
        }
        else
        {
            decommitCount++;
        }
    }
    while (++index < this->GetAvailablePageCount() && decommitCount < pageToDecommit);

    if (startIndex < index)
    {
        uint pageCount = index - startIndex;
        this->DecommitFreePagesInternal(startIndex, pageCount);
    }

    Assert(decommitCount <= this->freePageCount);
    this->decommitPageCount += decommitCount;
    this->freePageCount -= decommitCount;
    return decommitCount;
}

//=============================================================================================================
// PageAllocator
//=============================================================================================================
#if DBG
#define ASSERT_THREAD() AssertMsg(this->ValidThreadAccess(), "Page allocation should only be used by a single thread");
#else
#define ASSERT_THREAD()
#endif

/*
 * Global counter to keep track of the total used bytes by the page allocator
 * per process for performance tooling. This is reported through the
 * JSCRIPT_PAGE_ALLOCATOR_USED_SIZE ETW event.
 */

static size_t totalUsedBytes = 0;
static size_t maxUsedBytes = 0;


template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
size_t PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::GetAndResetMaxUsedBytes()
{
    size_t value = maxUsedBytes;
    maxUsedBytes = 0;
    return value;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
size_t
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::GetProcessUsedBytes()
{
    return totalUsedBytes;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
uint
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::GetMaxAllocPageCount()
{
    return maxAllocPageCount;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::PageAllocatorBase(AllocationPolicyManager * policyManager,
    Js::ConfigFlagsTable& flagTable,
    PageAllocatorType type,
    uint maxFreePageCount, bool zeroPages,
#if ENABLE_BACKGROUND_PAGE_FREEING
    BackgroundPageQueue * backgroundPageQueue,
#endif
    uint maxAllocPageCount, uint secondaryAllocPageCount,
    bool stopAllocationOnOutOfMemory, bool excludeGuardPages, HANDLE processHandle, bool enableWriteBarrier) :
    policyManager(policyManager),
    pageAllocatorFlagTable(flagTable),
    maxFreePageCount(maxFreePageCount),
    freePageCount(0),
    allocFlags(0),
    zeroPages(zeroPages),
#if ENABLE_BACKGROUND_PAGE_ZEROING
    queueZeroPages(false),
    hasZeroQueuedPages(false),
    backgroundPageQueue(backgroundPageQueue),
#endif
    minFreePageCount(0),
    isUsed(false),
    idleDecommitEnterCount(1),
    isClosed(false),
    stopAllocationOnOutOfMemory(stopAllocationOnOutOfMemory),
    disableAllocationOutOfMemory(false),
    secondaryAllocPageCount(secondaryAllocPageCount),
    excludeGuardPages(excludeGuardPages),
    type(type)
    , reservedBytes(0)
    , committedBytes(0)
    , usedBytes(0)
    , numberOfSegments(0)
    , processHandle(processHandle)
    , enableWriteBarrier(enableWriteBarrier)
{
    AssertMsg(Math::IsPow2(maxAllocPageCount + secondaryAllocPageCount), "Illegal maxAllocPageCount: Why is this not a power of 2 aligned?");

    this->maxAllocPageCount = maxAllocPageCount;

#if DBG
    // By default, a page allocator is not associated with any thread context
    // Any host which wishes to associate it with a thread context must do so explicitly
    this->threadContextHandle = NULL;
    this->concurrentThreadId = (DWORD)-1;
#endif
#if DBG
    this->disableThreadAccessCheck = false;
    this->debugMinFreePageCount = 0;
#endif
#if DBG_DUMP
    this->decommitPageCount = 0;
    this->debugName = nullptr;
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    this->verifyEnabled = false;
    this->disablePageReuse = false;
#endif

#ifdef PROFILE_MEM
    this->memoryData = MemoryProfiler::GetPageMemoryData(type);
#endif

    PageTracking::PageAllocatorCreated((PageAllocator*)this);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::~PageAllocatorBase()
{
    AssertMsg(this->ValidThreadAccess(), "Page allocator tear-down should only happen on the owning thread");

#if DBG
    Assert(!this->HasMultiThreadAccess());
#endif

    SubUsedBytes(usedBytes);

    SubCommittedBytes(committedBytes);
    SubReservedBytes(reservedBytes);

    ReleaseSegmentList(&segments);
    ReleaseSegmentList(&fullSegments);
    ReleaseSegmentList(&emptySegments);
    ReleaseSegmentList(&decommitSegments);
    ReleaseSegmentList(&largeSegments);

    PageTracking::PageAllocatorDestroyed((PageAllocator*)this);
}

#if ENABLE_BACKGROUND_PAGE_ZEROING
template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::StartQueueZeroPage()
{
    Assert(HasZeroPageQueue());
    Assert(!queueZeroPages);
    queueZeroPages = true;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::StopQueueZeroPage()
{
    Assert(HasZeroPageQueue());
    Assert(queueZeroPages);
    queueZeroPages = false;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
bool
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::HasZeroPageQueue() const
{
    bool hasZeroPageQueue = (ZeroPages() && this->backgroundPageQueue != nullptr);
    Assert(backgroundPageQueue == nullptr || hasZeroPageQueue == backgroundPageQueue->isZeroPageQueue);
    return hasZeroPageQueue;
}

#if DBG
template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
bool
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::HasZeroQueuedPages() const
{
    Assert(!HasZeroPageQueue() || hasZeroQueuedPages ||
        ((ZeroPageQueue *)this->backgroundPageQueue)->QueryDepth() == 0);
    return hasZeroQueuedPages;
}
#endif
#endif //ENABLE_BACKGROUND_PAGE_ZEROING

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
PageAllocation *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AllocPagesForBytes(size_t requestBytes)
{
    Assert(!isClosed);
    ASSERT_THREAD();

    uint pageSize = AutoSystemInfo::PageSize;
    uint addSize = sizeof(PageAllocation) + pageSize - 1;   // this shouldn't overflow
    // overflow check
    size_t allocSize = AllocSizeMath::Add(requestBytes, addSize);
    if (allocSize == (size_t)-1)
    {
        return nullptr;
    }

    size_t pages = allocSize / pageSize;

    return this->AllocAllocation(pages);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
TPageSegment *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AllocPageSegment(
    DListBase<TPageSegment>& segmentList,
    PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment> * pageAllocator,
    bool committed, bool allocated, bool enableWriteBarrier)
{
    TPageSegment * segment = segmentList.PrependNode(&NoThrowNoMemProtectHeapAllocator::Instance,
        pageAllocator, committed, allocated, enableWriteBarrier);

    if (segment == nullptr)
    {
        return nullptr;
    }

    if (!segment->Initialize((committed ? MEM_COMMIT : 0) | pageAllocator->allocFlags, pageAllocator->excludeGuardPages))
    {
        segmentList.RemoveHead(&NoThrowNoMemProtectHeapAllocator::Instance);
        return nullptr;
    }
    return segment;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
TPageSegment *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AllocPageSegment(
    DListBase<TPageSegment>& segmentList,
    PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment> * pageAllocator,
    void* address, uint pageCount, uint committedCount, bool enableWriteBarrier)
{
    TPageSegment * segment = segmentList.PrependNode(&NoThrowNoMemProtectHeapAllocator::Instance,
        pageAllocator, address, pageCount, committedCount, enableWriteBarrier);
    pageAllocator->ReportExternalAlloc(pageCount * AutoSystemInfo::PageSize);
    return segment;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
TPageSegment *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AddPageSegment(DListBase<TPageSegment>& segmentList)
{
    Assert(!this->HasMultiThreadAccess());

    TPageSegment * segment = AllocPageSegment(segmentList, this, true, false, this->enableWriteBarrier);

    if (segment != nullptr)
    {
        this->LogAllocSegment(segment);
        this->AddFreePageCount(this->maxAllocPageCount);
    }
    return segment;
}

#if ENABLE_BACKGROUND_PAGE_FREEING
template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::TryAllocFromZeroPagesList(
    uint pageCount, TPageSegment ** pageSegment, BackgroundPageQueue* bgPageQueue, bool isPendingZeroList)
{
    FAULTINJECT_MEMORY_NOTHROW(this->debugName, pageCount*AutoSystemInfo::PageSize);

    char * pages = nullptr;
    FreePageEntry* localList = nullptr;

#if ENABLE_BACKGROUND_PAGE_ZEROING
    if (CONFIG_FLAG(EnableBGFreeZero))
    {
        while (true)
        {
            FreePageEntry * freePage = isPendingZeroList ? ((ZeroPageQueue *)backgroundPageQueue)->PopZeroPageEntry() : backgroundPageQueue->PopFreePageEntry();
            if (freePage == nullptr)
            {
                break;
            }

            if (freePage->pageCount == pageCount)
            {
                *pageSegment = freePage->segment;
                pages = (char *)freePage;
                memset(pages, 0, isPendingZeroList ? (pageCount*AutoSystemInfo::PageSize) : sizeof(FreePageEntry));
                this->FillAllocPages(pages, pageCount);
                break;
            }
            else
            {
                if (isPendingZeroList)
                {
                    memset((char *)freePage + sizeof(FreePageEntry), 0, (freePage->pageCount*AutoSystemInfo::PageSize) - sizeof(FreePageEntry));
                }

                freePage->Next = localList;
                localList = (FreePageEntry*)freePage;

                if (freePage->pageCount > pageCount)
                {
                    *pageSegment = freePage->segment;
                    freePage->pageCount -= pageCount;
                    pages = (char *)freePage + freePage->pageCount * AutoSystemInfo::PageSize;
                    this->FillAllocPages(pages, pageCount);
                    break;
                }
            }
        }
    }
#endif

    if (localList != nullptr)
    {
        uint newFreePages = 0;
        while (localList != nullptr)
        {
            FreePageEntry* freePagesEntry = localList;
            localList = (FreePageEntry*)localList->Next;

            TPageSegment * segment = freePagesEntry->segment;
            pageCount = freePagesEntry->pageCount;

            DListBase<TPageSegment> * fromSegmentList = GetSegmentList(segment);
            Assert(fromSegmentList != nullptr);

            memset(freePagesEntry, 0, sizeof(FreePageEntry));

            segment->ReleasePages(freePagesEntry, pageCount);
            newFreePages += pageCount;

            TransferSegment(segment, fromSegmentList);

        }

        LogFreePages(newFreePages);
        PAGE_ALLOC_VERBOSE_TRACE(_u("New free pages: %d\n"), newFreePages);
        this->AddFreePageCount(newFreePages);
#if DBG
        UpdateMinimum(this->debugMinFreePageCount, this->freePageCount);
#endif
    }

    return pages;
}
#endif

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::TryAllocFromZeroPages(uint pageCount, TPageSegment ** pageSegment)
{
#if ENABLE_BACKGROUND_PAGE_ZEROING
    if (CONFIG_FLAG(EnableBGFreeZero))
    {
        if (backgroundPageQueue != nullptr)
        {
            return TryAllocFromZeroPagesList(pageCount, pageSegment, backgroundPageQueue, false);
        }

        if (this->hasZeroQueuedPages)
        {
            __analysis_assume(backgroundPageQueue != nullptr);
            return TryAllocFromZeroPagesList(pageCount, pageSegment, backgroundPageQueue, true);
        }
    }
#endif

    return nullptr;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
template <bool notPageAligned>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::TryAllocFreePages(uint pageCount, TPageSegment ** pageSegment)
{
    Assert(!HasMultiThreadAccess());
    char* pages = nullptr;

    if (this->freePageCount >= pageCount)
    {
        FAULTINJECT_MEMORY_NOTHROW(this->debugName, pageCount*AutoSystemInfo::PageSize);
        typename DListBase<TPageSegment>::EditingIterator i(&this->segments);

        while (i.Next())
        {
            TPageSegment * freeSegment = &i.Data();

            pages = freeSegment->template AllocPages<notPageAligned>(pageCount);
            if (pages != nullptr)
            {
                LogAllocPages(pageCount);
                if (freeSegment->GetFreePageCount() == 0)
                {
                    i.MoveCurrentTo(&this->fullSegments);
                }

                this->freePageCount -= pageCount;
                *pageSegment = freeSegment;

#if DBG
                UpdateMinimum(this->debugMinFreePageCount, this->freePageCount);
#endif
                this->FillAllocPages(pages, pageCount);
                return pages;
            }
        }
    }

    pages = TryAllocFromZeroPages(pageCount, pageSegment);

    return pages;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::FillAllocPages(__in void * address, uint pageCount)
{
    const size_t bufferSize = AutoSystemInfo::PageSize * pageCount;

#if DBG
#ifdef RECYCLER_ZERO_MEM_CHECK
    byte * localAddr = (byte *)this->GetVirtualAllocator()->AllocLocal(address, bufferSize);
    if (!localAddr)
    {
        MemoryOperationLastError::RecordError(E_OUTOFMEMORY);
        return;
    }
    for (size_t i = 0; i < bufferSize; i++)
    {
        // new pages are filled with zeros, old pages are filled with DbgMemFill
        Assert(localAddr[i] == 0 || localAddr[i] == DbgMemFill);
    }
    this->GetVirtualAllocator()->FreeLocal(localAddr);
#endif
#endif

#ifdef RECYCLER_MEMORY_VERIFY
    if (verifyEnabled)
    {
        Assert(this->processHandle == GetCurrentProcess());
        memset(address, Recycler::VerifyMemFill, bufferSize);
        return;
    }
#endif

#if DBG
    if (ZeroPages())
    {
        // for release build, the page is zeroed in ReleasePages
        Assert(this->processHandle == GetCurrentProcess());
        memset(address, 0, bufferSize);
    }
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::FillFreePages(__in void * address, uint pageCount)
{
#if DBG
    MemSetLocal(address, DbgMemFill, AutoSystemInfo::PageSize * pageCount);
#else
#ifdef RECYCLER_MEMORY_VERIFY
    if (verifyEnabled)
    {
        return;
    }
#endif
    if (ZeroPages())
    {
        //
        // Do memset via non-temporal store to avoid evicting existing processor cache.
        // This helps low-end machines with limited cache size.
        //
#if defined(_M_IX86) || defined(_M_X64)
        if (CONFIG_FLAG(ZeroMemoryWithNonTemporalStore))
        {
            js_memset_zero_nontemporal(address, AutoSystemInfo::PageSize * pageCount);
        }
        else
#endif
        {
            memset(address, 0, AutoSystemInfo::PageSize * pageCount);
        }
    }
#endif

}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
template <bool notPageAligned>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::TryAllocDecommittedPages(uint pageCount, TPageSegment ** pageSegment)
{
    Assert(!this->HasMultiThreadAccess());

    typename DListBase<TPageSegment>::EditingIterator i(&decommitSegments);

    while (i.Next())
    {
        TPageSegment * freeSegment = &i.Data();
        uint oldFreePageCount = freeSegment->GetFreePageCount();
        uint oldDecommitPageCount = freeSegment->GetDecommitPageCount();

        char * pages = freeSegment->template DoAllocDecommitPages<notPageAligned>(pageCount);
        if (pages != nullptr)
        {
            this->freePageCount = this->freePageCount - oldFreePageCount + freeSegment->GetFreePageCount();

#if DBG_DUMP
            this->decommitPageCount = this->decommitPageCount - oldDecommitPageCount + freeSegment->GetDecommitPageCount();
#endif
#if DBG
            UpdateMinimum(this->debugMinFreePageCount, this->freePageCount);
#endif
            uint recommitPageCount = pageCount - (oldFreePageCount - freeSegment->GetFreePageCount());
            LogRecommitPages(recommitPageCount);
            LogAllocPages(pageCount);

            if (freeSegment->GetDecommitPageCount() == 0)
            {
                auto toList = GetSegmentList(freeSegment);
                i.MoveCurrentTo(toList);
            }

            *pageSegment = freeSegment;
            return pages;
        }
    }
    return nullptr;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
PageAllocation *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AllocAllocation(size_t pageCount)
{
    PageAllocation * pageAllocation;
    TSegment * segment;
    if (pageCount > this->maxAllocPageCount)
    {
        // We need some space reserved for secondary allocations
        segment = AllocSegment(pageCount);
        if (segment == nullptr)
        {
            return nullptr;
        }
        pageAllocation = (PageAllocation *)segment->GetAddress();
        pageAllocation->pageCount = segment->GetAvailablePageCount();
    }
    else
    {
        Assert(pageCount <= UINT_MAX);
        pageAllocation = (PageAllocation *)AllocPages((uint)pageCount, (TPageSegment **)&segment);
        if (pageAllocation == nullptr)
        {
            return nullptr;
        }
        pageAllocation->pageCount = pageCount;
    }
    pageAllocation->segment = segment;

    return pageAllocation;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
TSegment *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AllocSegment(size_t pageCount)
{
    Assert(!isClosed);
    ASSERT_THREAD();

    // Even though we don't idle decommit large segments, we still need to consider these allocations
    // as using the page allocator
    this->isUsed = true;

    TSegment * segment = largeSegments.PrependNode(&NoThrowNoMemProtectHeapAllocator::Instance,
        this, pageCount, enableWriteBarrier);
    if (segment == nullptr)
    {
        return nullptr;
    }
    if (!segment->Initialize(MEM_COMMIT | allocFlags, excludeGuardPages))
    {
        largeSegments.RemoveHead(&NoThrowNoMemProtectHeapAllocator::Instance);
        return nullptr;
    }

    LogAllocSegment(segment);
    LogAllocPages(segment->GetPageCount());

    PageTracking::ReportAllocation((PageAllocator*)this, segment->GetAddress(), AutoSystemInfo::PageSize * segment->GetPageCount());
#ifdef RECYCLER_MEMORY_VERIFY
    if (verifyEnabled)
    {
        Assert(this->processHandle == GetCurrentProcess());
        memset(segment->GetAddress(), Recycler::VerifyMemFill, AutoSystemInfo::PageSize * segment->GetPageCount());
    }
#endif

    return segment;
}

template <>
void PageAllocatorBase<VirtualAllocWrapper>::InitVirtualAllocator(VirtualAllocWrapper * virtualAllocator)
{
    this->allocatorType = GetAllocatorType<VirtualAllocWrapper>();
    // default page allocator must keep virtualAllocator nullptr
    Assert(this->virtualAllocator == nullptr && virtualAllocator == nullptr);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::InitVirtualAllocator(TVirtualAlloc * virtualAllocator)
{
    Assert(this->virtualAllocator == nullptr);
    this->virtualAllocator = virtualAllocator;  // Init to given virtualAllocator, may be nullptr temporarily
    this->allocatorType = GetAllocatorType<TVirtualAlloc>();
}

template <>
VirtualAllocWrapper* PageAllocatorBase<VirtualAllocWrapper>::GetVirtualAllocator() const
{
    Assert(this->allocatorType == GetAllocatorType<VirtualAllocWrapper>());
    return &VirtualAllocWrapper::Instance;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
TVirtualAlloc*
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::GetVirtualAllocator() const
{
    Assert(this->allocatorType == GetAllocatorType<TVirtualAlloc>());
    return reinterpret_cast<TVirtualAlloc*>(this->virtualAllocator);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::Alloc(size_t * pageCount, TSegment ** segment)
{
    Assert(this->allocatorType == GetAllocatorType<TVirtualAlloc>());
    return AllocInternal<false>(pageCount, segment);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
template <bool doPageAlign>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AllocInternal(size_t * pageCount, TSegment ** segment)
{
    char * addr = nullptr;

    if (*pageCount > this->maxAllocPageCount)
    {
        // Don't bother trying to do single chunk allocation here
        // We're allocating a new segment. If the segment size is
        // within a single chunk, great, otherwise, doesn't matter

        // We need some space reserved for secondary allocations
        TSegment * newSegment = this->AllocSegment(*pageCount);
        if (newSegment != nullptr)
        {
            addr = newSegment->GetAddress();
            *pageCount = newSegment->GetAvailablePageCount();
            *segment = newSegment;
        }
    }
    else
    {
        Assert(*pageCount <= UINT_MAX);
        TPageSegment * pageSegment;

        if (doPageAlign)
        {
            // TODO: Remove this entire codepath since doPageAlign is not being used anymore
            addr = this->AllocPagesPageAligned((uint)*pageCount, &pageSegment);
        }
        else
        {
            addr = this->AllocPages((uint) *pageCount, &pageSegment);
        }

        if (addr != nullptr)
        {
            *segment = pageSegment;
        }
    }
    return addr;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::UpdateMinFreePageCount()
{
    UpdateMinimum(minFreePageCount, freePageCount);
    Assert(debugMinFreePageCount == minFreePageCount);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::ResetMinFreePageCount()
{
    minFreePageCount = freePageCount;
#if DBG
    debugMinFreePageCount = freePageCount;
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::ClearMinFreePageCount()
{
    minFreePageCount = 0;
#if DBG
    debugMinFreePageCount = 0;
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AllocPages(uint pageCount, TPageSegment ** pageSegment)
{
    Assert(this->allocatorType == GetAllocatorType<TVirtualAlloc>());
    return AllocPagesInternal<true /* noPageAligned */>(pageCount, pageSegment);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AllocPagesPageAligned(uint pageCount, TPageSegment ** pageSegment)
{
    return AllocPagesInternal<false /* noPageAligned */>(pageCount, pageSegment);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
template <bool notPageAligned>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AllocPagesInternal(uint pageCount, TPageSegment ** pageSegment)
{
    Assert(!isClosed);
    ASSERT_THREAD();
    Assert(pageCount <= this->maxAllocPageCount);

    this->isUsed = true;

    SuspendIdleDecommit();
    char * allocation = TryAllocFreePages<notPageAligned>(pageCount, pageSegment);
    if (allocation == nullptr)
    {
        allocation = SnailAllocPages<notPageAligned>(pageCount, pageSegment);
    }
    ResumeIdleDecommit();

    PageTracking::ReportAllocation((PageAllocator*)this, allocation, AutoSystemInfo::PageSize * pageCount);

    if (!notPageAligned)
    {
        Assert(TPageSegment::IsAllocationPageAligned(allocation, pageCount));
    }

    return allocation;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::OnAllocFromNewSegment(uint pageCount, __in void* pages, TSegment* newSegment)
{
    DListBase<TPageSegment>* targetSegmentList = (pageCount == maxAllocPageCount) ? &fullSegments : &segments;
    LogAllocPages(pageCount);

    this->FillAllocPages(pages, pageCount);
    this->freePageCount -= pageCount;
#if DBG
    UpdateMinimum(this->debugMinFreePageCount, this->freePageCount);
#endif

    Assert(targetSegmentList != nullptr);
    emptySegments.MoveHeadTo(targetSegmentList);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
template <bool notPageAligned>
char *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::SnailAllocPages(uint pageCount, TPageSegment ** pageSegment)
{
    Assert(!this->HasMultiThreadAccess());

    char * pages = nullptr;
    TPageSegment * newSegment = nullptr;

    if (!emptySegments.Empty())
    {
        newSegment = &emptySegments.Head();

        if (!notPageAligned && !TPageSegment::IsAllocationPageAligned(newSegment->GetAddress(), pageCount))
        {
            newSegment = nullptr;

            // Scan through the empty segments for a segment that can fit this allocation
            FOREACH_DLISTBASE_ENTRY_EDITING(TPageSegment, emptySegment, &this->emptySegments, iter)
            {
                if (TPageSegment::IsAllocationPageAligned(emptySegment.GetAddress(), pageCount))
                {
                    iter.MoveCurrentTo(&this->emptySegments);
                    newSegment = &emptySegment;
                    break;
                }
            }
            NEXT_DLISTBASE_ENTRY_EDITING
        }

        if (newSegment != nullptr)
        {
            pages = newSegment->template AllocPages<notPageAligned>(pageCount);
            if (pages != nullptr)
            {
                OnAllocFromNewSegment(pageCount, pages, newSegment);
                *pageSegment = newSegment;
                return pages;
            }
        }
    }

    pages = TryAllocDecommittedPages<notPageAligned>(pageCount, pageSegment);
    if (pages != nullptr)
    {
        // TryAllocDecommittedPages may give out a mix of free pages and decommitted pages.
        // Free pages are filled with 0xFE in debug build, so we need to zero them
        // out before giving it out. In release build, free page is already zeroed
        // in ReleasePages
        this->FillAllocPages(pages, pageCount);
        return pages;
    }

    Assert(pages == nullptr);
    Assert(maxAllocPageCount >= pageCount);
    if (maxAllocPageCount != pageCount && (maxFreePageCount < maxAllocPageCount - pageCount + freePageCount))
    {
        // If we exceed the number of max free page count, allocate from a new fully decommit block
        TPageSegment * decommitSegment = AllocPageSegment(
            this->decommitSegments, this, false, false, this->enableWriteBarrier);
        if (decommitSegment == nullptr)
        {
            return nullptr;
        }

        pages = decommitSegment->template DoAllocDecommitPages<notPageAligned>(pageCount);
        if (pages != nullptr)
        {
#if DBG_DUMP
            this->decommitPageCount = this->decommitPageCount + decommitSegment->GetDecommitPageCount();
#endif
            this->FillAllocPages(pages, pageCount);

            LogRecommitPages(pageCount);
            LogAllocPages(pageCount);

            *pageSegment = decommitSegment;
        }
        return pages;
    }

    // At this point, we haven't been able to allocate either from the
    // decommitted pages, or from the empty segment list, so we'll
    // try allocating a segment. In a page allocator with a pre-reserved segment,
    // we're not allowed to allocate additional segments so return here.
    // Otherwise, add a new segment and allocate from it

    newSegment = AddPageSegment(emptySegments);
    if (newSegment == nullptr)
    {
        return nullptr;
    }

    pages = newSegment->template AllocPages<notPageAligned>(pageCount);
    if (notPageAligned)
    {
        // REVIEW: Is this true for single-chunk allocations too? Are new segments guaranteed to
        // allow for single-chunk allocations to succeed?
        Assert(pages != nullptr);
    }

    if (pages != nullptr)
    {
        OnAllocFromNewSegment(pageCount, pages, newSegment);
        *pageSegment = newSegment;
    }

    return pages;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
DListBase<TPageSegment> *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::GetSegmentList(TPageSegment * segment)
{
    Assert(!this->HasMultiThreadAccess());

    return
        (segment->IsAllDecommitted()) ? nullptr :
        (segment->IsFull()) ? &fullSegments :
        (segment->ShouldBeInDecommittedList()) ? &decommitSegments :
        (segment->IsEmpty()) ? &emptySegments :
        &segments;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::ReleaseAllocation(PageAllocation * allocation)
{
    SuspendIdleDecommit();
    ReleaseAllocationNoSuspend(allocation);
    ResumeIdleDecommit();
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::ReleaseAllocationNoSuspend(PageAllocation * allocation)
{
    this->Release((char *)allocation, allocation->pageCount, allocation->segment);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::Release(void * address, size_t pageCount, void * segmentParam)
{
    TSegment * segment = (TSegment*)segmentParam;
    Assert(!this->HasMultiThreadAccess());
    Assert(segment->GetAllocator() == this);
    if (pageCount > this->maxAllocPageCount)
    {
        Assert(address == segment->GetAddress());
        Assert(pageCount == segment->GetAvailablePageCount());
        this->ReleaseSegment(segment);
    }
    else
    {
        Assert(pageCount <= UINT_MAX);
        this->ReleasePages(address, static_cast<uint>(pageCount), (TPageSegment *)segment);
    }
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::ReleaseSegment(TSegment * segment)
{
    ASSERT_THREAD();
#ifdef RECYCLER_NO_PAGE_REUSE
    if (disablePageReuse)
    {
        Assert(this->processHandle == GetCurrentProcess());
#pragma prefast(suppress:6250, "Calling 'VirtualFree' without the MEM_RELEASE flag might free memory but not address descriptors (VADs).")
        VirtualFree(segment->GetAddress(), segment->GetPageCount() * AutoSystemInfo::PageSize, MEM_DECOMMIT);
        return;
    }
#endif
    PageTracking::ReportFree((PageAllocator*)this, segment->GetAddress(), AutoSystemInfo::PageSize * segment->GetPageCount());
    LogFreePages(segment->GetPageCount());
    LogFreeSegment(segment);
    largeSegments.RemoveElement(&NoThrowNoMemProtectHeapAllocator::Instance, segment);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AddFreePageCount(uint pageCount)
{
    // minFreePageCount is only updated on release of a page or before decommit
    // so that we don't have to update it on every page allocation.
    UpdateMinFreePageCount();
    this->freePageCount += pageCount;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::ReleasePages(__in void * address, uint pageCount, __in void * segmentParam)
{
    Assert(pageCount <= this->maxAllocPageCount);
    TPageSegment * segment = (TPageSegment*) segmentParam;
    ASSERT_THREAD();
    Assert(!this->HasMultiThreadAccess());

#ifdef RECYCLER_NO_PAGE_REUSE
    if (disablePageReuse)
    {
        Assert(this->processHandle == GetCurrentProcess());
#pragma prefast(suppress:6250, "Calling 'VirtualFree' without the MEM_RELEASE flag might free memory but not address descriptors (VADs).")
        VirtualFree(address, pageCount * AutoSystemInfo::PageSize, MEM_DECOMMIT);
        return;
    }
#endif

    PageTracking::ReportFree((PageAllocator*)this, address, AutoSystemInfo::PageSize * pageCount);

    DListBase<TPageSegment> * fromSegmentList = GetSegmentList(segment);
    Assert(fromSegmentList != nullptr);

    /**
     * The logic here is as follows:
     * - If we have sufficient pages already, such that the newly free pages are going
     *   to cause us to exceed the threshold of free pages we want:
     *     - First check and see if we have empty segments. If we do, just release that
     *       entire segment back to the operating system, and add the current segments
     *       free pages to our free page pool
     *     - Otherwise, if there are no empty segments (i.e our memory is fragmented),
     *       decommit the pages that are being released so that they don't count towards
     *       our working set
     * - If we don't have enough pages:
     *    - If we're in the free page queuing mode where we have a "pages to zero out" queue
     *      put it in that queue and we're done
     *    - Otherwise, zero it out, and add it to the free page pool
     *  Now that we've either decommitted or freed the pages in the segment,
     *  move the segment to the right segment list
     */
    if (this->freePageCount + pageCount > maxFreePageCount)
    {
        // Release a whole segment if possible to reduce the number of VirtualFree and fragmentation
        if (!ZeroPages() && !emptySegments.Empty())
        {
            Assert(emptySegments.Head().GetDecommitPageCount() == 0);
            LogFreeSegment(&emptySegments.Head());
            emptySegments.RemoveHead(&NoThrowNoMemProtectHeapAllocator::Instance);
            this->freePageCount -= maxAllocPageCount;

#if DBG
            UpdateMinimum(this->debugMinFreePageCount, this->freePageCount);
            MemSetLocal(address, DbgMemFill, AutoSystemInfo::PageSize * pageCount);
#endif
            segment->ReleasePages(address, pageCount);
            LogFreePages(pageCount);
            this->AddFreePageCount(pageCount);

        }
        else
        {
            segment->template DecommitPages<false>(address, pageCount);
            LogFreePages(pageCount);
            LogDecommitPages(pageCount);
#if DBG_DUMP
            this->decommitPageCount += pageCount;
#endif
        }
    }
    else
    {
#if ENABLE_BACKGROUND_PAGE_ZEROING
        if (CONFIG_FLAG(EnableBGFreeZero))
        {
            if (QueueZeroPages())
            {
                Assert(HasZeroPageQueue());
                AddPageToZeroQueue(address, pageCount, segment);
                return;
            }
        }
#endif

        this->FillFreePages((char *)address, pageCount);
        segment->ReleasePages(address, pageCount);
        LogFreePages(pageCount);
        this->AddFreePageCount(pageCount);
    }

    TransferSegment(segment, fromSegmentList);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::MemSetLocal(_In_ void *dst, int val, size_t sizeInBytes)
{
    memset(dst, val, sizeInBytes);
}

#if ENABLE_OOP_NATIVE_CODEGEN
template<>
void
PageAllocatorBase<SectionAllocWrapper>::MemSetLocal(_In_ void *dst, int val, size_t sizeInBytes)
{
    LPVOID localAddr = this->GetVirtualAllocator()->AllocLocal(dst, sizeInBytes);
    if (localAddr == nullptr)
    {
        MemoryOperationLastError::RecordError(JSERR_FatalMemoryExhaustion);
    }
    else
    {
        memset(localAddr, val, sizeInBytes);
        this->GetVirtualAllocator()->FreeLocal(localAddr);
    }
}

template<>
void
PageAllocatorBase<PreReservedSectionAllocWrapper>::MemSetLocal(_In_ void *dst, int val, size_t sizeInBytes)
{
    LPVOID localAddr = this->GetVirtualAllocator()->AllocLocal(dst, sizeInBytes);
    if (localAddr == nullptr)
    {
        MemoryOperationLastError::RecordError(JSERR_FatalMemoryExhaustion);
    }
    else
    {
        memset(localAddr, val, sizeInBytes);
        this->GetVirtualAllocator()->FreeLocal(localAddr);
    }
}
#endif

#if ENABLE_BACKGROUND_PAGE_ZEROING
template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
typename PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::FreePageEntry *
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::PopPendingZeroPage()
{
    Assert(HasZeroPageQueue());
    return ((ZeroPageQueue *) backgroundPageQueue)->PopZeroPageEntry();
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AddPageToZeroQueue(__in void * address, uint pageCount, __in TPageSegment * pageSegment)
{
    Assert(HasZeroPageQueue());
    Assert(pageSegment->GetAllocator() == this);
    FreePageEntry * entry = (FreePageEntry *)address;
    entry->segment = pageSegment;
    entry->pageCount = pageCount;
    ((ZeroPageQueue *)backgroundPageQueue)->PushZeroPageEntry(entry);
    this->hasZeroQueuedPages = true;
}
#endif

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::TransferSegment(TPageSegment * segment, DListBase<TPageSegment> * fromSegmentList)
{
    DListBase<TPageSegment> * toSegmentList = GetSegmentList(segment);

    if (fromSegmentList != toSegmentList)
    {
        if (toSegmentList)
        {
            AssertMsg(segment->GetSecondaryAllocator() == nullptr  || fromSegmentList != &fullSegments || segment->GetSecondaryAllocator()->CanAllocate(),
                "If it's being moved from a full segment it should be able to do secondary allocations");
            fromSegmentList->MoveElementTo(segment, toSegmentList);
        }
        else
        {
            LogFreePartiallyDecommittedPageSegment(segment);
            fromSegmentList->RemoveElement(&NoThrowNoMemProtectHeapAllocator::Instance, segment);
#if DBG_DUMP
            this->decommitPageCount -= maxAllocPageCount;
#endif
        }
    }
}

#if ENABLE_BACKGROUND_PAGE_ZEROING
template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::BackgroundZeroQueuedPages()
{
    Assert(HasZeroPageQueue());
    AutoCriticalSection autocs(&backgroundPageQueue->backgroundPageQueueCriticalSection);
    ZeroQueuedPages();
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::ZeroQueuedPages()
{
    Assert(HasZeroPageQueue());

    while (true)
    {
        FreePageEntry * freePageEntry = PopPendingZeroPage();
        if (freePageEntry == nullptr)
        {
            break;
        }
        TPageSegment * segment = freePageEntry->segment;
        uint pageCount = freePageEntry->pageCount;

        //
        // Do memset via non-temporal store to avoid evicting existing processor cache.
        // This helps low-end machines with limited cache size.
        //
        Assert(this->processHandle == GetCurrentProcess());
#if defined(_M_IX86) || defined(_M_X64)
        if (CONFIG_FLAG(ZeroMemoryWithNonTemporalStore))
        {
            js_memset_zero_nontemporal(freePageEntry, AutoSystemInfo::PageSize * pageCount);
        }
        else
#endif
        {
            memset(freePageEntry, 0, pageCount * AutoSystemInfo::PageSize);
        }

        QueuePages(freePageEntry, pageCount, segment);
    }
    this->hasZeroQueuedPages = false;
}
#endif

#if ENABLE_BACKGROUND_PAGE_FREEING
template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::BackgroundReleasePages(void * address, uint pageCount, TPageSegment * segment)
{
    FillFreePages(address, pageCount);
    QueuePages(address, pageCount, segment);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::QueuePages(void * address, uint pageCount, TPageSegment * segment)
{
    Assert(backgroundPageQueue);
    FreePageEntry * freePageEntry = (FreePageEntry *)address;
    freePageEntry->segment = segment;
    freePageEntry->pageCount = pageCount;
    backgroundPageQueue->PushFreePageEntry(freePageEntry);
}
#endif

#if ENABLE_BACKGROUND_PAGE_FREEING
template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::FlushBackgroundPages()
{
    Assert(!this->HasMultiThreadAccess());
    Assert(backgroundPageQueue);

    // We can have additional pages queued up to be zeroed out here
    // and that's okay since they'll eventually be zeroed out before being flushed

    uint newFreePages = 0;

    while (true)
    {
        FreePageEntry * freePageEntry = backgroundPageQueue->PopFreePageEntry();
        if (freePageEntry == nullptr)
        {
            break;
        }
        TPageSegment * segment = freePageEntry->segment;
        uint pageCount = freePageEntry->pageCount;

        DListBase<TPageSegment> * fromSegmentList = GetSegmentList(segment);
        Assert(fromSegmentList != nullptr);

        Assert(this->processHandle == GetCurrentProcess());
        memset(freePageEntry, 0, sizeof(FreePageEntry));

        segment->ReleasePages(freePageEntry, pageCount);
        newFreePages += pageCount;

        TransferSegment(segment, fromSegmentList);
    }

    LogFreePages(newFreePages);

    PAGE_ALLOC_VERBOSE_TRACE(_u("New free pages: %d\n"), newFreePages);
    this->AddFreePageCount(newFreePages);
}
#endif

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::SuspendIdleDecommit()
{
#ifdef IDLE_DECOMMIT_ENABLED
    if (this->idleDecommitEnterCount != 0)
    {
        return;
    }
    Assert(this->IsIdleDecommitPageAllocator());
    ((IdleDecommitPageAllocator *)this)->cs.Enter();
    PAGE_ALLOC_VERBOSE_TRACE_0(_u("SuspendIdleDecommit"));
#endif
}

template<>
void
PageAllocatorBase<SectionAllocWrapper>::SuspendIdleDecommit()
{
    Assert(!this->IsIdleDecommitPageAllocator());
}
template<>
void
PageAllocatorBase<PreReservedSectionAllocWrapper>::SuspendIdleDecommit()
{
    Assert(!this->IsIdleDecommitPageAllocator());
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::ResumeIdleDecommit()
{
#ifdef IDLE_DECOMMIT_ENABLED
    if (this->idleDecommitEnterCount != 0)
    {
        return;
    }
    Assert(this->IsIdleDecommitPageAllocator());
    PAGE_ALLOC_VERBOSE_TRACE(_u("ResumeIdleDecommit"));
    ((IdleDecommitPageAllocator *)this)->cs.Leave();
#endif
}

template<>
void
PageAllocatorBase<SectionAllocWrapper>::ResumeIdleDecommit()
{
    Assert(!this->IsIdleDecommitPageAllocator());
}
template<>
void
PageAllocatorBase<PreReservedSectionAllocWrapper>::ResumeIdleDecommit()
{
    Assert(!this->IsIdleDecommitPageAllocator());
}


template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::DecommitNow(bool all)
{
    Assert(!this->HasMultiThreadAccess());

#if DBG_DUMP
    size_t deleteCount = 0;
#endif
#if ENABLE_BACKGROUND_PAGE_ZEROING
    if (CONFIG_FLAG(EnableBGFreeZero))
    {
        // First, drain the zero page queue.
        // This will cause the free page count to be accurate
        if (HasZeroPageQueue())
        {
            int numZeroPagesFreed = 0;

            // There might be queued zero pages.  Drain them first

            while (true)
            {
                FreePageEntry * freePageEntry = PopPendingZeroPage();
                if (freePageEntry == nullptr)
                {
                    break;
                }
                PAGE_ALLOC_TRACE_AND_STATS_0(_u("Freeing page from zero queue"));
                TPageSegment * segment = freePageEntry->segment;
                uint pageCount = freePageEntry->pageCount;

                numZeroPagesFreed += pageCount;

                DListBase<TPageSegment> * fromSegmentList = GetSegmentList(segment);
                Assert(fromSegmentList != nullptr);

                // Check for all here, since the actual free page count can't be determined
                // until we've flushed the zeroed page queue
                if (all)
                {
                    // Decommit them immediately if we are decommitting all pages.
                    segment->template DecommitPages<false>(freePageEntry, pageCount);
                    LogFreePages(pageCount);
                    LogDecommitPages(pageCount);

                    if (segment->IsAllDecommitted())
                    {
                        LogFreePartiallyDecommittedPageSegment(segment);
                        fromSegmentList->RemoveElement(&NoThrowNoMemProtectHeapAllocator::Instance, segment);
#if DBG_DUMP
                        deleteCount += maxAllocPageCount;
#endif
                        continue;
                    }
                }
                else
                {
                    // Zero them and release them in case we don't decommit them.
                    Assert(this->processHandle == GetCurrentProcess());
                    memset(freePageEntry, 0, pageCount * AutoSystemInfo::PageSize);
                    segment->ReleasePages(freePageEntry, pageCount);
                    LogFreePages(pageCount);
                }

                TransferSegment(segment, fromSegmentList);
            }

            // Take the lock to make sure the recycler thread has finished zeroing out the pages after
            // we drained the queue
            {
                AutoCriticalSection autoCS(&backgroundPageQueue->backgroundPageQueueCriticalSection);
                this->hasZeroQueuedPages = false;
                Assert(!this->HasZeroQueuedPages());
            }

            FlushBackgroundPages();
        }
    }
#endif

    if (this->freePageCount == 0)
    {
        Assert(debugMinFreePageCount == 0);
        return;
    }

    PAGE_ALLOC_TRACE_AND_STATS_0(_u("Decommit now"));

    // minFreePageCount is not updated on every page allocate,
    // so we have to do a final update here.
    UpdateMinFreePageCount();

    size_t newFreePageCount;

    if (all)
    {
        newFreePageCount = this->GetFreePageLimit();

        PAGE_ALLOC_TRACE_AND_STATS_0(_u("Full decommit"));
    }
    else
    {
        // Decommit half the min free page count since last partial decommit
        Assert(this->minFreePageCount <= this->freePageCount);
        newFreePageCount = this->freePageCount - (this->minFreePageCount / 2);

        // Ensure we don't decommit down to fewer than our partial decommit minimum
        newFreePageCount = max(newFreePageCount, static_cast<size_t>(MinPartialDecommitFreePageCount));

        PAGE_ALLOC_TRACE_AND_STATS_0(_u("Partial decommit"));
    }

    if (newFreePageCount >= this->freePageCount)
    {
        PAGE_ALLOC_TRACE_AND_STATS_0(_u("No pages to decommit"));
        return;
    }

    size_t pageToDecommit = this->freePageCount - newFreePageCount;

    PAGE_ALLOC_TRACE_AND_STATS(_u("Decommit page count = %d"), pageToDecommit);
    PAGE_ALLOC_TRACE_AND_STATS(_u("Free page count = %d"), this->freePageCount);
    PAGE_ALLOC_TRACE_AND_STATS(_u("New free page count = %d"), newFreePageCount);

#if DBG_DUMP
    size_t decommitCount = 0;
#endif

    // decommit from page that already has other decommitted page already
    {
        typename DListBase<TPageSegment>::EditingIterator i(&decommitSegments);

        while (pageToDecommit > 0  && i.Next())
        {
            size_t pageDecommitted = i.Data().DecommitFreePages(pageToDecommit);
            LogDecommitPages(pageDecommitted);
#if DBG_DUMP
            decommitCount += pageDecommitted;
#endif
            if (i.Data().GetDecommitPageCount() == maxAllocPageCount)
            {
                LogFreePartiallyDecommittedPageSegment(&i.Data());
                i.RemoveCurrent(&NoThrowNoMemProtectHeapAllocator::Instance);
#if DBG_DUMP
                deleteCount += maxAllocPageCount;
#endif
            }
            pageToDecommit -= pageDecommitted;
        }
    }

    // decommit pages that are empty

    while (pageToDecommit > 0 && !emptySegments.Empty())
    {
        if (pageToDecommit >= maxAllocPageCount)
        {
            Assert(emptySegments.Head().GetDecommitPageCount() == 0);
            LogFreeSegment(&emptySegments.Head());
            emptySegments.RemoveHead(&NoThrowNoMemProtectHeapAllocator::Instance);

            pageToDecommit -= maxAllocPageCount;
#if DBG_DUMP
            decommitCount += maxAllocPageCount;
            deleteCount += maxAllocPageCount;
#endif
        }
        else
        {
            size_t pageDecommitted = emptySegments.Head().DecommitFreePages(pageToDecommit);
            LogDecommitPages(pageDecommitted);
#if DBG_DUMP
            decommitCount += pageDecommitted;
#endif
            Assert(pageDecommitted == pageToDecommit);
            emptySegments.MoveHeadTo(&decommitSegments);
            pageToDecommit = 0;
        }
    }

    {
        typename DListBase<TPageSegment>::EditingIterator i(&segments);

        while (pageToDecommit > 0  && i.Next())
        {
            size_t pageDecommitted = i.Data().DecommitFreePages(pageToDecommit);
            LogDecommitPages(pageDecommitted);
#if DBG_DUMP
            decommitCount += pageDecommitted;
#endif
            Assert(i.Data().GetDecommitPageCount() != 0);
            Assert(i.Data().GetDecommitPageCount() <= maxAllocPageCount);
            i.MoveCurrentTo(&decommitSegments);
            pageToDecommit -= pageDecommitted;

        }
    }


    Assert(pageToDecommit == 0);

#if DBG_DUMP
    Assert(this->freePageCount == newFreePageCount + decommitCount);
#endif

    this->freePageCount = newFreePageCount;

#if DBG
    UpdateMinimum(this->debugMinFreePageCount, this->freePageCount);
    Check();
#endif
#if DBG_DUMP
    this->decommitPageCount += (decommitCount - deleteCount);
    if (CUSTOM_PHASE_TRACE1(this->pageAllocatorFlagTable, Js::PageAllocatorPhase))
    {
        if (CUSTOM_PHASE_STATS1(this->pageAllocatorFlagTable, Js::PageAllocatorPhase))
        {
            Output::Print(_u(" After decommit now:\n"));
            this->DumpStats();
        }
        Output::Flush();
    }
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AddReservedBytes(size_t bytes)
{
    reservedBytes += bytes;
#ifdef PERF_COUNTERS
    GetReservedSizeCounter() += bytes;
    GetTotalReservedSizeCounter() += bytes;
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::SubReservedBytes(size_t bytes)
{
    reservedBytes -= bytes;
#ifdef PERF_COUNTERS
    GetReservedSizeCounter() -= bytes;
    GetTotalReservedSizeCounter() -= bytes;
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AddCommittedBytes(size_t bytes)
{
    committedBytes += bytes;
#ifdef PERF_COUNTERS
    GetCommittedSizeCounter() += bytes;
    GetTotalCommittedSizeCounter() += bytes;
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::SubCommittedBytes(size_t bytes)
{
    committedBytes -= bytes;
#ifdef PERF_COUNTERS
    GetCommittedSizeCounter() -= bytes;
    GetTotalCommittedSizeCounter() -= bytes;
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AddUsedBytes(size_t bytes)
{
    usedBytes += bytes;
#if defined(_M_X64_OR_ARM64)
    size_t lastTotalUsedBytes = ::InterlockedExchangeAdd64((volatile LONG64 *)&totalUsedBytes, bytes);
#else
    DWORD lastTotalUsedBytes = ::InterlockedExchangeAdd(&totalUsedBytes, bytes);
#endif

    if (totalUsedBytes > maxUsedBytes)
    {
        maxUsedBytes = totalUsedBytes;
    }

    // ETW events from different threads may be reported out of order, producing an
    // incorrect representation of current used bytes in the process. We've determined that this is an
    // acceptable issue, which will be mitigated at the level of the application consuming the event.
    JS_ETW(EventWriteJSCRIPT_PAGE_ALLOCATOR_USED_SIZE(lastTotalUsedBytes + bytes));
#ifndef ENABLE_JS_ETW
    Unused(lastTotalUsedBytes);
#endif

#ifdef PERF_COUNTERS
    GetUsedSizeCounter() += bytes;
    GetTotalUsedSizeCounter() += bytes;
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::SubUsedBytes(size_t bytes)
{
    Assert(bytes <= usedBytes);
    Assert(bytes <= totalUsedBytes);

    usedBytes -= bytes;

#if defined(_M_X64_OR_ARM64)
    size_t lastTotalUsedBytes = ::InterlockedExchangeAdd64((volatile LONG64 *)&totalUsedBytes, -(LONG64)bytes);
#else
    DWORD lastTotalUsedBytes = ::InterlockedExchangeSubtract(&totalUsedBytes, bytes);
#endif

    // ETW events from different threads may be reported out of order, producing an
    // incorrect representation of current used bytes in the process. We've determined that this is an
    // acceptable issue, which will be mitigated at the level of the application consuming the event.
    JS_ETW(EventWriteJSCRIPT_PAGE_ALLOCATOR_USED_SIZE(lastTotalUsedBytes - bytes));
#ifndef ENABLE_JS_ETW
    Unused(lastTotalUsedBytes);
#endif

#ifdef PERF_COUNTERS
    GetUsedSizeCounter() -= bytes;
    GetTotalUsedSizeCounter() -= bytes;
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::AddNumberOfSegments(size_t segmentCount)
{
    numberOfSegments += segmentCount;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::SubNumberOfSegments(size_t segmentCount)
{
    numberOfSegments -= segmentCount;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::IntegrateSegments(DListBase<TPageSegment>& segmentList, uint segmentCount, size_t pageCount)
{
#if DBG
    size_t debugPageCount = 0;
    uint debugSegmentCount = 0;
    typename DListBase<TPageSegment>::Iterator i(&segmentList);
    while (i.Next())
    {
        Assert(i.Data().GetAllocator() == this);
        debugSegmentCount++;
        debugPageCount += i.Data().GetPageCount();
    }
    Assert(debugSegmentCount == segmentCount);
    Assert(debugPageCount == pageCount);
#endif
    LogAllocSegment(segmentCount, pageCount);
    LogAllocPages(pageCount);

    this->SuspendIdleDecommit();
    segmentList.MoveTo(&this->fullSegments);
    this->ResumeIdleDecommit();
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogAllocSegment(TSegment * segment)
{
    LogAllocSegment(1, segment->GetPageCount());
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogAllocSegment(uint segmentCount, size_t pageCount)
{
    size_t bytes = pageCount * AutoSystemInfo::PageSize;
    AddReservedBytes(bytes);
    AddCommittedBytes(bytes);
    AddNumberOfSegments(segmentCount);
#ifdef PROFILE_MEM
    if (this->memoryData)
    {
        this->memoryData->allocSegmentCount += segmentCount;
        this->memoryData->allocSegmentBytes += pageCount * AutoSystemInfo::PageSize;

        this->memoryData->currentCommittedPageCount += pageCount;
        this->memoryData->peakCommittedPageCount = max(this->memoryData->peakCommittedPageCount, this->memoryData->currentCommittedPageCount);
    }
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogFreeSegment(TSegment * segment)
{
    size_t bytes = segment->GetPageCount() * AutoSystemInfo::PageSize;
    SubCommittedBytes(bytes);
    SubReservedBytes(bytes);
    SubNumberOfSegments(1);
#ifdef PROFILE_MEM
    if (this->memoryData)
    {
        this->memoryData->releaseSegmentCount++;
        this->memoryData->releaseSegmentBytes += segment->GetPageCount() * AutoSystemInfo::PageSize;
        this->memoryData->currentCommittedPageCount -= segment->GetPageCount();
    }
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogFreeDecommittedSegment(TSegment * segment)
{
    SubReservedBytes(segment->GetPageCount() * AutoSystemInfo::PageSize);
    SubNumberOfSegments(1);
#ifdef PROFILE_MEM
    if (this->memoryData)
    {
        this->memoryData->releaseSegmentCount++;
        this->memoryData->releaseSegmentBytes += segment->GetPageCount() * AutoSystemInfo::PageSize;
    }
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogFreePages(size_t pageCount)
{
    SubUsedBytes(pageCount * AutoSystemInfo::PageSize);
#ifdef PROFILE_MEM
    if (this->memoryData)
    {
        this->memoryData->releasePageCount += pageCount;
    }
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogFreePartiallyDecommittedPageSegment(TPageSegment * pageSegment)
{
    AddCommittedBytes(pageSegment->GetDecommitPageCount() * AutoSystemInfo::PageSize);
#ifdef PROFILE_MEM
    if (this->memoryData)
    {
        this->memoryData->currentCommittedPageCount += pageSegment->GetDecommitPageCount();
    }
#endif
    LogFreeSegment(pageSegment);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogAllocPages(size_t pageCount)
{
    AddUsedBytes(pageCount * AutoSystemInfo::PageSize);
#ifdef PROFILE_MEM
    if (this->memoryData)
    {
        this->memoryData->allocPageCount += pageCount;
    }
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogRecommitPages(size_t pageCount)
{
#ifdef PROFILE_MEM
    if (this->memoryData)
    {
        this->memoryData->recommitPageCount += pageCount;
    }
#endif
    LogCommitPages(pageCount);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogCommitPages(size_t pageCount)
{
    AddCommittedBytes(pageCount * AutoSystemInfo::PageSize);
#ifdef PROFILE_MEM
    if (this->memoryData)
    {
        this->memoryData->currentCommittedPageCount += pageCount;
        this->memoryData->peakCommittedPageCount = max(this->memoryData->peakCommittedPageCount, this->memoryData->currentCommittedPageCount);
    }
#endif
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::LogDecommitPages(size_t pageCount)
{
    SubCommittedBytes(pageCount * AutoSystemInfo::PageSize);
#ifdef PROFILE_MEM
    if (this->memoryData)
    {
        this->memoryData->decommitPageCount += pageCount;
        this->memoryData->currentCommittedPageCount -= pageCount;
    }
#endif
}

#if DBG_DUMP
template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::DumpStats() const
{
    Output::Print(_u("  Full/Partial/Empty/Decommit/Large Segments: %4d %4d %4d %4d %4d\n"),
        fullSegments.Count(), segments.Count(), emptySegments.Count(), decommitSegments.Count(), largeSegments.Count());

    Output::Print(_u("  Free/Decommit/Min Free Pages              : %4d %4d %4d\n"),
        this->freePageCount, this->decommitPageCount, this->minFreePageCount);
}
#endif

#if DBG
template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
void
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::Check()
{
#if ENABLE_BACKGROUND_PAGE_ZEROING
    if (CONFIG_FLAG(EnableBGFreeZero))
    {
        Assert(!this->HasZeroQueuedPages());
    }
#endif
    size_t currentFreePageCount = 0;

    typename DListBase<TPageSegment>::Iterator segmentsIterator(&segments);
    while (segmentsIterator.Next())
    {
        currentFreePageCount += segmentsIterator.Data().GetFreePageCount();
    }

    typename DListBase<TPageSegment>::Iterator fullSegmentsIterator(&fullSegments);
    while (fullSegmentsIterator.Next())
    {
        currentFreePageCount += fullSegmentsIterator.Data().GetFreePageCount();
    }

    typename DListBase<TPageSegment>::Iterator emptySegmentsIterator(&emptySegments);
    while (emptySegmentsIterator.Next())
    {
        currentFreePageCount += emptySegmentsIterator.Data().GetFreePageCount();
    }

    typename DListBase<TPageSegment>::Iterator decommitSegmentsIterator(&decommitSegments);
    while (decommitSegmentsIterator.Next())
    {
        currentFreePageCount += decommitSegmentsIterator.Data().GetFreePageCount();
    }

    Assert(freePageCount == currentFreePageCount);
}
#endif

template<typename T>
HeapPageAllocator<T>::HeapPageAllocator(AllocationPolicyManager * policyManager, bool allocXdata, bool excludeGuardPages, T * virtualAllocator, HANDLE processHandle) :
    PageAllocatorBase<T>(policyManager,
        Js::Configuration::Global.flags,
        PageAllocatorType_CustomHeap,
        /*maxFreePageCount*/ 0,
        /*zeroPages*/ false,
#if ENABLE_BACKGROUND_PAGE_FREEING || ENABLE_BACKGROUND_PAGE_ZEROING
        /*zeroPageQueue*/ nullptr,
#endif
        /*maxAllocPageCount*/ allocXdata ? (Base::DefaultMaxAllocPageCount - XDATA_RESERVE_PAGE_COUNT) : Base::DefaultMaxAllocPageCount,
        /*secondaryAllocPageCount=*/ allocXdata ? XDATA_RESERVE_PAGE_COUNT : 0,
        /*stopAllocationOnOutOfMemory*/ false,
        excludeGuardPages,
        processHandle),
    allocXdata(allocXdata)
{
    this->InitVirtualAllocator(virtualAllocator);
}

template<typename T>
void
HeapPageAllocator<T>::ReleaseDecommitted(void * address, size_t pageCount, __in void *  segmentParam)
{
    SegmentBase<T> * segment = (SegmentBase<T>*) segmentParam;
    if (pageCount > this->maxAllocPageCount)
    {
        Assert(address == segment->GetAddress());
        Assert(pageCount == segment->GetAvailablePageCount());
        this->ReleaseDecommittedSegment(segment);
    }
    else
    {
        Assert(pageCount <= UINT_MAX);
        this->TrackDecommittedPages(address, (uint)pageCount, (PageSegment *)segment);
    }
}

template<typename T>
void
HeapPageAllocator<T>::ReleaseDecommittedSegment(__in SegmentBase<T>* segment)
{
    ASSERT_THREAD();

    this->LogFreeDecommittedSegment(segment);
    this->largeSegments.RemoveElement(&NoThrowNoMemProtectHeapAllocator::Instance, segment);
}

// decommit the page but don't release it
template<typename T>
void
HeapPageAllocator<T>::DecommitPages(__in char* address, size_t pageCount /* = 1 */)
{
    Assert(pageCount <= MAXUINT32);
#pragma prefast(suppress:__WARNING_WIN32UNRELEASEDVADS, "The remainder of the clean-up is done later.");
    this->GetVirtualAllocator()->Free(address, pageCount * AutoSystemInfo::PageSize, MEM_DECOMMIT);
    this->LogFreePages(pageCount);
    this->LogDecommitPages(pageCount);
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
template <typename T>
void PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::ReleaseSegmentList(DListBase<T> * segmentList)
{
    segmentList->Clear(&NoThrowNoMemProtectHeapAllocator::Instance);
}

template<typename T>
BOOL
HeapPageAllocator<T>::ProtectPages(__in char* address, size_t pageCount, __in void* segmentParam, DWORD dwVirtualProtectFlags, DWORD desiredOldProtectFlag)
{
    SegmentBase<T> * segment = (SegmentBase<T>*)segmentParam;
#if DBG
    Assert(address >= segment->GetAddress());
    Assert(((uint)(((char *)address) - segment->GetAddress()) <= (segment->GetPageCount() - pageCount) * AutoSystemInfo::PageSize));

    if (this->IsPageSegment(segment))
    {
        PageSegmentBase<T> * pageSegment = static_cast<PageSegmentBase<T>*>(segment);
        AssertMsg(pageCount <= MAXUINT32, "PageSegment should always be smaller than 4G pages");
        Assert(!pageSegment->IsFreeOrDecommitted(address, static_cast<uint>(pageCount)));
    }
#endif

    // check address alignment, and that the address is in correct range
    if (((uintptr_t)address & (AutoSystemInfo::PageSize - 1)) != 0
        || address < segment->GetAddress()
        || ((uint)(((char *)address) - segment->GetAddress()) > (segment->GetPageCount() - pageCount) * AutoSystemInfo::PageSize))
    {
        // OOPJIT TODO: don't bring down the whole JIT process
        CustomHeap_BadPageState_fatal_error((ULONG_PTR)this);
        return FALSE;
    }


    // OOP JIT page protection is immutable
    if (this->processHandle != GetCurrentProcess())
    {
        return TRUE;
    }
    MEMORY_BASIC_INFORMATION memBasicInfo;

    // check old protection on all pages about to change, ensure the fidelity
    size_t bytes = VirtualQuery(address, &memBasicInfo, sizeof(memBasicInfo));
    if (bytes == 0)
    {
        MemoryOperationLastError::RecordLastError();
    }
    if (bytes == 0
        || memBasicInfo.RegionSize < pageCount * AutoSystemInfo::PageSize
        || desiredOldProtectFlag != memBasicInfo.Protect)
    {
        CustomHeap_BadPageState_fatal_error((ULONG_PTR)this);
        return FALSE;
    }

    /*Verify if we always pass the PAGE_TARGETS_NO_UPDATE flag, if the protect flag is EXECUTE*/
#if defined(_CONTROL_FLOW_GUARD)
    if (AutoSystemInfo::Data.IsCFGEnabled() &&
        (dwVirtualProtectFlags & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) &&
        ((dwVirtualProtectFlags & PAGE_TARGETS_NO_UPDATE) == 0))
    {
        CustomHeap_BadPageState_fatal_error((ULONG_PTR)this);
        return FALSE;
    }
#endif

#if defined(ENABLE_JIT_CLAMP)
    bool makeExecutable = (dwVirtualProtectFlags & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) ? true : false;

    AutoEnableDynamicCodeGen enableCodeGen(makeExecutable);
#endif

#if DBG_DUMP || defined(RECYCLER_TRACE)
    if (this->pageAllocatorFlagTable.IsEnabled(Js::TraceProtectPagesFlag))
    {
        Output::Print(_u("VirtualProtect(0x%p, %d, %d, %d)\n"), address, pageCount, pageCount * AutoSystemInfo::PageSize, dwVirtualProtectFlags);
    }
#endif

    DWORD oldProtect; // this is only for first page
    BOOL retVal = VirtualProtect(address, pageCount * AutoSystemInfo::PageSize, dwVirtualProtectFlags, &oldProtect);
    if (retVal == FALSE)
    {
        CustomHeap_BadPageState_fatal_error((ULONG_PTR)this);
    }
    else
    {
        Assert(oldProtect == desiredOldProtectFlag);
    }

    return retVal;
}

template<typename T>
void
HeapPageAllocator<T>::TrackDecommittedPages(void * address, uint pageCount, __in void* segmentParam)
{
    PageSegmentBase<T> * segment = (PageSegmentBase<T>*)segmentParam;
    ASSERT_THREAD();
    Assert(!this->HasMultiThreadAccess());
    Assert(pageCount <= this->maxAllocPageCount);

    DListBase<PageSegmentBase<T>> * fromSegmentList = this->GetSegmentList(segment);

    // Update the state of the segment with the decommitted pages
    segment->template DecommitPages<true>(address, pageCount);
    // Move the segment to its appropriate list
    this->TransferSegment(segment, fromSegmentList);
}

template<typename T>
bool HeapPageAllocator<T>::AllocSecondary(void* segmentParam, ULONG_PTR functionStart, DWORD functionSize, ushort pdataCount, ushort xdataSize, SecondaryAllocation* allocation)
{
    SegmentBase<T> * segment = (SegmentBase<T> *)segmentParam;
    Assert(segment->GetSecondaryAllocator());

    bool success;
    if (this->IsPageSegment(segment))
    {
        PageSegmentBase<T>* pageSegment = static_cast<PageSegmentBase<T>*>(segment);

        // We should get the segment list BEFORE xdata allocation happens.
        DListBase<PageSegmentBase<T>> * fromSegmentList = this->GetSegmentList(pageSegment);

        success = segment->GetSecondaryAllocator()->Alloc(functionStart, functionSize, pdataCount, xdataSize, allocation);

        // If no more XDATA allocations can take place.
        if (success && !pageSegment->CanAllocSecondary() && fromSegmentList != &this->fullSegments)
        {
            AssertMsg(this->GetSegmentList(pageSegment) == &this->fullSegments, "This segment should now be in the full list if it can't allocate secondary");

            OUTPUT_TRACE(Js::EmitterPhase, _u("XDATA Wasted pages:%u\n"), pageSegment->GetFreePageCount());
            this->freePageCount -= pageSegment->GetFreePageCount();
            fromSegmentList->MoveElementTo(pageSegment, &this->fullSegments);
#if DBG
            UpdateMinimum(this->debugMinFreePageCount, this->freePageCount);
#endif
        }
    }
    else
    {
        // A large segment should always be able to do secondary allocations
        Assert(segment->CanAllocSecondary());
        success = segment->GetSecondaryAllocator()->Alloc(functionStart, functionSize, pdataCount, xdataSize, allocation);
    }

#ifdef _M_X64
    // In ARM it's OK to have xdata size be 0
    AssertMsg(allocation->address != nullptr, "All segments that cannot allocate xdata should have been already moved to full segments list");
#endif
    return success;
}

template<typename T>
bool HeapPageAllocator<T>::ReleaseSecondary(const SecondaryAllocation& allocation, void* segmentParam)
{
    SegmentBase<T> * segment = (SegmentBase<T>*)segmentParam;
    Assert(allocation.address != nullptr);
    Assert(segment->GetSecondaryAllocator());

    if (this->IsPageSegment(segment))
    {
        PageSegmentBase<T>* pageSegment = static_cast<PageSegmentBase<T>*>(segment);
        auto fromList = this->GetSegmentList(pageSegment);

        pageSegment->GetSecondaryAllocator()->Release(allocation);

        auto toList = this->GetSegmentList(pageSegment);

        if (fromList != toList)
        {
            OUTPUT_TRACE(Js::EmitterPhase, _u("XDATA reclaimed pages:%u\n"), pageSegment->GetFreePageCount());
            fromList->MoveElementTo(pageSegment, toList);

            AssertMsg(fromList == &this->fullSegments, "Releasing a secondary allocator should make a state change only if the segment was originally in the full list");
            AssertMsg(pageSegment->CanAllocSecondary(), "It should be allocate secondary now");
            this->AddFreePageCount(pageSegment->GetFreePageCount());
            return true;
        }
    }
    else
    {
        Assert(segment->CanAllocSecondary());
        segment->GetSecondaryAllocator()->Release(allocation);
    }
    return false;
}

template<typename T>
bool
HeapPageAllocator<T>::IsAddressFromAllocator(__in void* address)
{
    typename DListBase<PageSegmentBase<T>>::Iterator segmentsIterator(&this->segments);
    while (segmentsIterator.Next())
    {
        if (this->IsAddressInSegment(address, segmentsIterator.Data()))
        {
            return true;
        }
    }

    typename DListBase<PageSegmentBase<T>>::Iterator fullSegmentsIterator(&this->fullSegments);
    while (fullSegmentsIterator.Next())
    {
        if (this->IsAddressInSegment(address, fullSegmentsIterator.Data()))
        {
            return true;
        }
    }

    typename DListBase<SegmentBase<T>>::Iterator largeSegmentsIterator(&this->largeSegments);
    while (largeSegmentsIterator.Next())
    {
        if (this->IsAddressInSegment(address, largeSegmentsIterator.Data()))
        {
            return true;
        }
    }

    typename DListBase<PageSegmentBase<T>>::Iterator decommitSegmentsIterator(&this->decommitSegments);
    while (decommitSegmentsIterator.Next())
    {
        if (this->IsAddressInSegment(address, decommitSegmentsIterator.Data()))
        {
            return true;
        }
    }

    return false;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
bool
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::IsAddressInSegment(__in void* address, const TPageSegment& segment)
{
    bool inSegment = this->IsAddressInSegment(address, static_cast<const TSegment&>(segment));

    if (inSegment)
    {
        return !segment.IsFreeOrDecommitted(address);
    }

    return inSegment;
}

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
bool
PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment>::IsAddressInSegment(__in void* address, const TSegment& segment)
{
    return segment.IsInSegment(address);
}

#if PDATA_ENABLED
#include "Memory/XDataAllocator.h"
template<typename T>
bool HeapPageAllocator<T>::CreateSecondaryAllocator(SegmentBase<T>* segment, bool committed, SecondaryAllocator** allocator)
{
    Assert(segment->GetAllocator() == this);
    Assert(segment->IsInCustomHeapAllocator());

    // If we are not allocating xdata there is nothing to do

    // ARM might allocate XDATA but not have a reserved region for it (no secondary alloc reserved space)
    if (!allocXdata)
    {
        Assert(segment->GetSecondaryAllocSize() == 0);
        *allocator = nullptr;
        return true;
    }

    if (!committed && segment->GetSecondaryAllocSize() != 0 &&
        !this->GetVirtualAllocator()->Alloc(segment->GetSecondaryAllocStartAddress(), segment->GetSecondaryAllocSize(),
        MEM_COMMIT, PAGE_READWRITE, true))
    {
        *allocator = nullptr;
        return false;
    }

    XDataAllocator* secondaryAllocator = HeapNewNoThrow(XDataAllocator, (BYTE*)segment->GetSecondaryAllocStartAddress(), segment->GetSecondaryAllocSize());
    bool success = false;
    if (secondaryAllocator)
    {
        if (secondaryAllocator->Initialize((BYTE*)segment->GetAddress(), (BYTE*)segment->GetEndAddress()))
        {
            success = true;
        }
        else
        {
            HeapDelete(secondaryAllocator);
            secondaryAllocator = nullptr;
        }
    }
    *allocator = secondaryAllocator;
    return success;
}
#endif

template<typename T>
uint PageSegmentBase<T>::GetCountOfFreePages() const
{
    return this->freePages.Count();
}

template<typename T>
uint PageSegmentBase<T>::GetNextBitInFreePagesBitVector(uint index) const
{
    return this->freePages.GetNextBit(index);
}

template<typename T>
BOOLEAN PageSegmentBase<T>::TestRangeInFreePagesBitVector(uint index, uint pageCount) const
{
    return this->freePages.TestRange(index, pageCount);
}

template<typename T>
BOOLEAN PageSegmentBase<T>::TestInFreePagesBitVector(uint index) const
{
    return this->freePages.Test(index);
}

template<typename T>
void PageSegmentBase<T>::ClearAllInFreePagesBitVector()
{
    return this->freePages.ClearAll();
}

template<typename T>
void PageSegmentBase<T>::ClearRangeInFreePagesBitVector(uint index, uint pageCount)
{
    return this->freePages.ClearRange(index, pageCount);
}

template<typename T>
void PageSegmentBase<T>::SetRangeInFreePagesBitVector(uint index, uint pageCount)
{
    return this->freePages.SetRange(index, pageCount);
}

template<typename T>
void PageSegmentBase<T>::ClearBitInFreePagesBitVector(uint index)
{
    return this->freePages.Clear(index);
}

template<typename T>
BOOLEAN PageSegmentBase<T>::TestInDecommitPagesBitVector(uint index) const
{
    return this->decommitPages.Test(index);
}

template<typename T>
BOOLEAN PageSegmentBase<T>::TestRangeInDecommitPagesBitVector(uint index, uint pageCount) const
{
    return this->decommitPages.TestRange(index, pageCount);
}

template<typename T>
void PageSegmentBase<T>::SetRangeInDecommitPagesBitVector(uint index, uint pageCount)
{
    return this->decommitPages.SetRange(index, pageCount);
}

template<typename T>
void PageSegmentBase<T>::ClearRangeInDecommitPagesBitVector(uint index, uint pageCount)
{
    return this->decommitPages.ClearRange(index, pageCount);
}

template<typename T>
uint PageSegmentBase<T>::GetCountOfDecommitPages() const
{
    return this->decommitPages.Count();
}

template<typename T>
void PageSegmentBase<T>::SetBitInDecommitPagesBitVector(uint index)
{
    this->decommitPages.Set(index);
}

template<typename T>
template <bool noPageAligned>
char * PageSegmentBase<T>::DoAllocDecommitPages(uint pageCount)
{
    return this->AllocDecommitPages<PageSegmentBase<T>::PageBitVector, noPageAligned>(pageCount, this->freePages, this->decommitPages);
}

template<typename T>
uint PageSegmentBase<T>::GetMaxPageCount()
{
    return MaxPageCount;
}

namespace Memory
{
    //Instantiate all the Templates in this class below.
    template class PageAllocatorBase < PreReservedVirtualAllocWrapper >;
    template class PageAllocatorBase < VirtualAllocWrapper >;
    template class HeapPageAllocator < PreReservedVirtualAllocWrapper >;
    template class HeapPageAllocator < VirtualAllocWrapper >;
    template class SegmentBase       < VirtualAllocWrapper > ;
    template class SegmentBase       < PreReservedVirtualAllocWrapper >;
    template class PageSegmentBase   < VirtualAllocWrapper >;
    template class PageSegmentBase   < PreReservedVirtualAllocWrapper >;
#if ENABLE_OOP_NATIVE_CODEGEN
    template class PageAllocatorBase < SectionAllocWrapper >;
    template class PageAllocatorBase < PreReservedSectionAllocWrapper >;
    template class HeapPageAllocator < SectionAllocWrapper >;
    template class HeapPageAllocator < PreReservedSectionAllocWrapper >;
    template class SegmentBase       < SectionAllocWrapper >;
    template class SegmentBase       < PreReservedSectionAllocWrapper >;
    template class PageSegmentBase   < SectionAllocWrapper >;
    template class PageSegmentBase   < PreReservedSectionAllocWrapper >;
#endif
}
