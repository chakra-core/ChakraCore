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

BucketId GetBucketForSize(size_t bytes);

struct PageAllocatorAllocation
{
    bool isDecommitted;
};

struct Page: public PageAllocatorAllocation
{
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
      freeBitVector(0xFFFFFFFF)
    {
        // Initialize PageAllocatorAllocation fields
        this->isDecommitted = false;
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
        struct: PageAllocatorAllocation
        {
            void* segment;
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

#if defined(_M_ARM32_OR_ARM64)
    void RegisterPdata(ULONG_PTR functionStart, DWORD length)
    {
        Assert(this->xdata.pdataCount > 0);
        XDataAllocator* xdataAllocator = GetXDataAllocator();
        xdataAllocator->Register(this->xdata, functionStart, length);
    }
#endif
#endif

};

/*
 * Simple free-listing based heap allocator
 *
 * Each allocation is tracked using a "HeapAllocation" record
 * Once we alloc, we start assigning chunks sliced from the end of a HeapAllocation
 * If we don't have enough to slice off, we push a new heap allocation record to the record stack, and try and assign from that
 */
class Heap
{
public:
    Heap(AllocationPolicyManager * policyManager, ArenaAllocator * alloc, bool allocXdata);

    Allocation* Alloc(size_t bytes, ushort pdataCount, ushort xdataSize, bool canAllocInPreReservedHeapPageSegment, bool isAnyJittedCode, _Inout_ bool* isAllJITCodeInPreReservedRegion);
    bool Free(__in Allocation* allocation);
    bool Decommit(__in Allocation* allocation);
    void FreeAll();
    bool IsInRange(__in void* address);

    template<typename T>
    HeapPageAllocator<T>* GetPageAllocator(Page * page)
    {
        AssertMsg(page, "Why is page null?");
        return GetPageAllocator<T>(page->segment);
    }

    template<typename T>
    HeapPageAllocator<T>* GetPageAllocator(void * segmentParam)
    {
        SegmentBase<T> * segment = (SegmentBase<T>*)segmentParam;
        AssertMsg(segment, "Why is segment null?");
        AssertMsg(segment->GetAllocator(), "Segment doesn't have an allocator?");

        Assert((HeapPageAllocator<VirtualAllocWrapper>*)(segment->GetAllocator()) == &this->pageAllocator ||
            (HeapPageAllocator<PreReservedVirtualAllocWrapper>*)(segment->GetAllocator()) == &this->preReservedHeapPageAllocator);

        return (HeapPageAllocator<T> *)(segment->GetAllocator());
    }

    bool IsPreReservedSegment(void * segment)
    {
        Assert(segment);
        return (((Segment*)(segment))->IsInPreReservedHeapPageAllocator());
    }

    HeapPageAllocator<PreReservedVirtualAllocWrapper> * GetPreReservedHeapPageAllocator()
    {
        return &preReservedHeapPageAllocator;
    }

    HeapPageAllocator<VirtualAllocWrapper>* GetHeapPageAllocator()
    {
        Assert(!pageAllocator.GetVirtualAllocator()->IsPreReservedRegionPresent());
        return &pageAllocator;
    }

    void ReleasePages(void* pageAddress, uint pageCount, __in void* segment)
    {
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPageAllocator<PreReservedVirtualAllocWrapper>(segment)->ReleasePages(pageAddress, pageCount, segment);
        }
        else
        {
            this->GetPageAllocator<VirtualAllocWrapper>(segment)->ReleasePages(pageAddress, pageCount, segment);
        }
    }

    BOOL ProtectPages(__in char* address, size_t pageCount, __in void* segment, DWORD dwVirtualProtectFlags, DWORD desiredOldProtectFlag)
    {
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            return this->GetPageAllocator<PreReservedVirtualAllocWrapper>(segment)->ProtectPages(address, pageCount, segment, dwVirtualProtectFlags, desiredOldProtectFlag);
        }
        else
        {
            return this->GetPageAllocator<VirtualAllocWrapper>(segment)->ProtectPages(address, pageCount, segment, dwVirtualProtectFlags, desiredOldProtectFlag);
        }
    }

    void TrackDecommittedPages(void * address, uint pageCount, __in void* segment)
    {
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPageAllocator<PreReservedVirtualAllocWrapper>(segment)->TrackDecommittedPages(address, pageCount, segment);
        }
        else
        {
            this->GetPageAllocator<VirtualAllocWrapper>(segment)->TrackDecommittedPages(address, pageCount, segment);
        }
    }

    void ReleaseSecondary(const SecondaryAllocation& allocation, void* segment)
    {
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPageAllocator<PreReservedVirtualAllocWrapper>(segment)->ReleaseSecondary(allocation, segment);
        }
        else
        {
            this->GetPageAllocator<VirtualAllocWrapper>(segment)->ReleaseSecondary(allocation, segment);
        }
    }

    void DecommitPages(__in char* address, size_t pageCount, void* segment)
    {
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPageAllocator<PreReservedVirtualAllocWrapper>(segment)->DecommitPages(address, pageCount);
        }
        else
        {
            this->GetPageAllocator<VirtualAllocWrapper>(segment)->DecommitPages(address, pageCount);
        }
    }

    bool AllocSecondary(void* segment, ULONG_PTR functionStart, size_t functionSize_t, ushort pdataCount, ushort xdataSize, SecondaryAllocation* allocation)
    {
        Assert(functionSize_t <= MAXUINT32);
        DWORD functionSize = static_cast<DWORD>(functionSize_t);
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            return this->GetPageAllocator<PreReservedVirtualAllocWrapper>(segment)->AllocSecondary(segment, functionStart, functionSize, pdataCount, xdataSize, allocation);
        }
        else
        {
            return this->GetPageAllocator<VirtualAllocWrapper>(segment)->AllocSecondary(segment, functionStart, functionSize, pdataCount, xdataSize, allocation);
        }
    }

    void Release(void * address, size_t pageCount, void * segment)
    {
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPageAllocator<PreReservedVirtualAllocWrapper>(segment)->Release(address, pageCount, segment);
        }
        else
        {
            this->GetPageAllocator<VirtualAllocWrapper>(segment)->Release(address, pageCount, segment);
        }
    }

    void ReleaseDecommitted(void * address, size_t pageCount, __in void *  segment)
    {
        Assert(segment);
        if (IsPreReservedSegment(segment))
        {
            this->GetPageAllocator<PreReservedVirtualAllocWrapper>(segment)->ReleaseDecommitted(address, pageCount, segment);
        }
        else
        {
            this->GetPageAllocator<VirtualAllocWrapper>(segment)->ReleaseDecommitted(address, pageCount, segment);
        }
    }

    char * EnsurePreReservedPageAllocation(PreReservedVirtualAllocWrapper * preReservedVirtualAllocator);
    // A page should be in full list if:
    // 1. It does not have any space
    // 2. Parent segment cannot allocate any more XDATA
    bool ShouldBeInFullList(Page* page)
    {
        return page->HasNoSpace() || (allocXdata && !((Segment*)(page->segment))->CanAllocSecondary());
    }

    BOOL ProtectAllocation(__in Allocation* allocation, DWORD dwVirtualProtectFlags, DWORD desiredOldProtectFlag, __in_opt char* addressInPage = nullptr);
    BOOL ProtectAllocationWithExecuteReadWrite(Allocation *allocation, char* addressInPage = nullptr);
    BOOL ProtectAllocationWithExecuteReadOnly(Allocation *allocation, char* addressInPage = nullptr);

    ~Heap();

#if DBG_DUMP
    void DumpStats();
#endif

private:
    /**
     * Inline methods
     */
    inline unsigned int GetChunkSizeForBytes(size_t bytes)
    {
        return (bytes > Page::Alignment ? static_cast<unsigned int>(bytes) / Page::Alignment : 1);
    }

    inline size_t GetNumPagesForSize(size_t bytes)
    {
        size_t allocSize = AllocSizeMath::Add(bytes, AutoSystemInfo::PageSize);

        if (allocSize == (size_t) -1)
        {
            return 0;
        }

        return ((allocSize - 1)/ AutoSystemInfo::PageSize);
    }

    inline BVIndex GetFreeIndexForPage(Page* page, size_t bytes)
    {
        unsigned int length = GetChunkSizeForBytes(bytes);
        BVIndex index = page->freeBitVector.FirstStringOfOnes(length);

        return index;
    }

    /**
     * Large object methods
     */
    Allocation* AllocLargeObject(size_t bytes, ushort pdataCount, ushort xdataSize, bool canAllocInPreReservedHeapPageSegment, bool isAnyJittedCode, _Inout_ bool* isAllJITCodeInPreReservedRegion);

    template<bool freeAll>
    bool FreeLargeObject(Allocation* header);

    void FreeLargeObjects()
    {
        FreeLargeObject<true>(nullptr);
    }

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

        BOOL result = this->ProtectPages(page->address, 1, page->segment, readWriteFlags, PAGE_EXECUTE);
        Assert(result && (PAGE_EXECUTE & readWriteFlags) == 0);
        return PAGE_EXECUTE;
    }

    template<DWORD readWriteFlags>
    DWORD EnsureAllocationReadWrite(Allocation* allocation)
    {
        if (allocation->IsLargeAllocation())
        {
            BOOL result = this->ProtectAllocation(allocation, readWriteFlags, PAGE_EXECUTE);
            Assert(result && (PAGE_EXECUTE & readWriteFlags) == 0);
            return PAGE_EXECUTE;
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

#if PDATA_ENABLED
    void FreeXdata(XDataAllocation* xdata, void* segment);
#endif

    void FreeDecommittedBuckets();
    void FreeDecommittedLargeObjects();

    /**
     * Page methods
     */
    Page*       AddPageToBucket(Page* page, BucketId bucket, bool wasFull = false);
    Allocation* AllocInPage(Page* page, size_t bytes, ushort pdataCount, ushort xdataSize);
    Page*       AllocNewPage(BucketId bucket, bool canAllocInPreReservedHeapPageSegment, bool isAnyJittedCode, _Inout_ bool* isAllJITCodeInPreReservedRegion);
    Page*       FindPageToSplit(BucketId targetBucket, bool findPreReservedHeapPages = false);

    template<class Fn>
    void TransferPages(Fn predicate, DListBase<Page>* fromList, DListBase<Page>* toList)
    {
        Assert(fromList != toList);

        for(int bucket = 0; bucket < BucketId::NumBuckets; bucket++)
        {
            FOREACH_DLISTBASE_ENTRY_EDITING(Page, page, &(fromList[bucket]), bucketIter)
            {
                if(predicate(&page))
                {
                    bucketIter.MoveCurrentTo(&(toList[bucket]));
                }
            }
            NEXT_DLISTBASE_ENTRY_EDITING;
        }
    }

    BVIndex     GetIndexInPage(__in Page* page, __in char* address);
    void        RemovePageFromFullList(Page* page);

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
    HeapPageAllocator<VirtualAllocWrapper>               pageAllocator;
    HeapPageAllocator<PreReservedVirtualAllocWrapper>    preReservedHeapPageAllocator;
    ArenaAllocator*                                   auxiliaryAllocator;

    /*
     * Various tracking lists
     */
    DListBase<Page>        buckets[NumBuckets];
    DListBase<Page>        fullPages[NumBuckets];
    DListBase<Allocation>  largeObjectAllocations;

    DListBase<Page>        decommittedPages;
    DListBase<Allocation>  decommittedLargeObjects;

    // Critical section synchronize in the BGJIT thread and IsInRange in the main thread
    CriticalSection        cs;
    bool                   allocXdata;
#if DBG
    bool inDtor;
#endif
};

// Helpers
unsigned int log2(size_t number);
BucketId GetBucketForSize(size_t bytes);
void FillDebugBreak(__out_bcount_full(byteCount) BYTE* buffer, __in size_t byteCount);
};
}
