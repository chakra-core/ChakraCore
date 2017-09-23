//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
namespace Memory
{

#define VerboseHeapTrace(...) { \
    OUTPUT_VERBOSE_TRACE(Js::CustomHeapPhase, __VA_ARGS__); \
}


#define HeapTrace(...) { \
    Output::Print(__VA_ARGS__); \
    Output::Flush(); \
}

namespace CustomHeap
{

enum BucketId
{
    InvalidBucket = -1,
    SmallObjectList,
    Bucket256,
    Bucket512,
    Bucket1024,
    Bucket2048,
    Bucket4096,
    LargeObjectList,
    NumBuckets
};

BucketId GetBucketForSize(DECLSPEC_GUARD_OVERFLOW size_t bytes);

struct Page
{
    bool inFullList;
    bool isDecommitted;
    void* segment;
    BVUnit       freeBitVector;
    char*        address;
    BucketId     currentBucket;

    bool HasNoSpace()
    {
        return freeBitVector.IsEmpty();
    }

    bool IsEmpty()
    {
        return freeBitVector.IsFull();
    }

    bool CanAllocate(BucketId targetBucket)
    {
        return freeBitVector.FirstStringOfOnes(targetBucket + 1) != BVInvalidIndex;
    }

    Page(__in char* address, void* segment, BucketId bucket):
      address(address),
      segment(segment),
      currentBucket(bucket),
      freeBitVector(0xFFFFFFFF),
    isDecommitted(false),
    inFullList(false)
    {
    }

    // Each bit in the bit vector corresponds to 128 bytes of memory
    // This implies that 128 bytes is the smallest allocation possible
    static const uint Alignment = 128;
    static const uint MaxAllocationSize = 4096;
};

struct Allocation
{
    union
    {
        Page*  page;
        struct
        {
            void* segment;
            bool isDecommitted;
        } largeObjectAllocation;
    };

    __field_bcount(size) char* address;
    size_t size;

    bool IsLargeAllocation() const { return size > Page::MaxAllocationSize; }
    size_t GetPageCount() const { Assert(this->IsLargeAllocation()); return size / AutoSystemInfo::PageSize; }

#if DBG
    // Initialized to false, this is set to true when the allocation
    // is actually used by the emit buffer manager
    // This is almost always true- it's there only for assertion purposes
    bool isAllocationUsed: 1;
    bool isNotExecutableBecauseOOM: 1;
#endif

#if PDATA_ENABLED
    XDataAllocation xdata;
    XDataAllocator* GetXDataAllocator()
    {
        XDataAllocator* allocator;
        if (!this->IsLargeAllocation())
        {
            allocator = static_cast<XDataAllocator*>(((Segment*)(this->page->segment))->GetSecondaryAllocator());
        }
        else
        {
            allocator = static_cast<XDataAllocator*>(((Segment*) (largeObjectAllocation.segment))->GetSecondaryAllocator());
        }
        return allocator;
    }
#endif
};

// Wrapper for the two HeapPageAllocator with and without the prereserved segment.
// Supports multiple thread access. Require explicit locking (via AutoCriticalSection)
template <typename TAlloc, typename TPreReservedAlloc>
class CodePageAllocators
{
public:
    CodePageAllocators(AllocationPolicyManager * policyManager, bool allocXdata, PreReservedVirtualAllocWrapper * virtualAllocator, HANDLE processHandle) :
        pageAllocator(policyManager, allocXdata, true /*excludeGuardPages*/, nullptr, processHandle),
        preReservedHeapAllocator(policyManager, allocXdata, true /*excludeGuardPages*/, virtualAllocator, processHandle),
        cs(4000),
        secondaryAllocStateChangedCount(0),
        processHandle(processHandle)
    {
#if DBG
        this->preReservedHeapAllocator.ClearConcurrentThreadId();
        this->pageAllocator.ClearConcurrentThreadId();
#endif
    }

#if _WIN32
    CodePageAllocators(AllocationPolicyManager * policyManager, bool allocXdata, SectionAllocWrapper * sectionAllocator, PreReservedSectionAllocWrapper * virtualAllocator, HANDLE processHandle) :
        pageAllocator(policyManager, allocXdata, true /*excludeGuardPages*/, sectionAllocator, processHandle),
        preReservedHeapAllocator(policyManager, allocXdata, true /*excludeGuardPages*/, virtualAllocator, processHandle),
        cs(4000),
        secondaryAllocStateChangedCount(0),
        processHandle(processHandle)
    {
#if DBG
        this->preReservedHeapAllocator.ClearConcurrentThreadId();
        this->pageAllocator.ClearConcurrentThreadId();
#endif
    }
#endif

    bool AllocXdata()
    {
        // Simple immutable data access, no need for lock
        return preReservedHeapAllocator.AllocXdata();
    }

    bool IsPreReservedSegment(void * segment)
    {
        // Simple immutable data access, no need for lock
        Assert(segment);
        return reinterpret_cast<SegmentBaseCommon*>(segment)->IsInPreReservedHeapPageAllocator();
    }

    bool IsInNonPreReservedPageAllocator(__in void *address)
    {
        Assert(this->cs.IsLocked());
        return this->pageAllocator.IsAddressFromAllocator(address);
    }

    char * Alloc(size_t * pages, void ** segment, bool canAllocInPreReservedHeapPageSegment, bool isAnyJittedCode, bool * isAllJITCodeInPreReservedRegion)
    {
        Assert(this->cs.IsLocked());
        char* address = nullptr;
        if (canAllocInPreReservedHeapPageSegment)
        {
            address = this->preReservedHeapAllocator.Alloc(pages, (SegmentBase<TPreReservedAlloc>**)(segment));
        }

        if (address == nullptr)
        {
            if (isAnyJittedCode)
            {
                *isAllJITCodeInPreReservedRegion = false;
            }
            address = this->pageAllocator.Alloc(pages, (SegmentBase<TAlloc>**)segment);
        }
        return address;
    }

    char * AllocLocal(char * remoteAddr, size_t size, void * segment);
    void FreeLocal(char * addr, void * segment);

    char * AllocPages(DECLSPEC_GUARD_OVERFLOW uint pages, void ** pageSegment, bool canAllocInPreReservedHeapPageSegment, bool isAnyJittedCode, bool * isAllJITCodeInPreReservedRegion)
    {
        Assert(this->cs.IsLocked());
        char * address = nullptr;
        if (canAllocInPreReservedHeapPageSegment)
        {
            address = this->preReservedHeapAllocator.AllocPages(pages, (PageSegmentBase<TPreReservedAlloc>**)pageSegment);

            if (address == nullptr)
            {
                VerboseHeapTrace(_u("PRE-RESERVE: PreReserved Segment CANNOT be allocated \n"));
            }
        }

        if (address == nullptr)    // if no space in Pre-reserved Page Segment, then allocate in regular ones.
        {
            if (isAnyJittedCode)
            {
                *isAllJITCodeInPreReservedRegion = false;
            }
            address = this->pageAllocator.AllocPages(pages, (PageSegmentBase<TAlloc>**)pageSegment);
        }
        else
        {
            VerboseHeapTrace(_u("PRE-RESERVE: Allocing new page in PreReserved Segment \n"));
        }

        return address;
    }

    void ReleasePages(void* pageAddress, uint pageCount, __in void* segment)
    {
        Assert(this->cs.IsLocked());
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPreReservedPageAllocator(segment)->ReleasePages(pageAddress, pageCount, segment);
        }
        else
        {
            this->GetPageAllocator(segment)->ReleasePages(pageAddress, pageCount, segment);
        }
    }

    BOOL ProtectPages(__in char* address, size_t pageCount, __in void* segment, DWORD dwVirtualProtectFlags, DWORD desiredOldProtectFlag)
    {
        // This is merely a wrapper for VirtualProtect, no need to synchornize, and doesn't touch any data.
        // No need to assert locked.
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            return this->GetPreReservedPageAllocator(segment)->ProtectPages(address, pageCount, segment, dwVirtualProtectFlags, desiredOldProtectFlag);
        }
        else
        {
            return this->GetPageAllocator(segment)->ProtectPages(address, pageCount, segment, dwVirtualProtectFlags, desiredOldProtectFlag);
        }
    }

    void TrackDecommittedPages(void * address, uint pageCount, __in void* segment)
    {
        Assert(this->cs.IsLocked());
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPreReservedPageAllocator(segment)->TrackDecommittedPages(address, pageCount, segment);
        }
        else
        {
            this->GetPageAllocator(segment)->TrackDecommittedPages(address, pageCount, segment);
        }
    }

    void ReleaseSecondary(const SecondaryAllocation& allocation, void* segment)
    {
        Assert(this->cs.IsLocked());
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            secondaryAllocStateChangedCount += (uint)this->GetPreReservedPageAllocator(segment)->ReleaseSecondary(allocation, segment);
        }
        else
        {
            secondaryAllocStateChangedCount += (uint)this->GetPageAllocator(segment)->ReleaseSecondary(allocation, segment);
        }
    }

    bool HasSecondaryAllocStateChanged(uint * lastSecondaryAllocStateChangedCount)
    {
        if (secondaryAllocStateChangedCount != *lastSecondaryAllocStateChangedCount)
        {
            *lastSecondaryAllocStateChangedCount = secondaryAllocStateChangedCount;
            return true;
        }
        return false;
    }

    void DecommitPages(__in char* address, size_t pageCount, void* segment)
    {
        // This is merely a wrapper for VirtualFree, no need to synchornize, and doesn't touch any data.
        // No need to assert locked.
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPreReservedPageAllocator(segment)->DecommitPages(address, pageCount);
        }
        else
        {
            this->GetPageAllocator(segment)->DecommitPages(address, pageCount);
        }
    }

    bool AllocSecondary(void* segment, ULONG_PTR functionStart, size_t functionSize_t, ushort pdataCount, ushort xdataSize, SecondaryAllocation* allocation)
    {
        Assert(this->cs.IsLocked());
        Assert(functionSize_t <= MAXUINT32);
        DWORD functionSize = static_cast<DWORD>(functionSize_t);
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            return this->GetPreReservedPageAllocator(segment)->AllocSecondary(segment, functionStart, functionSize, pdataCount, xdataSize, allocation);
        }
        else
        {
            return this->GetPageAllocator(segment)->AllocSecondary(segment, functionStart, functionSize, pdataCount, xdataSize, allocation);
        }
    }

    void Release(void * address, size_t pageCount, void * segment)
    {
        Assert(this->cs.IsLocked());
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPreReservedPageAllocator(segment)->Release(address, pageCount, segment);
        }
        else
        {
            this->GetPageAllocator(segment)->Release(address, pageCount, segment);
        }
    }

    void ReleaseDecommitted(void * address, size_t pageCount, __in void *  segment)
    {
        Assert(this->cs.IsLocked());
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPreReservedPageAllocator(segment)->ReleaseDecommitted(address, pageCount, segment);
        }
        else
        {
            this->GetPageAllocator(segment)->ReleaseDecommitted(address, pageCount, segment);
        }
    }
    CriticalSection cs;
private:

    template<typename T>
    HeapPageAllocator<T>* GetPageAllocator(Page * page)
    {
        AssertMsg(page, "Why is page null?");
        return GetPageAllocator<T>(page->segment);
    }

    HeapPageAllocator<TAlloc>* GetPageAllocator(void * segmentParam)
    {
        SegmentBase<TAlloc> * segment = (SegmentBase<TAlloc>*)segmentParam;
        AssertMsg(segment, "Why is segment null?");
        Assert((HeapPageAllocator<TAlloc>*)(segment->GetAllocator()) == &this->pageAllocator);
        return (HeapPageAllocator<TAlloc> *)(segment->GetAllocator());
    }

    HeapPageAllocator<TPreReservedAlloc>* GetPreReservedPageAllocator(void * segmentParam)
    {
        SegmentBase<TPreReservedAlloc> * segment = (SegmentBase<TPreReservedAlloc>*)segmentParam;
        AssertMsg(segment, "Why is segment null?");
        Assert((HeapPageAllocator<TPreReservedAlloc>*)(segment->GetAllocator()) == &this->preReservedHeapAllocator);
        return (HeapPageAllocator<TPreReservedAlloc> *)(segment->GetAllocator());
    }

    HeapPageAllocator<TAlloc>               pageAllocator;
    HeapPageAllocator<TPreReservedAlloc>  preReservedHeapAllocator;
    HANDLE processHandle;

    // Track the number of time a segment's secondary allocate change from full to available to allocate.
    // So that we know whether CustomHeap to know when to update their "full page"
    // It is ok to overflow this variable.  All we care is if the state has changed.
    // If in the unlikely scenario that we do overflow, then we delay the full pages in CustomHeap from
    // being made available.
    uint secondaryAllocStateChangedCount;
};

typedef CodePageAllocators<VirtualAllocWrapper, PreReservedVirtualAllocWrapper> InProcCodePageAllocators;
#if _WIN32
typedef CodePageAllocators<SectionAllocWrapper, PreReservedSectionAllocWrapper> OOPCodePageAllocators;
#endif
/*
 * Simple free-listing based heap allocator
 *
 * Each allocation is tracked using a "HeapAllocation" record
 * Once we alloc, we start assigning chunks sliced from the end of a HeapAllocation
 * If we don't have enough to slice off, we push a new heap allocation record to the record stack, and try and assign from that
 *
 * Single thread only. Require external locking.  (Currently, EmitBufferManager manage the locking)
 */
template <typename TAlloc, typename TPreReservedAlloc>
class Heap
{
public:
    Heap(ArenaAllocator * alloc, CodePageAllocators<TAlloc, TPreReservedAlloc>  * codePageAllocators, HANDLE processHandle);

    Allocation* Alloc(DECLSPEC_GUARD_OVERFLOW size_t bytes, ushort pdataCount, ushort xdataSize, bool canAllocInPreReservedHeapPageSegment, bool isAnyJittedCode, _Inout_ bool* isAllJITCodeInPreReservedRegion);
    void Free(__in Allocation* allocation);
    void DecommitAll();
    void FreeAll();
    bool IsInHeap(__in void* address);

    // A page should be in full list if:
    // 1. It does not have any space
    // 2. Parent segment cannot allocate any more XDATA
    bool ShouldBeInFullList(Page* page)
    {
        return page->HasNoSpace() || (codePageAllocators->AllocXdata() && !((Segment*)(page->segment))->CanAllocSecondary());
    }

    BOOL ProtectAllocation(__in Allocation* allocation, DWORD dwVirtualProtectFlags, DWORD desiredOldProtectFlag, __in_opt char* addressInPage = nullptr);
    BOOL ProtectAllocationWithExecuteReadWrite(Allocation *allocation, __in_opt char* addressInPage = nullptr);
    BOOL ProtectAllocationWithExecuteReadOnly(Allocation *allocation, __in_opt char* addressInPage = nullptr);

    ~Heap();

#if DBG_DUMP
    void DumpStats();
#endif

private:
    /**
     * Inline methods
     */
    inline unsigned int GetChunkSizeForBytes(DECLSPEC_GUARD_OVERFLOW size_t bytes)
    {
        return (bytes > Page::Alignment ? static_cast<unsigned int>(bytes) / Page::Alignment : 1);
    }

    inline size_t GetNumPagesForSize(DECLSPEC_GUARD_OVERFLOW size_t bytes)
    {
        size_t allocSize = AllocSizeMath::Add(bytes, AutoSystemInfo::PageSize);

        if (allocSize == (size_t) -1)
        {
            return 0;
        }

        return ((allocSize - 1)/ AutoSystemInfo::PageSize);
    }

    inline BVIndex GetFreeIndexForPage(Page* page, DECLSPEC_GUARD_OVERFLOW size_t bytes)
    {
        unsigned int length = GetChunkSizeForBytes(bytes);
        BVIndex index = page->freeBitVector.FirstStringOfOnes(length);

        return index;
    }

    /**
     * Large object methods
     */
    Allocation* AllocLargeObject(DECLSPEC_GUARD_OVERFLOW size_t bytes, ushort pdataCount, ushort xdataSize, bool canAllocInPreReservedHeapPageSegment, bool isAnyJittedCode, _Inout_ bool* isAllJITCodeInPreReservedRegion);

    void FreeLargeObject(Allocation* header);

    void FreeLargeObjects();

    //Called during Free
    DWORD EnsurePageWriteable(Page* page);

    // this get called when freeing the whole page
    DWORD EnsureAllocationWriteable(Allocation* allocation);

    // this get called when only freeing a part in the page
    DWORD EnsureAllocationExecuteWriteable(Allocation* allocation);

    template<DWORD readWriteFlags>
    DWORD EnsurePageReadWrite(Page* page)
    {
        Assert(!page->isDecommitted);
        this->codePageAllocators->ProtectPages(page->address, 1, page->segment, readWriteFlags, PAGE_EXECUTE_READ);
        return PAGE_EXECUTE_READ;
    }

    template<DWORD readWriteFlags>

    DWORD EnsureAllocationReadWrite(Allocation* allocation)
    {
        if (allocation->IsLargeAllocation())
        {
            this->ProtectAllocation(allocation, readWriteFlags, PAGE_EXECUTE_READ);
            return PAGE_EXECUTE_READ;
        }
        else
        {
            return EnsurePageReadWrite<readWriteFlags>(allocation->page);
        }
    }

    /**
     * Freeing Methods
     */
    void FreeBuckets(bool freeOnlyEmptyPages);
    void FreeBucket(DListBase<Page>* bucket, bool freeOnlyEmptyPages);
    void FreePage(Page* page);
    bool FreeAllocation(Allocation* allocation);
    void FreeAllocationHelper(Allocation * allocation, BVIndex index, uint length);

#if PDATA_ENABLED
    void FreeXdata(XDataAllocation* xdata, void* segment);
#endif

    void FreeDecommittedBuckets();
    void FreeDecommittedLargeObjects();

    /**
     * Page methods
     */
    Page*       AddPageToBucket(Page* page, BucketId bucket, bool wasFull = false);
    bool        AllocInPage(Page* page, DECLSPEC_GUARD_OVERFLOW size_t bytes, ushort pdataCount, ushort xdataSize, Allocation ** allocation);
    Page*       AllocNewPage(BucketId bucket, bool canAllocInPreReservedHeapPageSegment, bool isAnyJittedCode, _Inout_ bool* isAllJITCodeInPreReservedRegion);
    Page*       FindPageToSplit(BucketId targetBucket, bool findPreReservedHeapPages = false);

    bool        UpdateFullPages();
    Page *      GetExistingPage(BucketId bucket, bool canAllocInPreReservedHeapPageSegment);

    BVIndex     GetIndexInPage(__in Page* page, __in char* address);
    bool        IsInHeap(DListBase<Page> const buckets[NumBuckets], __in void *address);
    bool        IsInHeap(DListBase<Page> const& buckets, __in void *address);
    bool        IsInHeap(DListBase<Allocation> const& allocations, __in void *address);

    /**
     * Stats
     */
#if DBG_DUMP
    size_t totalAllocationSize;
    size_t freeObjectSize;
    size_t allocationsSinceLastCompact;
    size_t freesSinceLastCompact;
#endif

    /**
     * Allocator stuff
     */
    CodePageAllocators<TAlloc, TPreReservedAlloc> *   codePageAllocators;
    ArenaAllocator*        auxiliaryAllocator;

    /*
     * Various tracking lists
     */
    DListBase<Page>        buckets[NumBuckets];
    DListBase<Page>        fullPages[NumBuckets];
    DListBase<Allocation>  largeObjectAllocations;

    DListBase<Page>        decommittedPages;
    DListBase<Allocation>  decommittedLargeObjects;

    uint                   lastSecondaryAllocStateChangedCount;
    HANDLE                 processHandle;
#if DBG
    bool inDtor;
#endif
};

typedef Heap<VirtualAllocWrapper, PreReservedVirtualAllocWrapper> InProcHeap;
#if _WIN32
typedef Heap<SectionAllocWrapper, PreReservedSectionAllocWrapper> OOPHeap;
#endif
// Helpers
unsigned int log2(size_t number);
BucketId GetBucketForSize(DECLSPEC_GUARD_OVERFLOW size_t bytes);
void FillDebugBreak(_Out_writes_bytes_all_(byteCount) BYTE* buffer, _In_ size_t byteCount);
} // namespace CustomHeap
} // namespace Memory
