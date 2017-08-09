//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"


HeapBucket::HeapBucket() :
    heapInfo(nullptr),
    sizeCat(0)
{
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    heapBlockCount = 0;
    newHeapBlockCount = 0;
    emptyHeapBlockCount = 0;
#endif

#ifdef RECYCLER_PAGE_HEAP
    isPageHeapEnabled = false;
#endif
}

uint
HeapBucket::GetBucketIndex() const
{
    return HeapInfo::GetBucketIndex(this->sizeCat);
}

uint
HeapBucket::GetMediumBucketIndex() const
{
    return HeapInfo::GetMediumBucketIndex(this->sizeCat);
}

namespace Memory
{

template <typename TBlockType>
HeapBucketT<TBlockType>::HeapBucketT() :
    nextAllocableBlockHead(nullptr),
    emptyBlockList(nullptr),
    fullBlockList(nullptr),
    heapBlockList(nullptr),
#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    allocableHeapBlockListHead((PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT)),
    sweepableHeapBlockList(nullptr),
#endif
#endif
    explicitFreeList(nullptr),
    lastExplicitFreeListAllocator(nullptr)
{
#ifdef RECYCLER_PAGE_HEAP
    explicitFreeLockBlockList = nullptr;
#endif

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    this->allocationsStartedDuringConcurrentSweep = false;
#endif

    isAllocationStopped = false;
}


template <typename TBlockType>
HeapBucketT<TBlockType>::~HeapBucketT()
{
    DeleteHeapBlockList(this->heapBlockList);
    DeleteHeapBlockList(this->fullBlockList);

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (allocableHeapBlockListHead != nullptr)
    {
        if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
        {
            FlushInterlockedSList(this->allocableHeapBlockListHead);
        }

        _aligned_free(this->allocableHeapBlockListHead);
    }

    DeleteHeapBlockList(sweepableHeapBlockList);
#endif
#endif

    Assert(this->heapBlockCount + this->newHeapBlockCount == 0);
    RECYCLER_SLOW_CHECK(Assert(this->emptyHeapBlockCount == HeapBlockList::Count(this->emptyBlockList)));
    DeleteEmptyHeapBlockList(this->emptyBlockList);
    Assert(this->heapBlockCount + this->newHeapBlockCount + this->emptyHeapBlockCount == 0);
}
};

template <typename TBlockType>
void
HeapBucketT<TBlockType>::DeleteHeapBlockList(TBlockType * list, Recycler * recycler)
{
    HeapBlockList::ForEachEditing(list, [recycler](TBlockType * heapBlock)
    {
#if DBG
        heapBlock->ReleasePagesShutdown(recycler);
#endif
        TBlockType::Delete(heapBlock);
    });
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::DeleteEmptyHeapBlockList(TBlockType * list)
{
    HeapBlockList::ForEachEditing(list, [](TBlockType * heapBlock)
    {
        TBlockType::Delete(heapBlock);
    });
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::DeleteHeapBlockList(TBlockType * list)
{
    DeleteHeapBlockList(list, this->heapInfo->recycler);
}

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
template<typename TBlockType>
bool
HeapBucketT<TBlockType>::PushHeapBlockToSList(PSLIST_HEADER list, TBlockType * heapBlock)
{
    Assert(list != nullptr);
    HeapBlockSListItem<TBlockType> * currentBlock = (HeapBlockSListItem<TBlockType> *) _aligned_malloc(sizeof(HeapBlockSListItem<TBlockType>), MEMORY_ALLOCATION_ALIGNMENT);
    if (currentBlock == nullptr)
    {
        return false;
    }

    currentBlock->itemHeapBlock = heapBlock;
    ::InterlockedPushEntrySList(list, &(currentBlock->itemEntry));
    return true;
}

template<typename TBlockType>
TBlockType *
HeapBucketT<TBlockType>::PopHeapBlockFromSList(PSLIST_HEADER list)
{
    Assert(list != nullptr);
    TBlockType * heapBlock = nullptr;

    PSLIST_ENTRY top = ::InterlockedPopEntrySList(list);
    if (top != nullptr)
    {
        HeapBlockSListItem<TBlockType> * topItem = (HeapBlockSListItem<TBlockType> *) top;
        heapBlock = topItem->itemHeapBlock;
        Assert(heapBlock != nullptr);
        _aligned_free(top);
    }

    return heapBlock;
}

template<typename TBlockType>
ushort
HeapBucketT<TBlockType>::QueryDepthInterlockedSList(PSLIST_HEADER list)
{
    Assert(list != nullptr);
    return ::QueryDepthSList(list);
}

template<typename TBlockType>
void
HeapBucketT<TBlockType>::FlushInterlockedSList(PSLIST_HEADER list)
{
    Assert(list != nullptr);
    if (::QueryDepthSList(list) > 0)
    {
        PSLIST_ENTRY listEntry = ::InterlockedPopEntrySList(list);
        while (listEntry != nullptr)
        {
            _aligned_free(listEntry);
            listEntry = ::InterlockedPopEntrySList(list);
        }
    }

    ::InterlockedFlushSList(list);
}
#endif

template <typename TBlockType>
void
HeapBucketT<TBlockType>::Initialize(HeapInfo * heapInfo, uint sizeCat)
{
    this->heapInfo = heapInfo;
#ifdef RECYCLER_PAGE_HEAP
    this->isPageHeapEnabled = heapInfo->IsPageHeapEnabledForBlock<typename TBlockType::HeapBlockAttributes>(sizeCat);
#endif
    this->sizeCat = sizeCat;
    allocatorHead.Initialize();
#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY)
    allocatorHead.bucket = this;
#endif
    this->lastExplicitFreeListAllocator = &allocatorHead;

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        if (allocableHeapBlockListHead == nullptr)
        {
            heapInfo->recycler->OutOfMemory();
        }
        else
        {
            ::InitializeSListHead(allocableHeapBlockListHead);
        }
    }
#endif
}

template <typename TBlockType>
template <class Fn>
void
HeapBucketT<TBlockType>::ForEachAllocator(Fn fn)
{
    TBlockAllocatorType * current = &allocatorHead;
    do
    {
        fn(current);
        current = current->GetNext();
    }
    while (current != &allocatorHead);
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::UpdateAllocators()
{
    ForEachAllocator([](TBlockAllocatorType * allocator) { allocator->UpdateHeapBlock(); });
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::ClearAllocators()
{
    ForEachAllocator([](TBlockAllocatorType * allocator) { ClearAllocator(allocator); });

#ifdef RECYCLER_PAGE_HEAP

#endif

#ifdef RECYCLER_MEMORY_VERIFY
    FreeObject* freeObject = this->explicitFreeList;

    while (freeObject)
    {
        HeapBlock* heapBlock = this->GetRecycler()->FindHeapBlock((void*)freeObject);
        Assert(heapBlock != nullptr);
        Assert(!heapBlock->IsLargeHeapBlock());
        TBlockType* smallBlock = (TBlockType*)heapBlock;

        smallBlock->ClearExplicitFreeBitForObject((void*)freeObject);
        freeObject = freeObject->GetNext();
    }
#endif

    this->explicitFreeList = nullptr;
}

#if ENABLE_CONCURRENT_GC
template <typename TBlockType>
void
HeapBucketT<TBlockType>::PrepareSweep()
{
    // CONCURRENT-TODO: Technically, We don't really need to invalidate allocators here,
    // but currently invalidating may update the unallocateCount which is
    // used to calculate the partial heuristics, so it needs to be done
    // before sweep. When the partial heuristic changes, we can remove this
    // (And remove rescan from leaf bucket, so this function doesn't need to exist)
    ClearAllocators();
}
#endif

template <typename TBlockType>
void
HeapBucketT<TBlockType>::AddAllocator(TBlockAllocatorType * allocator)
{
    Assert(allocator != &this->allocatorHead);
    allocator->Initialize();
    allocator->next = this->allocatorHead.next;
    allocator->prev = &this->allocatorHead;
    allocator->next->prev = allocator;
    this->allocatorHead.next = allocator;
#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY)
    allocator->bucket = this;
#endif
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::RemoveAllocator(TBlockAllocatorType * allocator)
{
    Assert(allocator != &this->allocatorHead);
    ClearAllocator(allocator);

    allocator->next->prev = allocator->prev;
    allocator->prev->next = allocator->next;

    if (allocator == this->lastExplicitFreeListAllocator)
    {
        this->lastExplicitFreeListAllocator = &allocatorHead;
    }
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::ClearAllocator(TBlockAllocatorType * allocator)
{
    allocator->Clear();
}

template <typename TBlockType>
bool
HeapBucketT<TBlockType>::IntegrateBlock(char * blockAddress, PageSegment * segment, Recycler * recycler)
{
    // Add a new heap block
    TBlockType * heapBlock = GetUnusedHeapBlock();
    if (heapBlock == nullptr)
    {
        return false;
    }

    // TODO: Consider supporting guard pages for this codepath
    if (!heapBlock->SetPage(blockAddress, segment, recycler))
    {
        FreeHeapBlock(heapBlock);
        return false;
    }

    heapBlock->SetNextBlock(this->fullBlockList);
    this->fullBlockList = heapBlock;
    RECYCLER_SLOW_CHECK(this->heapBlockCount++);

    this->heapInfo->uncollectedAllocBytes += heapBlock->GetAndClearLastFreeCount() * heapBlock->GetObjectSize();
    RecyclerMemoryTracking::ReportAllocation(recycler, blockAddress, heapBlock->GetObjectSize() * heapBlock->GetObjectCount());
    RECYCLER_PERF_COUNTER_ADD(LiveObject,heapBlock->GetObjectCount());
    RECYCLER_PERF_COUNTER_ADD(LiveObjectSize, heapBlock->GetObjectSize() * heapBlock->GetObjectCount());

    if (heapBlock->IsLargeHeapBlock())
    {
        RECYCLER_PERF_COUNTER_ADD(LargeHeapBlockLiveObject,heapBlock->GetObjectCount());
        RECYCLER_PERF_COUNTER_ADD(LargeHeapBlockLiveObjectSize, heapBlock->GetObjectSize() * heapBlock->GetObjectCount());
    }
    else
    {
        RECYCLER_PERF_COUNTER_ADD(SmallHeapBlockLiveObject,heapBlock->GetObjectCount());
        RECYCLER_PERF_COUNTER_ADD(SmallHeapBlockLiveObjectSize, heapBlock->GetObjectSize() * heapBlock->GetObjectCount());
    }

#if DBG
    heapBlock->SetIsIntegratedBlock();
#endif

    return true;
}

#if DBG
template <typename TBlockType>
bool
HeapBucketT<TBlockType>::AllocatorsAreEmpty() const
{
    TBlockAllocatorType const * current = &allocatorHead;
    do
    {
        if (current->GetHeapBlock() != nullptr || current->GetExplicitFreeList() != nullptr)
        {
            return false;
        }
        current = current->GetNext();
    }
    while (current != &allocatorHead);
    return true;
}

template <typename TBlockType>
bool
HeapBucketT<TBlockType>::HasPendingDisposeHeapBlocks() const
{
#ifdef RECYCLER_WRITE_BARRIER
    return (IsFinalizableBucket || IsFinalizableWriteBarrierBucket) &&
    ((SmallFinalizableHeapBucketT<typename TBlockType::HeapBlockAttributes> *)this)->pendingDisposeList != nullptr;
#else
    return IsFinalizableBucket && ((SmallFinalizableHeapBucketT<typename TBlockType::HeapBlockAttributes> *)this)->pendingDisposeList != nullptr;
#endif
}
#endif

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
template <typename TBlockType>
size_t
HeapBucketT<TBlockType>::GetNonEmptyHeapBlockCount(bool checkCount) const
{
    size_t currentHeapBlockCount = HeapBlockList::Count(fullBlockList);
    currentHeapBlockCount += HeapBlockList::Count(heapBlockList);
    bool allocatingDuringConcurrentSweep = false;

#if ENABLE_CONCURRENT_GC
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        allocatingDuringConcurrentSweep = true;
        // This lock is needed only in the debug mode while we verify block counts. Not needed otherwise, as this list is never accessed concurrently.
        // Items are added to it by the allocator when allocations are allowed during concurrent sweep. The list is drained during the next sweep while
        // allocation are stopped.
        debugSweepableHeapBlockListLock.Enter();
        currentHeapBlockCount += QueryDepthInterlockedSList(allocableHeapBlockListHead);
        currentHeapBlockCount += HeapBlockList::Count(sweepableHeapBlockList);
        debugSweepableHeapBlockListLock.Leave();
    }
#endif
#endif

    // Recycler can be null if we have OOM in the ctor
    if (this->GetRecycler() && this->GetRecycler()->recyclerSweep != nullptr)
    {
        currentHeapBlockCount += this->GetRecycler()->recyclerSweep->GetHeapBlockCount(this);
    }
#endif

    // There is no way to determine the number of item in an SLIST if there are >= 65535 items in the list.
    RECYCLER_SLOW_CHECK(Assert(!checkCount || heapBlockCount == currentHeapBlockCount || allocatingDuringConcurrentSweep));

    return currentHeapBlockCount;
}

template <typename TBlockType>
size_t
HeapBucketT<TBlockType>::GetEmptyHeapBlockCount() const
{
    size_t count = HeapBlockList::Count(this->emptyBlockList);
    RECYCLER_SLOW_CHECK(Assert(count == this->emptyHeapBlockCount));
    return count;
}
#endif

template <typename TBlockType>
char *
HeapBucketT<TBlockType>::TryAlloc(Recycler * recycler, TBlockAllocatorType * allocator, size_t sizeCat, ObjectInfoBits attributes)
{
    AUTO_NO_EXCEPTION_REGION;

    Assert((attributes & InternalObjectInfoBitMask) == attributes);

    ClearAllocator(allocator);

    TBlockType * heapBlock = this->nextAllocableBlockHead;
#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP 
#if SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    bool heapBlockInSweepableList = false;
    if (heapBlock == nullptr && CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        // This lock is needed only in the debug mode while we verify block counts. Not needed otherwise, as this list is never accessed concurrently.
        // Items are added to it by the allocator when allocations are allowed during concurrent sweep. The list is drained during the next sweep while
        // allocation are stopped.
        debugSweepableHeapBlockListLock.Enter();
#endif
        heapBlock = PopHeapBlockFromSList(this->allocableHeapBlockListHead);
        if (heapBlock != nullptr)
        {
            // Put the block in the sweepable heap block list so we don't lose track of it. The block will eventually be moved to the 
            // heapBlockList or fullBlockList as appropriate during the next sweep.
            heapBlock->SetNextBlock(sweepableHeapBlockList);
            sweepableHeapBlockList = heapBlock;
            heapBlockInSweepableList = true;
        }
#if DBG|| defined(RECYCLER_SLOW_CHECK_ENABLED)
        debugSweepableHeapBlockListLock.Leave();
#endif
    }
#endif
#endif

   if (heapBlock != nullptr)
   {
        Assert(!this->IsAllocationStopped());

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
        // When allocations are allowed during concurrent sweep we set nextAllocableBlockHead to NULL as the allocator will pick heap blocks from the
        // interlocked SLIST. During that time, the heap block at the top of the SLIST is always the nextAllocableBlockHead.
        // If the heapBlock was just picked from the SLIST and nextAllocableBlockHead is not NULL then we just resumed normal allocations on the background thread
        // while finishing the concurrent sweep, and the nextAllocableBlockHead is already set properly.
        if (this->nextAllocableBlockHead != nullptr && !heapBlockInSweepableList)
#endif
        {
            this->nextAllocableBlockHead = heapBlock->GetNextBlock();
        }

        allocator->Set(heapBlock);
    }
    else if (this->explicitFreeList != nullptr)
    {
        allocator->SetExplicitFreeList(this->explicitFreeList);
        this->lastExplicitFreeListAllocator = allocator;
        this->explicitFreeList = nullptr;
    }
    else
    {
        return nullptr;
    }
    // We just found a block we can allocate on
    char * memBlock = allocator->template SlowAlloc<false /* disallow fault injection */>(recycler, sizeCat, attributes);
    Assert(memBlock != nullptr);
    return memBlock;
}

template <typename TBlockType>
char *
HeapBucketT<TBlockType>::TryAllocFromNewHeapBlock(Recycler * recycler, TBlockAllocatorType * allocator, size_t sizeCat, size_t size, ObjectInfoBits attributes)
{
    AUTO_NO_EXCEPTION_REGION;

    Assert((attributes & InternalObjectInfoBitMask) == attributes);

#ifdef RECYCLER_PAGE_HEAP
    if (IsPageHeapEnabled(attributes))
    {
        return this->PageHeapAlloc(recycler, sizeCat, size, attributes, this->heapInfo->pageHeapMode, true);
    }
#endif

    TBlockType * heapBlock = CreateHeapBlock(recycler);
    if (heapBlock == nullptr)
    {
        return nullptr;
    }

    // new heap block added, allocate from that.
    allocator->SetNew(heapBlock);
    // We just created a block we can allocate on
    char * memBlock = allocator->template SlowAlloc<false /* disallow fault injection */>(recycler, sizeCat, attributes);
    Assert(memBlock != nullptr || IS_FAULTINJECT_NO_THROW_ON);
    return memBlock;
}

Recycler *
HeapBucket::GetRecycler() const
{
    return this->heapInfo->recycler;
}

#ifdef RECYCLER_PAGE_HEAP
template <typename TBlockType>
char *
HeapBucketT<TBlockType>::PageHeapAlloc(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, size_t size, ObjectInfoBits attributes, PageHeapMode mode, bool nothrow)
{
    AllocationVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("In PageHeapAlloc [Size: 0x%x, Attributes: 0x%x]\n"), size, attributes);
    char* addr =  heapInfo->largeObjectBucket.PageHeapAlloc(recycler, sizeCat, size, attributes, mode, nothrow);

    if (addr)
    {
        this->GetRecycler()->autoHeap.uncollectedAllocBytes += sizeCat;
    }

    return addr;
}
#endif

template <typename TBlockType>
char *
HeapBucketT<TBlockType>::SnailAlloc(Recycler * recycler, TBlockAllocatorType * allocator, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, size_t size, ObjectInfoBits attributes, bool nothrow)
{
    AllocationVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("In SnailAlloc [Size: 0x%x, Attributes: 0x%x]\n"), sizeCat, attributes);

    Assert(sizeCat == this->sizeCat);
    Assert((attributes & InternalObjectInfoBitMask) == attributes);

    char * memBlock = this->TryAlloc(recycler, allocator, sizeCat, attributes);
    if (memBlock != nullptr)
    {
        return memBlock;
    }

#if ENABLE_CONCURRENT_GC
    // No free memory, try to collect with allocated bytes and time heuristic, and concurrently
    BOOL collected = recycler->disableCollectOnAllocationHeuristics ? recycler->FinishConcurrent<FinishConcurrentOnAllocation>() :
        recycler->CollectNow<CollectOnAllocation>();
#else
    BOOL collected = recycler->disableCollectOnAllocationHeuristics ? FALSE : recycler->CollectNow<CollectOnAllocation>();
#endif

    AllocationVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("TryAlloc failed, forced collection on allocation [Collected: %d]\n"), collected);
    if (!collected)
    {
#if ENABLE_CONCURRENT_GC
        // wait for background sweeping finish if there are too many pages allocated during background sweeping
#if ENABLE_PARTIAL_GC
        if (recycler->IsConcurrentSweepExecutingState() && this->heapInfo->uncollectedNewPageCount > (uint)CONFIG_FLAG(NewPagesCapDuringBGSweeping))
#else
        if (recycler->IsConcurrentSweepExecutingState())
#endif
        {
            recycler->FinishConcurrent<ForceFinishCollection>();
            memBlock = this->TryAlloc(recycler, allocator, sizeCat, attributes);
            if (memBlock != nullptr)
            {
                return memBlock;
            }
        }
#endif
        // We didn't collect, try to add a new heap block
        memBlock = TryAllocFromNewHeapBlock(recycler, allocator, sizeCat, size, attributes);
        if (memBlock != nullptr)
        {
            return memBlock;
        }

        // Can't even allocate a new block, we need force a collection and
        //allocate some free memory, add a new heap block again, or throw out of memory
        AllocationVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("TryAllocFromNewHeapBlock failed, forcing in-thread collection\n"));
        recycler->CollectNow<CollectNowForceInThread>();
    }

    // Collection might trigger finalizer, which might allocate memory. So the allocator
    // might have a heap block already, try to allocate from that first
    memBlock = allocator->template SlowAlloc<true /* allow fault injection */>(recycler, sizeCat, attributes);
    if (memBlock != nullptr)
    {
        return memBlock;
    }

    AllocationVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("SlowAlloc failed\n"));

    // do the allocation
    memBlock = this->TryAlloc(recycler, allocator, sizeCat, attributes);
    if (memBlock != nullptr)
    {
        return memBlock;
    }

    AllocationVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("TryAlloc failed\n"));
    // add a heap block if there are no preallocated memory left.
    memBlock = TryAllocFromNewHeapBlock(recycler, allocator, sizeCat, size, attributes);
    if (memBlock != nullptr)
    {
        return memBlock;
    }

    AllocationVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("TryAllocFromNewHeapBlock failed- triggering OOM handler"));

    if (nothrow == false)
    {
        // Can't add a heap block, we are out of memory
        // Since we're allowed to throw, throw right here
        recycler->OutOfMemory();
    }

    return nullptr;
}

template <typename TBlockType>
TBlockType*
HeapBucketT<TBlockType>::GetUnusedHeapBlock()
{
    // Add a new heap block
    TBlockType * heapBlock = emptyBlockList;
    if (heapBlock == nullptr)
    {
        // We couldn't find a reusable heap block
        heapBlock = TBlockType::New(this);
        RECYCLER_SLOW_CHECK(Assert(this->emptyHeapBlockCount == 0));
    }
    else
    {
        emptyBlockList = heapBlock->GetNextBlock();
        RECYCLER_SLOW_CHECK(this->emptyHeapBlockCount--);
    }
    return heapBlock;
}

template <typename TBlockType>
TBlockType *
HeapBucketT<TBlockType>::CreateHeapBlock(Recycler * recycler)
{
    FAULTINJECT_MEMORY_NOTHROW(_u("HeapBlock"), sizeof(TBlockType));

    // Add a new heap block
    TBlockType * heapBlock = GetUnusedHeapBlock();
    if (heapBlock == nullptr)
    {
        return nullptr;
    }

    if (!heapBlock->ReassignPages(recycler))
    {
        FreeHeapBlock(heapBlock);
        return nullptr;
    }

    // Add it to head of heap block list so we will keep track of the block
    recycler->autoHeap.AppendNewHeapBlock(heapBlock, this);
#ifdef RECYCLER_SLOW_CHECK_ENABLED
#if ENABLE_CONCURRENT_GC
    ::InterlockedIncrement(&this->newHeapBlockCount);
#else
    this->heapBlockCount++;
#endif
#endif
    return heapBlock;
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::FreeHeapBlock(TBlockType * heapBlock)
{
    heapBlock->Reset();
    heapBlock->SetNextBlock(emptyBlockList);
    emptyBlockList = heapBlock;
    RECYCLER_SLOW_CHECK(this->emptyHeapBlockCount++);
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::ResetMarks(ResetMarkFlags flags)
{
    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount((flags & ResetMarkFlags_Background) != 0));

#if !ENABLE_CONCURRENT_GC
    Assert((flags & ResetMarkFlags_Background) == 0);
#endif

    if ((flags & ResetMarkFlags_Background) == 0)
    {
        // The is equivalent to the ClearAllocators in Rescan
        // But since we are not doing concurrent, we need to do it here.
        ClearAllocators();
    }

    // Note, mark bits are now cleared in HeapBlockMap32::ResetMarks, so we don't need to clear them here.

    if ((flags & ResetMarkFlags_ScanImplicitRoot) != 0)
    {
        HeapBlockList::ForEach(fullBlockList, [flags](TBlockType * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
            Assert(!heapBlock->HasFreeObject());
        });

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
        if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
        {
            HeapBlockList::ForEach(sweepableHeapBlockList, [flags](TBlockType * heapBlock)
            {
                heapBlock->MarkImplicitRoots();
            });
        }
#endif

        HeapBlockList::ForEach(heapBlockList, [flags](TBlockType * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });
    }

#if DBG
    if ((flags & ResetMarkFlags_Background) == 0)
    {
        // When allocations are enabled for buckets during oncurrent sweep we don't keep track of the nextAllocableBlockHead as it directly
        // comes out of the SLIST. As a result, the below validations can't be performed reliably on a heap block.
#if !(ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP)
        // Verify that if you are in the heapBlockList, before the nextAllocableBlockHead, we have fully allocated from
        // the block already, except if we have cleared from the allocator, or it is still in the allocator
        HeapBlockList::ForEach(heapBlockList, nextAllocableBlockHead, [](TBlockType * heapBlock)
        {
            // If the heap block is in the allocator, then the heap block may or may not have free object still
            // So we can't assert.  Otherwise, we have free object iff we were cleared from allocator
            Assert(heapBlock->IsInAllocator() || heapBlock->HasFreeObject() == heapBlock->IsClearedFromAllocator());
        });

        // We should still have allocable free object after nextAllocableBlockHead
        HeapBlockList::ForEach(nextAllocableBlockHead, [](TBlockType * heapBlock)
        {
            Assert(heapBlock->HasFreeObject());
        });
#endif
    }
#endif
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::ScanNewImplicitRoots(Recycler * recycler)
{
    HeapBlockList::ForEach(fullBlockList, [recycler](TBlockType * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        HeapBlockList::ForEach(sweepableHeapBlockList, [recycler](TBlockType * heapBlock)
        {
            heapBlock->ScanNewImplicitRoots(recycler);
        });
    }
#endif

    HeapBlockList::ForEach(heapBlockList, [recycler](TBlockType * heapBlock)
    {
        heapBlock->ScanNewImplicitRoots(recycler);
    });
}

#if DBG
template <typename TBlockType>
void
HeapBucketT<TBlockType>::VerifyBlockConsistencyInList(TBlockType * heapBlock, RecyclerVerifyListConsistencyData& recyclerSweep)
{
    bool* expectFull = nullptr;
    bool* expectDispose = nullptr;
    HeapBlock* nextAllocableBlockHead = nullptr;

    if (TBlockType::HeapBlockAttributes::IsSmallBlock)
    {
        expectFull = &recyclerSweep.smallBlockVerifyListConsistencyData.expectFull;
        expectDispose = &recyclerSweep.smallBlockVerifyListConsistencyData.expectDispose;
        nextAllocableBlockHead = recyclerSweep.smallBlockVerifyListConsistencyData.nextAllocableBlockHead;
    }
    else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
    {
        expectFull = &recyclerSweep.mediumBlockVerifyListConsistencyData.expectFull;
        expectDispose = &recyclerSweep.mediumBlockVerifyListConsistencyData.expectDispose;
        nextAllocableBlockHead = recyclerSweep.mediumBlockVerifyListConsistencyData.nextAllocableBlockHead;
    }
    else
    {
        Assert(false);
    }

    if (heapBlock == nextAllocableBlockHead)
    {
        (*expectFull) = false;
    }
    if (heapBlock->IsClearedFromAllocator())
    {
        // As the blocks are added to a stack and used from there during concurrent sweep, the exepectFull assertion doesn't hold anymore. 
        // We could do some work to make this work again but there may be perf hit and it may be fragile.
#if !(ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP)
        Assert(*expectFull && !*expectDispose);
        Assert(heapBlock->HasFreeObject());
        Assert(!heapBlock->HasAnyDisposeObjects());
#endif
    }
    else if (*expectDispose)
    {
        Assert(heapBlock->IsAnyFinalizableBlock() && heapBlock->template AsFinalizableBlock<typename TBlockType::HeapBlockAttributes>()->IsPendingDispose());
        Assert(heapBlock->HasAnyDisposeObjects());
    }
    else
    {
        Assert(!heapBlock->HasAnyDisposeObjects());

        // ExpectFull is a bit of a misnomer if the list in question is the heap block list. It's there to check
        // of the heap block in question is before the nextAllocableBlockHead or not. This is to ensure that
        // blocks before nextAllocableBlockHead that are not being bump allocated from must be considered "full".
        // However, the exception is if this is the only heap block in this bucket, in which case nextAllocableBlockHead
        // would be null
#if !(ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP)
        // As the blocks are added to the SLIST and used from there during concurrent sweep, the exepectFull assertion doesn't hold anymore.
        Assert(*expectFull == (!heapBlock->HasFreeObject() || heapBlock->IsInAllocator()) || nextAllocableBlockHead == nullptr);
#endif
    }
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::VerifyBlockConsistencyInList(TBlockType * heapBlock, RecyclerVerifyListConsistencyData const& recyclerSweep, SweepState state)
{
    bool expectFull = false;
    bool expectDispose = false;

    if (TBlockType::HeapBlockAttributes::IsSmallBlock)
    {
        expectFull = recyclerSweep.smallBlockVerifyListConsistencyData.expectFull;
        expectDispose = recyclerSweep.smallBlockVerifyListConsistencyData.expectDispose;
    }
    else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
    {
        expectFull = recyclerSweep.mediumBlockVerifyListConsistencyData.expectFull;
        expectDispose = recyclerSweep.mediumBlockVerifyListConsistencyData.expectDispose;
    }
    else
    {
        Assert(false);
    }

    if (heapBlock->IsClearedFromAllocator())
    {
        // this function is called during sweep and we are recreating the heap block list
        // which would make all the block to be in it's rightful place
        heapBlock->SetIsClearedFromAllocator(false);

        Assert(SweepStateFull != state);
    }
    else
    {
        // You can still be full only if you are full before.
        Assert(expectFull || SweepStateFull != state);
    }

    // If you were pending dispose before, you can only be pending dispose after
    Assert(!expectDispose || SweepStatePendingDispose == state);
}
#endif // DBG

#if ENABLE_PARTIAL_GC
template <typename TBlockType>
bool
HeapBucketT<TBlockType>::DoQueuePendingSweep(Recycler * recycler)
{
    return IsNormalBucket && recycler->inPartialCollectMode;
}

template <typename TBlockType>
bool
HeapBucketT<TBlockType>::DoPartialReuseSweep(Recycler * recycler)
{
    // With leaf, we don't need to do a partial sweep
    // WriteBarrier-TODO: We shouldn't need to do this for write barrier heap buckets either
    return !IsLeafBucket && recycler->inPartialCollectMode;
}
#endif

template <typename TBlockType>
void
HeapBucketT<TBlockType>::SweepHeapBlockList(RecyclerSweep& recyclerSweep, TBlockType * heapBlockList, bool allocable)
{
#if DBG
    if (TBlockType::HeapBlockAttributes::IsSmallBlock)
    {
        Assert(recyclerSweep.smallBlockVerifyListConsistencyData.hasSetupVerifyListConsistencyData);
        recyclerSweep.smallBlockVerifyListConsistencyData.hasSetupVerifyListConsistencyData = false;
    }
    else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
    {
        Assert(recyclerSweep.mediumBlockVerifyListConsistencyData.hasSetupVerifyListConsistencyData);
        recyclerSweep.mediumBlockVerifyListConsistencyData.hasSetupVerifyListConsistencyData = false;
    }
    else
    {
        Assert(false);
    }
#endif

    Recycler * recycler = recyclerSweep.GetRecycler();

    // Whether we run in thread or background thread, we want to queue up pending sweep
    // only if we are doing partial GC so we can calculate the heuristics before
    // determinate we want to fully sweep the block or partially sweep the block

#if ENABLE_PARTIAL_GC
    // CONCURRENT-TODO: Add a mode where we can do in thread sweep, and concurrent partial sweep?
    bool const queuePendingSweep = this->DoQueuePendingSweep(recycler);
#else
    bool const queuePendingSweep = false;
#endif

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    Assert(this->IsAllocationStopped() || this->AllocationsStartedDuringConcurrentSweep());
#else
    Assert(this->IsAllocationStopped());
#endif

    HeapBlockList::ForEachEditing(heapBlockList, [=, &recyclerSweep](TBlockType * heapBlock)
    {
        // The whole list need to be consistent
        DebugOnly(VerifyBlockConsistencyInList(heapBlock, recyclerSweep));

        SweepState state = heapBlock->Sweep(recyclerSweep, queuePendingSweep, allocable);

        DebugOnly(VerifyBlockConsistencyInList(heapBlock, recyclerSweep, state));

        switch (state)
        {
#if ENABLE_CONCURRENT_GC
        case SweepStatePendingSweep:
        {
            Assert(IsNormalBucket);
            // blocks that have swept object. Queue up the block for concurrent sweep.
            Assert(queuePendingSweep);
            TBlockType *& pendingSweepList = recyclerSweep.GetPendingSweepBlockList(this);
            heapBlock->SetNextBlock(pendingSweepList);
            pendingSweepList = heapBlock;
#if ENABLE_PARTIAL_GC
            recyclerSweep.NotifyAllocableObjects(heapBlock);
#endif
            break;
        }
#endif
        case SweepStatePendingDispose:
        {
            Assert(!recyclerSweep.IsBackground());
#ifdef RECYCLER_WRITE_BARRIER
            Assert(IsFinalizableBucket || IsFinalizableWriteBarrierBucket);
#else
            Assert(IsFinalizableBucket);
#endif

            DebugOnly(heapBlock->template AsFinalizableBlock<typename TBlockType::HeapBlockAttributes>()->SetIsPendingDispose());

            // These are the blocks that have swept finalizable object

            // We already transferred the non finalizable swept objects when we are not doing
            // concurrent collection, so we only need to queue up the blocks that have
            // finalizable objects, so that we can go through and call the dispose, and then
            // transfer the finalizable object back to the free list.
            SmallFinalizableHeapBucketT<typename TBlockType::HeapBlockAttributes> * finalizableHeapBucket = (SmallFinalizableHeapBucketT<typename TBlockType::HeapBlockAttributes>*)this;
            heapBlock->template AsFinalizableBlock<typename TBlockType::HeapBlockAttributes>()->SetNextBlock(finalizableHeapBucket->pendingDisposeList);
            finalizableHeapBucket->pendingDisposeList = heapBlock->template AsFinalizableBlock<typename TBlockType::HeapBlockAttributes>();
            Assert(!recycler->hasPendingTransferDisposedObjects);
            recycler->hasDisposableObject = true;
            break;
        }
        case SweepStateSwept:
        {
            Assert(this->nextAllocableBlockHead == nullptr);
            Assert(heapBlock->HasFreeObject());
            heapBlock->SetNextBlock(this->heapBlockList);
            this->heapBlockList = heapBlock;
#if ENABLE_PARTIAL_GC
            recyclerSweep.NotifyAllocableObjects(heapBlock);
#endif
            break;
        }
        case SweepStateFull:
        {
            Assert(!heapBlock->HasFreeObject());
            heapBlock->SetNextBlock(this->fullBlockList);
            this->fullBlockList = heapBlock;
            break;
        }
        case SweepStateEmpty:
        {
            // the block is empty, just free them
#ifdef RECYCLER_MEMORY_VERIFY
            // Let's verify it before we free it
            if (recycler->VerifyEnabled())
            {
                heapBlock->Verify();
            }
#endif

            RECYCLER_STATS_INC(recycler, numEmptySmallBlocks[heapBlock->GetHeapBlockType()]);

#if ENABLE_CONCURRENT_GC
            // CONCURRENT-TODO: Finalizable block never have background == true and always be processed
            // in thread, so it will not queue up the pages even if we are doing concurrent GC
            if (recyclerSweep.IsBackground())
            {
#ifdef RECYCLER_WRITE_BARRIER
                Assert(!(IsFinalizableBucket || IsFinalizableWriteBarrierBucket));
#else
                Assert(!IsFinalizableBucket);
#endif
                // CONCURRENT-TODO: We will zero heap block even if the number free page pool exceed
                // the maximum and will get decommitted anyway
                recyclerSweep.template QueueEmptyHeapBlock<TBlockType>(this, heapBlock);
                RECYCLER_STATS_INC(recycler, numZeroedOutSmallBlocks);
            }
            else
#endif
            {
                // Just free the page in thread (and zero the page)
                heapBlock->ReleasePagesSweep(recycler);
                FreeHeapBlock(heapBlock);
                RECYCLER_SLOW_CHECK(this->heapBlockCount--);
            }

            break;
        }
        }
    });
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::SweepBucket(RecyclerSweep& recyclerSweep)
{
    DebugOnly(TBlockType * savedNextAllocableBlockHead);
    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(recyclerSweep.IsBackground()));
#if ENABLE_CONCURRENT_GC
    if (recyclerSweep.HasSetupBackgroundSweep())
    {
        // SetupBackgroundSweep set nextAllocableBlockHead to null already
        Assert(IsAllocationStopped());
        DebugOnly(savedNextAllocableBlockHead = recyclerSweep.GetSavedNextAllocableBlockHead(this));
    }
    else
#endif
    {
        Assert(AllocatorsAreEmpty());
        DebugOnly(savedNextAllocableBlockHead = this->nextAllocableBlockHead);
        this->StopAllocationBeforeSweep();
    }

    // We just started sweeping.  These pending lists should be empty
#if ENABLE_CONCURRENT_GC
    Assert(recyclerSweep.GetPendingSweepBlockList(this) == nullptr);
#else
    Assert(!recyclerSweep.IsBackground());
#endif

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        // Return the blocks we may have allocated from during a previous concurrent sweep back to the fullBlockList. We need to rebuild the free bit vectors for these blocks.
        HeapBlockList::ForEachEditing(this->sweepableHeapBlockList, [this](TBlockType * heapBlock)
        {
            heapBlock->BuildFreeBitVector();
            heapBlock->SetNextBlock(this->fullBlockList);
            this->fullBlockList = heapBlock;
        });

        this->sweepableHeapBlockList = nullptr;
    }
#endif

#if DBG
    if (TBlockType::HeapBlockAttributes::IsSmallBlock)
    {
        recyclerSweep.SetupVerifyListConsistencyDataForSmallBlock((SmallHeapBlock*) savedNextAllocableBlockHead, true, false);
    }
    else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
    {
        recyclerSweep.SetupVerifyListConsistencyDataForMediumBlock((MediumHeapBlock*) savedNextAllocableBlockHead, true, false);
    }
    else
    {
        Assert(false);
    }
#endif

    // Move the list locally.  We will relink them during sweep
    TBlockType * currentFullBlockList = fullBlockList;
    TBlockType * currentHeapBlockList = heapBlockList;
    this->heapBlockList = nullptr;
    this->fullBlockList = nullptr;

    this->SweepHeapBlockList(recyclerSweep, currentHeapBlockList, true);

#if DBG
    if (TBlockType::HeapBlockAttributes::IsSmallBlock)
    {
        recyclerSweep.SetupVerifyListConsistencyDataForSmallBlock(nullptr, true, false);
    }
    else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
    {
        recyclerSweep.SetupVerifyListConsistencyDataForMediumBlock(nullptr, true, false);
    }
    else
    {
        Assert(false);
    }
#endif

    this->SweepHeapBlockList(recyclerSweep, currentFullBlockList, false);

    // We shouldn't have allocate from any block yet
    Assert(this->nextAllocableBlockHead == nullptr);
}

template <typename TBlockType>
bool
HeapBucketT<TBlockType>::AllowAllocationsDuringConcurrentSweep()
{
#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    if (!CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        return false;
    }

    Recycler * recycler = this->GetRecycler();
#if ENABLE_PARTIAL_GC
    bool isPartialGC = (recycler->recyclerSweep != nullptr) && recycler->recyclerSweep->InPartialCollect();
#else
    bool isPartialGC = false;
#endif

    // Allocations are allowed during concurrent sweep for small non-finalizable buckets while not doing a Partial GC.
    return (recycler->IsConcurrentSweepSetupState() || recycler->InConcurrentSweep()) && !this->IsAnyFinalizableBucket() && !isPartialGC;
#else
    return false;
#endif
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::StopAllocationBeforeSweep()
{
#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    this->allocationsStartedDuringConcurrentSweep = false;
#endif

    Assert(!this->IsAllocationStopped());
    this->isAllocationStopped = true;
    this->nextAllocableBlockHead = nullptr;
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::StartAllocationAfterSweep()
{
    Assert(this->IsAllocationStopped());
    this->isAllocationStopped = false;
    this->nextAllocableBlockHead = this->heapBlockList;
}

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
template <typename TBlockType>
void
HeapBucketT<TBlockType>::StartAllocationDuringConcurrentSweep()
{
    Assert(this->IsAllocationStopped());
    this->isAllocationStopped = false;
    Assert(!this->allocationsStartedDuringConcurrentSweep);
    this->allocationsStartedDuringConcurrentSweep = true;

#if SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    // When allocations are allowed during concurrent sweep we set nextAllocableBlockHead to NULL as the allocator will pick heap blocks from the
    // interlocked SLIST. During that time, the heap block at the top of the SLIST is always the nextAllocableBlockHead.
    this->nextAllocableBlockHead = nullptr;
#endif
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::ResumeNormalAllocationAfterConcurrentSweep(TBlockType * newNextAllocableBlockHead)
{
    this->allocationsStartedDuringConcurrentSweep = false;
    this->isAllocationStopped = false;
    // If the newNextAllocableBlockHead is NULL at this point that means we have exhausted usable blocks and will have to allocate a new block the next time.
    this->nextAllocableBlockHead = newNextAllocableBlockHead;
}

template <typename TBlockType>
bool
HeapBucketT<TBlockType>::AllocationsStartedDuringConcurrentSweep() const
{
    return this->allocationsStartedDuringConcurrentSweep;
}
#endif

template <typename TBlockType>
bool
HeapBucketT<TBlockType>::IsAllocationStopped() const
{
    if (this->isAllocationStopped)
    {
        Assert(this->nextAllocableBlockHead == nullptr);
        return true;
    }
    return false;
}

template <typename TBlockType>
uint
HeapBucketT<TBlockType>::Rescan(Recycler * recycler, RescanFlags flags)
{
#if ENABLE_CONCURRENT_GC
    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(!!recycler->IsConcurrentMarkState()));
#else
    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(false /* background */));
#endif

#if ENABLE_CONCURRENT_GC
    // If we do the final rescan concurrently, the main thread will prepare for sweep concurrently
    // If we do rescan in thread, we will need to prepare sweep here.
    // However, if we are in the rescan for OOM, we have already done it, so no need to do it again
    if (!recycler->IsConcurrentMarkState() && !recycler->inEndMarkOnLowMemory)
    {
        this->PrepareSweep();
    }
#endif

    // By default heap bucket doesn't rescan anything
    return 0;
}

#if ENABLE_CONCURRENT_GC
template <typename TBlockType>
void
HeapBucketT<TBlockType>::MergeNewHeapBlock(TBlockType * heapBlock)
{
    Assert(heapBlock->GetObjectSize() == this->sizeCat);
    heapBlock->SetNextBlock(this->heapBlockList);
    this->heapBlockList = heapBlock;
    RECYCLER_SLOW_CHECK(::InterlockedDecrement(&this->newHeapBlockCount));
    RECYCLER_SLOW_CHECK(this->heapBlockCount++);
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::SetupBackgroundSweep(RecyclerSweep& recyclerSweep)
{
    // Don't allocate from existing block temporary when concurrent sweeping

    // Currently Rescan clear allocators, if we remove the uncollectedAllocBytes there, we can
    // avoid it there and do it here.
    Assert(this->AllocatorsAreEmpty());

    DebugOnly(recyclerSweep.SaveNextAllocableBlockHead(this));
    Assert(recyclerSweep.GetPendingSweepBlockList(this) == nullptr);

    this->StopAllocationBeforeSweep();
}
#endif

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
template <typename TBlockType>
void
HeapBucketT<TBlockType>::FinishConcurrentSweep()
{
#if SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    Assert(this->allocableHeapBlockListHead != nullptr);

    TBlockType * newNextAllocableBlockHead = nullptr;
    // Put the blocks from the allocable SLIST into the heapBlockList.
    TBlockType * heapBlock = PopHeapBlockFromSList(this->allocableHeapBlockListHead);
    while (heapBlock != nullptr)
    {
        newNextAllocableBlockHead = heapBlock;
        heapBlock->SetNextBlock(this->heapBlockList);
        this->heapBlockList = heapBlock;
        heapBlock = PopHeapBlockFromSList(this->allocableHeapBlockListHead);
    }

    Assert(QueryDepthInterlockedSList(this->allocableHeapBlockListHead) == 0);

    this->ResumeNormalAllocationAfterConcurrentSweep(newNextAllocableBlockHead);
#endif

    Assert(!this->IsAllocationStopped());
}
#endif

template <typename TBlockType>
void
HeapBucketT<TBlockType>::AppendAllocableHeapBlockList(TBlockType * list)
{
    // Add the list to the end of the current list
    TBlockType * currentHeapBlockList = this->heapBlockList;
    if (currentHeapBlockList == nullptr)
    {
        // There weren't any heap block list before, just move the list over and start allocate from it
        this->heapBlockList = list;
        this->nextAllocableBlockHead = list;
    }
    else
    {
        // Find the last block and append the pendingSwpetList
        TBlockType * tail = HeapBlockList::Tail(currentHeapBlockList);
        Assert(tail != nullptr);
        tail->SetNextBlock(list);

        // If we are not currently allocating from the existing heapBlockList,
        // that means fill all the exiting one already, we should start with what we just appended.
        if (this->nextAllocableBlockHead == nullptr)
        {
            this->nextAllocableBlockHead = list;
        }
    }
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size))
{
    UpdateAllocators();
    HeapBucket::EnumerateObjects(fullBlockList, infoBits, CallBackFunction);
#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        HeapBucket::EnumerateObjects(sweepableHeapBlockList, infoBits, CallBackFunction);
    }
#endif
    HeapBucket::EnumerateObjects(heapBlockList, infoBits, CallBackFunction);
}

#ifdef RECYCLER_SLOW_CHECK_ENABLED
template <typename TBlockType>
void
HeapBucketT<TBlockType>::VerifyHeapBlockCount(bool background)
{
    // TODO-REFACTOR: GetNonEmptyHeapBlockCount really should be virtual
    static_cast<typename SmallHeapBlockType<TBlockType::RequiredAttributes, typename TBlockType::HeapBlockAttributes>::BucketType *>(this)->GetNonEmptyHeapBlockCount(true);
    if (!background)
    {
        this->GetEmptyHeapBlockCount();
    }
}

template <typename TBlockType>
size_t
HeapBucketT<TBlockType>::Check(bool checkCount)
{
    Assert(this->GetRecycler()->recyclerSweep == nullptr);
    UpdateAllocators();
    size_t smallHeapBlockCount = HeapInfo::Check(true, false, this->fullBlockList);
#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        // This lock is needed only in the debug mode while we verify block counts. Not needed otherwise, as this list is never accessed concurrently.
        // Items are added to it by the allocator when allocations are allowed during concurrent sweep. The list is drained during the next sweep while
        // allocation are stopped.
        debugSweepableHeapBlockListLock.Enter();
        smallHeapBlockCount += HeapInfo::Check(true, false, this->sweepableHeapBlockList);
        debugSweepableHeapBlockListLock.Leave();
    }
#endif
    smallHeapBlockCount += HeapInfo::Check(true, false, this->heapBlockList, this->nextAllocableBlockHead);
    smallHeapBlockCount += HeapInfo::Check(false, false, this->nextAllocableBlockHead);
    Assert(!checkCount || this->heapBlockCount == smallHeapBlockCount);
    return smallHeapBlockCount;
}
#endif

#ifdef DUMP_FRAGMENTATION_STATS
template <typename TBlockType>
void
HeapBucketT<TBlockType>::AggregateBucketStats(HeapBucketStats& stats)
{
    auto allocatorHead = &this->allocatorHead;
    auto allocatorCurr = allocatorHead;

    do
    {
        TBlockType* allocatorHeapBlock = allocatorCurr->GetHeapBlock();
        if (allocatorHeapBlock)
        {
            allocatorHeapBlock->AggregateBlockStats(stats, true, allocatorCurr->freeObjectList, allocatorCurr->endAddress != 0);
        }
        allocatorCurr = allocatorCurr->GetNext();
    } while (allocatorCurr != allocatorHead);

    auto blockStatsAggregator = [&stats](TBlockType* heapBlock) {
        heapBlock->AggregateBlockStats(stats);
    };

    HeapBlockList::ForEach(emptyBlockList, blockStatsAggregator);
    HeapBlockList::ForEach(fullBlockList, blockStatsAggregator);
#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        HeapBlockList::ForEach(sweepableHeapBlockList, blockStatsAggregator);
    }
#endif
    HeapBlockList::ForEach(heapBlockList, blockStatsAggregator);
}
#endif

#ifdef RECYCLER_MEMORY_VERIFY
template <typename TBlockType>
void
HeapBucketT<TBlockType>::Verify()
{
    UpdateAllocators();
#if DBG
    RecyclerVerifyListConsistencyData recyclerVerifyListConsistencyData;

    if (TBlockType::HeapBlockAttributes::IsSmallBlock)
    {
        recyclerVerifyListConsistencyData.smallBlockVerifyListConsistencyData.SetupVerifyListConsistencyData((SmallHeapBlock*) nullptr, true, false);
    }
    else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
    {
        recyclerVerifyListConsistencyData.mediumBlockVerifyListConsistencyData.SetupVerifyListConsistencyData((MediumHeapBlock*) nullptr, true, false);
    }
    else
    {
        Assert(false);
    }
#endif

    HeapBlockList::ForEach(fullBlockList, [DebugOnly(&recyclerVerifyListConsistencyData)](TBlockType * heapBlock)
    {
        DebugOnly(VerifyBlockConsistencyInList(heapBlock, recyclerVerifyListConsistencyData));
        heapBlock->Verify();
    });

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
#if DBG
        if (TBlockType::HeapBlockAttributes::IsSmallBlock)
        {
            recyclerVerifyListConsistencyData.smallBlockVerifyListConsistencyData.SetupVerifyListConsistencyData((SmallHeapBlock*) nullptr, true, false);
        }
        else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
        {
            recyclerVerifyListConsistencyData.mediumBlockVerifyListConsistencyData.SetupVerifyListConsistencyData((MediumHeapBlock*) nullptr, true, false);
        }
        else
        {
            Assert(false);
        }
#endif

        HeapBlockList::ForEach(sweepableHeapBlockList, [DebugOnly(&recyclerVerifyListConsistencyData)](TBlockType * heapBlock)
        {
            DebugOnly(VerifyBlockConsistencyInList(heapBlock, recyclerVerifyListConsistencyData));
            heapBlock->Verify();
        });
    }
#endif

#if DBG
    if (TBlockType::HeapBlockAttributes::IsSmallBlock)
    {
        recyclerVerifyListConsistencyData.smallBlockVerifyListConsistencyData.SetupVerifyListConsistencyData((SmallHeapBlock*) this->nextAllocableBlockHead, true, false);
    }
    else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
    {
        recyclerVerifyListConsistencyData.mediumBlockVerifyListConsistencyData.SetupVerifyListConsistencyData((MediumHeapBlock*) this->nextAllocableBlockHead, true, false);
    }
    else
    {
        Assert(false);
    }
#endif

    HeapBlockList::ForEach(heapBlockList, [this, DebugOnly(&recyclerVerifyListConsistencyData)](TBlockType * heapBlock)
    {
        DebugOnly(VerifyBlockConsistencyInList(heapBlock, recyclerVerifyListConsistencyData));
        char * bumpAllocateAddress = nullptr;
        this->ForEachAllocator([heapBlock, &bumpAllocateAddress](TBlockAllocatorType * allocator)
        {
            if (allocator->GetHeapBlock() == heapBlock && allocator->GetEndAddress() != nullptr)
            {
                Assert(bumpAllocateAddress == nullptr);
                bumpAllocateAddress = (char *)allocator->GetFreeObjectList();
            }
        });
        if (bumpAllocateAddress != nullptr)
        {
            heapBlock->VerifyBumpAllocated(bumpAllocateAddress);
        }
        else
        {
            heapBlock->Verify(false);
        }
    });
}
#endif

#ifdef RECYCLER_VERIFY_MARK
template <typename TBlockType>
void
HeapBucketT<TBlockType>::VerifyMark()
{
    HeapBlockList::ForEach(this->fullBlockList, [](TBlockType * heapBlock)
    {
        heapBlock->VerifyMark();
    });

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        HeapBlockList::ForEach(this->sweepableHeapBlockList, [](TBlockType * heapBlock)
        {
            heapBlock->VerifyMark();
        });
    }
#endif

    HeapBlockList::ForEach(this->heapBlockList, [](TBlockType * heapBlock)
    {
        heapBlock->VerifyMark();
    });
}
#endif

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::Initialize(HeapInfo * heapInfo, uint sizeCat)
{
    heapBucket.Initialize(heapInfo, sizeCat);
    leafHeapBucket.Initialize(heapInfo, sizeCat);
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.Initialize(heapInfo, sizeCat);
    smallFinalizableWithBarrierHeapBucket.Initialize(heapInfo, sizeCat);
#endif
    finalizableHeapBucket.Initialize(heapInfo, sizeCat);
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::ResetMarks(ResetMarkFlags flags)
{
    heapBucket.ResetMarks(flags);
    leafHeapBucket.ResetMarks(flags);
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.ResetMarks(flags);
    smallFinalizableWithBarrierHeapBucket.ResetMarks(flags);
#endif

    // Although we pass in premarkFreeObjects, the finalizable heap bucket ignores
    // this parameter and never pre-marks free objects
    finalizableHeapBucket.ResetMarks(flags);
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::ScanInitialImplicitRoots(Recycler * recycler)
{
    heapBucket.ScanInitialImplicitRoots(recycler);
    // Don't need to scan implicit roots on leaf heap bucket
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.ScanInitialImplicitRoots(recycler);
    smallFinalizableWithBarrierHeapBucket.ScanInitialImplicitRoots(recycler);
#endif
    finalizableHeapBucket.ScanInitialImplicitRoots(recycler);
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::ScanNewImplicitRoots(Recycler * recycler)
{
    heapBucket.ScanNewImplicitRoots(recycler);
    // Need to scan new implicit roots on leaf heap bucket
    leafHeapBucket.ScanNewImplicitRoots(recycler);
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.ScanNewImplicitRoots(recycler);
    smallFinalizableWithBarrierHeapBucket.ScanNewImplicitRoots(recycler);
#endif
    finalizableHeapBucket.ScanNewImplicitRoots(recycler);
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::Sweep(RecyclerSweep& recyclerSweep)
{
    heapBucket.Sweep(recyclerSweep);
    leafHeapBucket.Sweep(recyclerSweep);
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.Sweep(recyclerSweep);
#endif
}

// Sweep finalizable objects first to ensure that if they reference any other
// objects in the finalizer - they are valid
template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::SweepFinalizableObjects(RecyclerSweep& recyclerSweep)
{
    finalizableHeapBucket.Sweep(recyclerSweep);
#ifdef RECYCLER_WRITE_BARRIER
    smallFinalizableWithBarrierHeapBucket.Sweep(recyclerSweep);
#endif
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::DisposeObjects()
{
    finalizableHeapBucket.DisposeObjects();
#ifdef RECYCLER_WRITE_BARRIER
    smallFinalizableWithBarrierHeapBucket.DisposeObjects();
#endif
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::TransferDisposedObjects()
{
    finalizableHeapBucket.TransferDisposedObjects();
#ifdef RECYCLER_WRITE_BARRIER
    smallFinalizableWithBarrierHeapBucket.TransferDisposedObjects();
#endif
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size))
{
    heapBucket.EnumerateObjects(infoBits, CallBackFunction);
    leafHeapBucket.EnumerateObjects(infoBits, CallBackFunction);
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.EnumerateObjects(infoBits, CallBackFunction);
    smallFinalizableWithBarrierHeapBucket.EnumerateObjects(infoBits, CallBackFunction);
#endif
    finalizableHeapBucket.EnumerateObjects(infoBits, CallBackFunction);
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::FinalizeAllObjects()
{
    finalizableHeapBucket.FinalizeAllObjects();
#ifdef RECYCLER_WRITE_BARRIER
    smallFinalizableWithBarrierHeapBucket.FinalizeAllObjects();
#endif
}

template <class TBlockAttributes>
uint
HeapBucketGroup<TBlockAttributes>::Rescan(Recycler * recycler, RescanFlags flags)
{
    return heapBucket.Rescan(recycler, flags) +
        leafHeapBucket.Rescan(recycler, flags) +
#ifdef RECYCLER_WRITE_BARRIER
        smallNormalWithBarrierHeapBucket.Rescan(recycler, flags) +
        smallFinalizableWithBarrierHeapBucket.Rescan(recycler, flags) +
#endif
        finalizableHeapBucket.Rescan(recycler, flags);
}

#if ENABLE_CONCURRENT_GC
template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::PrepareSweep()
{
    heapBucket.PrepareSweep();
    leafHeapBucket.PrepareSweep();
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.PrepareSweep();
    smallFinalizableWithBarrierHeapBucket.PrepareSweep();
#endif
    finalizableHeapBucket.PrepareSweep();
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::SetupBackgroundSweep(RecyclerSweep& recyclerSweep)
{
    heapBucket.SetupBackgroundSweep(recyclerSweep);
    leafHeapBucket.SetupBackgroundSweep(recyclerSweep);
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.SetupBackgroundSweep(recyclerSweep);
#endif
}
#endif
#if ENABLE_PARTIAL_GC
template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::SweepPartialReusePages(RecyclerSweep& recyclerSweep)
{
    // Leaf heap bucket are always reused for allocation and can be done on the concurrent thread
    // WriteBarrier-TODO: Do the same for write barrier buckets
    heapBucket.SweepPartialReusePages(recyclerSweep);

#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.SweepPartialReusePages(recyclerSweep);
    smallFinalizableWithBarrierHeapBucket.SweepPartialReusePages(recyclerSweep);
#endif

    finalizableHeapBucket.SweepPartialReusePages(recyclerSweep);
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::FinishPartialCollect(RecyclerSweep * recyclerSweep)
{
    heapBucket.FinishPartialCollect(recyclerSweep);
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.FinishPartialCollect(recyclerSweep);
    smallFinalizableWithBarrierHeapBucket.FinishPartialCollect(recyclerSweep);
#endif
    finalizableHeapBucket.FinishPartialCollect(recyclerSweep);

    // Leaf heap block always do a full sweep instead of partial sweep
    // (since touching the page doesn't affect rescan)
    // So just need to verify heap block count (which finishPartialCollect would have done)
    // WriteBarrier-TODO: Do that same for write barrier buckets
    RECYCLER_SLOW_CHECK(leafHeapBucket.VerifyHeapBlockCount(recyclerSweep != nullptr && recyclerSweep->IsBackground()));
}
#endif

#if ENABLE_CONCURRENT_GC
template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::SweepPendingObjects(RecyclerSweep& recyclerSweep)
{
    // For leaf buckets, we can always reuse the page as we don't need to rescan them for partial GC
    // It should have been swept immediately during Sweep
    // WriteBarrier-TODO: Do the same for write barrier buckets
    Assert(recyclerSweep.GetPendingSweepBlockList(&leafHeapBucket) == nullptr);

    heapBucket.SweepPendingObjects(recyclerSweep);
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.SweepPendingObjects(recyclerSweep);
    smallFinalizableWithBarrierHeapBucket.SweepPendingObjects(recyclerSweep);
#endif

    finalizableHeapBucket.SweepPendingObjects(recyclerSweep);
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::TransferPendingEmptyHeapBlocks(RecyclerSweep& recyclerSweep)
{
    recyclerSweep.TransferPendingEmptyHeapBlocks(&heapBucket);
    recyclerSweep.TransferPendingEmptyHeapBlocks(&leafHeapBucket);
#ifdef RECYCLER_WRITE_BARRIER
    recyclerSweep.TransferPendingEmptyHeapBlocks(&smallNormalWithBarrierHeapBucket);
    recyclerSweep.TransferPendingEmptyHeapBlocks(&smallFinalizableWithBarrierHeapBucket);
#endif
    recyclerSweep.TransferPendingEmptyHeapBlocks(&finalizableHeapBucket);
}
#endif

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
template <class TBlockAttributes>
size_t
HeapBucketGroup<TBlockAttributes>::GetNonEmptyHeapBlockCount(bool checkCount) const
{
    return heapBucket.GetNonEmptyHeapBlockCount(checkCount) +
        finalizableHeapBucket.GetNonEmptyHeapBlockCount(checkCount) +
#ifdef RECYCLER_WRITE_BARRIER
        smallNormalWithBarrierHeapBucket.GetNonEmptyHeapBlockCount(checkCount) +
        smallFinalizableWithBarrierHeapBucket.GetNonEmptyHeapBlockCount(checkCount) +
#endif
        leafHeapBucket.GetNonEmptyHeapBlockCount(checkCount);
}

template <class TBlockAttributes>
size_t
HeapBucketGroup<TBlockAttributes>::GetEmptyHeapBlockCount() const
{
    return heapBucket.GetEmptyHeapBlockCount() +
        finalizableHeapBucket.GetEmptyHeapBlockCount() +
#ifdef RECYCLER_WRITE_BARRIER
        smallNormalWithBarrierHeapBucket.GetEmptyHeapBlockCount() +
        smallFinalizableWithBarrierHeapBucket.GetEmptyHeapBlockCount() +
#endif
        leafHeapBucket.GetEmptyHeapBlockCount();
}
#endif

#ifdef RECYCLER_SLOW_CHECK_ENABLED
template <class TBlockAttributes>
size_t
HeapBucketGroup<TBlockAttributes>::Check()
{
    return heapBucket.Check() + finalizableHeapBucket.Check() + leafHeapBucket.Check()
#ifdef RECYCLER_WRITE_BARRIER
        + smallNormalWithBarrierHeapBucket.Check() + smallFinalizableWithBarrierHeapBucket.Check()
#endif
        ;
}
#endif
#ifdef RECYCLER_MEMORY_VERIFY
template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::Verify()
{
    heapBucket.Verify();
    finalizableHeapBucket.Verify();
    leafHeapBucket.Verify();
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.Verify();
    smallFinalizableWithBarrierHeapBucket.Verify();
#endif
}
#endif
#ifdef RECYCLER_VERIFY_MARK
template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::VerifyMark()
{
    heapBucket.VerifyMark();
    finalizableHeapBucket.VerifyMark();
    leafHeapBucket.VerifyMark();
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.VerifyMark();
    smallFinalizableWithBarrierHeapBucket.VerifyMark();
#endif
}
#endif

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::StartAllocationDuringConcurrentSweep()
{
    // If there were no allocable heap blocks we would not have started alllocations. Stop allocations, only if we started allocations for each of these buckets.
    if (heapBucket.IsAllocationStopped())
    {
        heapBucket.StartAllocationDuringConcurrentSweep();
    }

    if (leafHeapBucket.IsAllocationStopped())
    {
        leafHeapBucket.StartAllocationDuringConcurrentSweep();
    }
#ifdef RECYCLER_WRITE_BARRIER
    if (smallNormalWithBarrierHeapBucket.IsAllocationStopped())
    {
        smallNormalWithBarrierHeapBucket.StartAllocationDuringConcurrentSweep();
    }
#endif
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::FinishConcurrentSweep()
{
    heapBucket.FinishConcurrentSweep();
    leafHeapBucket.FinishConcurrentSweep();

#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.FinishConcurrentSweep();
#endif
}
#endif

#if DBG
template <class TBlockAttributes>
bool
HeapBucketGroup<TBlockAttributes>::AllocatorsAreEmpty()
{
    return heapBucket.AllocatorsAreEmpty()
        && finalizableHeapBucket.AllocatorsAreEmpty()
        && leafHeapBucket.AllocatorsAreEmpty()
#ifdef RECYCLER_WRITE_BARRIER
        && smallNormalWithBarrierHeapBucket.AllocatorsAreEmpty()
        && smallFinalizableWithBarrierHeapBucket.AllocatorsAreEmpty()
#endif
        ;
}
#endif

namespace Memory
{
    template class HeapBucketGroup<SmallAllocationBlockAttributes>;
    template class HeapBucketGroup<MediumAllocationBlockAttributes>;

    EXPLICIT_INSTANTIATE_WITH_SMALL_HEAP_BLOCK_TYPE(HeapBucketT);
};

