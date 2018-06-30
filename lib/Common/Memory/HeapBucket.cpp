//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"


HeapBucket::HeapBucket() :
    heapInfo(nullptr),
    sizeCat(0)
{
#if defined(RECYCLER_SLOW_CHECK_ENABLED) || ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    heapBlockCount = 0;
    newHeapBlockCount = 0;
#endif

#if defined(RECYCLER_SLOW_CHECK_ENABLED)
    emptyHeapBlockCount = 0;
#endif

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    this->allocationsStartedDuringConcurrentSweep = false;
    this->concurrentSweepAllocationsThresholdExceeded = false;
#endif

#ifdef RECYCLER_PAGE_HEAP
    isPageHeapEnabled = false;
#endif
}

HeapInfo *
HeapBucket::GetHeapInfo() const
{
    return this->heapInfo;
}

uint
HeapBucket::GetSizeCat() const
{
    return this->sizeCat;
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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if SUPPORT_WIN32_SLIST
    lastKnownNextAllocableBlockHead(nullptr),
    allocableHeapBlockListHead(nullptr),
    sweepableHeapBlockList(nullptr),
#endif
#endif
    explicitFreeList(nullptr),
    lastExplicitFreeListAllocator(nullptr)
{
#ifdef RECYCLER_PAGE_HEAP
    explicitFreeLockBlockList = nullptr;
#endif

    isAllocationStopped = false;
}


template <typename TBlockType>
HeapBucketT<TBlockType>::~HeapBucketT()
{
    DeleteHeapBlockList(this->heapBlockList);
    DeleteHeapBlockList(this->fullBlockList);

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if SUPPORT_WIN32_SLIST
    if (allocableHeapBlockListHead != nullptr)
    {
        if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
        {
            FlushInterlockedSList(this->allocableHeapBlockListHead);
        }

        _aligned_free(this->allocableHeapBlockListHead);
    }

    DeleteHeapBlockList(this->sweepableHeapBlockList);
#endif
#endif

#if defined(RECYCLER_SLOW_CHECK_ENABLED) || ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    Assert(this->heapBlockCount + this->newHeapBlockCount == 0);
#endif
    RECYCLER_SLOW_CHECK(Assert(this->emptyHeapBlockCount == HeapBlockList::Count(this->emptyBlockList)));
    DeleteEmptyHeapBlockList(this->emptyBlockList);
#if defined(RECYCLER_SLOW_CHECK_ENABLED)
    Assert(this->heapBlockCount + this->newHeapBlockCount + this->emptyHeapBlockCount == 0);
#endif
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
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

    // While in the SLIST the blocks live as standalone, when they come out they
    // will go into appropriate list and the Next block will be set accordingly.
    heapBlock->SetNextBlock(nullptr);
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

#if defined(RECYCLER_SLOW_CHECK_ENABLED) || ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    this->heapBlockCount++;
#endif

    recycler->autoHeap.uncollectedAllocBytes += heapBlock->GetAndClearLastFreeCount() * heapBlock->GetObjectSize();
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

template <typename TBlockType>
void
HeapBucketT<TBlockType>::AssertCheckHeapBlockNotInAnyList(TBlockType * heapBlock)
{
#if DBG
    AssertMsg(!HeapBlockList::Contains(heapBlock, heapBlockList), "The heap block already exists in the heapBlockList.");
    AssertMsg(!HeapBlockList::Contains(heapBlock, fullBlockList), "The heap block already exists in the fullBlockList.");
    AssertMsg(!HeapBlockList::Contains(heapBlock, emptyBlockList), "The heap block already exists in the emptyBlockList.");
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    AssertMsg(!HeapBlockList::Contains(heapBlock, sweepableHeapBlockList), "The heap block already exists in the sweepableHeapBlockList.");
#endif
#endif
}

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
#if SUPPORT_WIN32_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && !this->IsAnyFinalizableBucket())
    {
        allocatingDuringConcurrentSweep = true;
        // This lock is needed only in the debug mode while we verify block counts. Not needed otherwise, as this list is never accessed concurrently.
        // Items are added to it by the allocator when allocations are allowed during concurrent sweep. The list is drained during the next sweep while
        // allocation are stopped.
        debugSweepableHeapBlockListLock.Enter();
        if (allocableHeapBlockListHead != nullptr)
        {
            currentHeapBlockCount += QueryDepthInterlockedSList(allocableHeapBlockListHead);
        }
        currentHeapBlockCount += HeapBlockList::Count(sweepableHeapBlockList);
        debugSweepableHeapBlockListLock.Leave();
    }
#endif
#endif

    // Recycler can be null if we have OOM in the ctor
    if (this->GetRecycler() && this->GetRecycler()->recyclerSweepManager != nullptr)
    {
        currentHeapBlockCount += this->GetRecycler()->recyclerSweepManager->GetHeapBlockCount(this);
    }
#endif

    // There is no way to determine the number of item in an SLIST if there are >= 65535 items in the list.
    RECYCLER_SLOW_CHECK(Assert(!checkCount || heapBlockCount == currentHeapBlockCount || (heapBlockCount >= 65535 && allocatingDuringConcurrentSweep)));

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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP 
#if SUPPORT_WIN32_SLIST
    bool heapBlockFromAllocableHeapBlockList = false;
    DebugOnly(bool heapBlockInPendingSweepPrepList = false);

    if (heapBlock == nullptr && this->allocationsStartedDuringConcurrentSweep)
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
            Assert(!this->IsAnyFinalizableBucket());
            DebugOnly(this->AssertCheckHeapBlockNotInAnyList(heapBlock));
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
            heapBlock->wasAllocatedFromDuringSweep = true;
#endif
#if DBG || defined(RECYCLER_TRACE)
            if (heapBlock->isPendingConcurrentSweepPrep)
            {
                AssertMsg(heapBlock->objectsAllocatedDuringConcurrentSweepCount == 0, "We just picked up this block for allocations during concurrent sweep, we haven't allocated from it yet.");

#ifdef RECYCLER_TRACE
                recycler->PrintBlockStatus(this, heapBlock, _u("[**31**] pending Pass1 prep, picked up for allocations during concurrent sweep."));
#endif
                DebugOnly(heapBlockInPendingSweepPrepList = true);
            }
            else
            {
                // Put the block in the sweepable heap block list so we don't lose track of it. The block will eventually be moved to the
                // heapBlockList or fullBlockList as appropriate during the next sweep.
                AssertMsg(!HeapBlockList::Contains(heapBlock, sweepableHeapBlockList), "The heap block already exists in this list.");

#ifdef RECYCLER_TRACE
                recycler->PrintBlockStatus(this, heapBlock, _u("[**32**] picked up for allocations during concurrent sweep."));
#endif
            }
#endif
            heapBlock->SetNextBlock(sweepableHeapBlockList);
            sweepableHeapBlockList = heapBlock;
            heapBlockFromAllocableHeapBlockList = true;
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
        // When allocations are allowed during concurrent sweep we set nextAllocableBlockHead to NULL as the allocator will pick heap blocks from the
        // interlocked SLIST. During that time, the heap block at the top of the SLIST is always the nextAllocableBlockHead.
        // If the heapBlock was just picked from the SLIST and nextAllocableBlockHead is not NULL then we just resumed normal allocations on the background thread
        // while finishing the concurrent sweep, and the nextAllocableBlockHead is already set properly.
        if (this->nextAllocableBlockHead != nullptr && !heapBlockFromAllocableHeapBlockList)
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

bool
HeapBucket::AllocationsStartedDuringConcurrentSweep() const
{
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    return this->allocationsStartedDuringConcurrentSweep;
#else
    return false;
#endif
}

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
bool
HeapBucket::ConcurrentSweepAllocationsThresholdExceeded() const
{
    return this->concurrentSweepAllocationsThresholdExceeded;
}

bool
HeapBucket::DoTwoPassConcurrentSweepPreCheck()
{
    this->concurrentSweepAllocationsThresholdExceeded = ((this->heapBlockCount + this->newHeapBlockCount) > RecyclerHeuristic::AllocDuringConcurrentSweepHeapBlockThreshold);

#ifdef RECYCLER_TRACE
    if (this->GetRecycler()->GetRecyclerFlagsTable().Trace.IsEnabled(Js::ConcurrentSweepPhase) && CONFIG_FLAG_RELEASE(Verbose))
    {
        if (this->concurrentSweepAllocationsThresholdExceeded)
        {
            Output::Print(_u("[HeapBucket 0x%p] exceeded concurrent sweep allocations threshold (%d). Total heap block count: %d \n"), this, RecyclerHeuristic::AllocDuringConcurrentSweepHeapBlockThreshold, this->heapBlockCount + this->newHeapBlockCount);
        }
    }
#endif

    return this->concurrentSweepAllocationsThresholdExceeded;
}
#endif

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
#if ENABLE_PARTIAL_GC
        // wait for background sweeping finish if there are too many pages allocated during background sweeping
        if (recycler->IsConcurrentSweepExecutingState() && recycler->autoHeap.uncollectedNewPageCount > (uint)CONFIG_FLAG(NewPagesCapDuringBGSweeping))
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
#if defined(RECYCLER_SLOW_CHECK_ENABLED)
        Assert(this->emptyHeapBlockCount == 0);
#endif
    }
    else
    {
        emptyBlockList = heapBlock->GetNextBlock();
#if defined(RECYCLER_SLOW_CHECK_ENABLED)
        this->emptyHeapBlockCount--;
#endif
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
    this->heapInfo->AppendNewHeapBlock(heapBlock, this);
#if defined(RECYCLER_SLOW_CHECK_ENABLED) || ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
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
#if defined(RECYCLER_SLOW_CHECK_ENABLED)
    this->emptyHeapBlockCount++;
#endif
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
        if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && !this->IsAnyFinalizableBucket())
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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
        if (!CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) || this->IsAnyFinalizableBucket())
#endif
        {
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
        }
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && !this->IsAnyFinalizableBucket())
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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
        // As the blocks are added to a SLIST and used from there during concurrent sweep, the expectFull assertion doesn't hold anymore.
        // We could do some work to make this work again but there may be perf hit and it may be fragile.
        if (!CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
#endif
        {
            Assert(*expectFull && !*expectDispose);
            Assert(heapBlock->HasFreeObject());
            Assert(!heapBlock->HasAnyDisposeObjects());
        }
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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
        // As the blocks are added to the SLIST and used from there during concurrent sweep, the expectFull assertion doesn't hold anymore.
        if (!CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
#endif
        {
            Assert(*expectFull == (!heapBlock->HasFreeObject() || heapBlock->IsInAllocator()) || nextAllocableBlockHead == nullptr);
        }
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    Assert(this->IsAllocationStopped() || this->AllocationsStartedDuringConcurrentSweep());
#else
    Assert(this->IsAllocationStopped());
#endif

    HeapBlockList::ForEachEditing(heapBlockList, [=, &recyclerSweep](TBlockType * heapBlock)
    {
        // The whole list need to be consistent
        DebugOnly(VerifyBlockConsistencyInList(heapBlock, recyclerSweep));

#ifdef RECYCLER_TRACE
        recyclerSweep.GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**1**] starting Sweep Pass1."));
#endif
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
            DebugOnly(this->AssertCheckHeapBlockNotInAnyList(heapBlock));
            AssertMsg(!HeapBlockList::Contains(heapBlock, pendingSweepList), "The heap block already exists in the pendingSweepList.");

            heapBlock->SetNextBlock(pendingSweepList);
            pendingSweepList = heapBlock;
#ifdef RECYCLER_TRACE
            recyclerSweep.GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**2**] finished Sweep Pass1, heapblock added to pendingSweepList."));
#endif
#if ENABLE_PARTIAL_GC
            recyclerSweep.GetManager()->NotifyAllocableObjects(heapBlock);
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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
            if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
            {
                AssertMsg(!heapBlock->isPendingConcurrentSweepPrep, "Finalizable blocks don't support allocations during concurrent sweep.");
            }
#endif

            DebugOnly(heapBlock->template AsFinalizableBlock<typename TBlockType::HeapBlockAttributes>()->SetIsPendingDispose());

            // These are the blocks that have swept finalizable object

            // We already transferred the non finalizable swept objects when we are not doing
            // concurrent collection, so we only need to queue up the blocks that have
            // finalizable objects, so that we can go through and call the dispose, and then
            // transfer the finalizable object back to the free list.
            SmallFinalizableHeapBucketT<typename TBlockType::HeapBlockAttributes> * finalizableHeapBucket = (SmallFinalizableHeapBucketT<typename TBlockType::HeapBlockAttributes>*)this;
            DebugOnly(this->AssertCheckHeapBlockNotInAnyList(heapBlock));
            //AssertMsg(!HeapBlockList::Contains(heapBlock, finalizableHeapBucket->pendingDisposeList), "The heap block already exists in the pendingDisposeList.");
            heapBlock->template AsFinalizableBlock<typename TBlockType::HeapBlockAttributes>()->SetNextBlock(finalizableHeapBucket->pendingDisposeList);
            finalizableHeapBucket->pendingDisposeList = heapBlock->template AsFinalizableBlock<typename TBlockType::HeapBlockAttributes>();
            Assert(!this->heapInfo->hasPendingTransferDisposedObjects);
            recycler->hasDisposableObject = true;
#ifdef RECYCLER_TRACE
            recyclerSweep.GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**3**] finished Sweep Pass1, heapblock added to pendingDisposeList."));
#endif
            break;
        }
        case SweepStateSwept:
        {
            Assert(this->nextAllocableBlockHead == nullptr);
            Assert(heapBlock->HasFreeObject());
            DebugOnly(this->AssertCheckHeapBlockNotInAnyList(heapBlock));

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
            if (this->AllocationsStartedDuringConcurrentSweep())
            {
                Assert(!this->IsAnyFinalizableBucket());
                Assert(!heapBlock->isPendingConcurrentSweepPrep);
                bool blockAddedToSList = HeapBucketT<TBlockType>::PushHeapBlockToSList(this->allocableHeapBlockListHead, heapBlock);

                // If we encountered OOM while pushing the heapBlock to the SLIST we must add it to the heapBlockList so we don't lose track of it.
                if (!blockAddedToSList)
                {
                    //TODO: akatti: We should handle this gracefully and try to recover from this state.
                    AssertOrFailFastMsg(false, "OOM while adding a heap block to the SLIST during concurrent sweep.");
                }
#ifdef RECYCLER_TRACE
                else
                {
                    this->GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**4**] swept and added to SLIST allocableHeapBlockListHead during Pass1."));
                }
#endif
            }
            else
#endif
            {
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
                AssertMsg(!this->AllowAllocationsDuringConcurrentSweep(), "Why are allocations not started during concurrent sweep?");
#endif
                heapBlock->SetNextBlock(this->heapBlockList);
                this->heapBlockList = heapBlock;
            }

#ifdef RECYCLER_TRACE
            recyclerSweep.GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**6**] finished Sweep Pass1, heapblock added to heapBlockList."));
#endif
#if ENABLE_PARTIAL_GC
            recyclerSweep.GetManager()->NotifyAllocableObjects(heapBlock);
#endif
            break;
        }
        case SweepStateFull:
        {
            Assert(!heapBlock->HasFreeObject());
            DebugOnly(this->AssertCheckHeapBlockNotInAnyList(heapBlock));
            heapBlock->SetNextBlock(this->fullBlockList);
            this->fullBlockList = heapBlock;
#ifdef RECYCLER_TRACE
            recyclerSweep.GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**7**] finished Sweep Pass1, heapblock FULL added to fullBlockList."));
#endif
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
            DebugOnly(this->AssertCheckHeapBlockNotInAnyList(heapBlock));
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
            if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
            {
                AssertMsg(heapBlock->objectsAllocatedDuringConcurrentSweepCount == 0, "We allocated to this block during concurrent sweep; it's not EMPTY anymore, it should NOT be freed or queued as EMPTY.");
            }
#endif

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
#ifdef RECYCLER_TRACE
                recyclerSweep.GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**8**] finished Sweep Pass1, heapblock EMPTY added to pendingEmptyBlockList."));
#endif
            }
            else
#endif
            {
                // Just free the page in thread (and zero the page)
                heapBlock->ReleasePagesSweep(recycler);
                FreeHeapBlock(heapBlock);
#if defined(RECYCLER_SLOW_CHECK_ENABLED) || ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
                this->heapBlockCount--;
#endif
#ifdef RECYCLER_TRACE
                recyclerSweep.GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**9**] finished Sweep Pass1, heapblock EMPTY, was FREED in-thread."));
#endif
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
    if (recyclerSweep.GetManager()->HasSetupBackgroundSweep())
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && this->sweepableHeapBlockList != nullptr)
    {
        Assert(!this->IsAnyFinalizableBucket());

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        // This lock is needed only in the debug mode while we verify block counts. Not needed otherwise, as this list is never accessed concurrently.
        // Items are added to it by the allocator when allocations are allowed during concurrent sweep. The list is drained during the next sweep while
        // allocation are stopped.
        debugSweepableHeapBlockListLock.Enter();
#endif
        // Return the blocks we may have allocated from during the previous concurrent sweep back to the fullBlockList.
        // We need to rebuild the free bit vectors for these blocks.
        HeapBlockList::ForEachEditing(this->sweepableHeapBlockList, [this](TBlockType * heapBlock)
        {
            heapBlock->BuildFreeBitVector();

            AssertMsg(!HeapBlockList::Contains(heapBlock, heapBlockList), "The heap block already exists in the heapBlockList.");
            AssertMsg(!HeapBlockList::Contains(heapBlock, fullBlockList), "The heap block already exists in the fullBlockList.");
            AssertMsg(!HeapBlockList::Contains(heapBlock, emptyBlockList), "The heap block already exists in the emptyBlockList.");

            heapBlock->SetNextBlock(this->fullBlockList);
            this->fullBlockList = heapBlock;
        });
        this->sweepableHeapBlockList = nullptr;
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        debugSweepableHeapBlockListLock.Leave();
#endif
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    // In order to allow allocations during sweep (Pass-1) we will set aside blocks after nextAllocableBlockHead (excluding) and allow
    // allocations to these blocks as we know that these blocks are not full yet. These will need to be swept later though before starting Pass-2
    // of the sweep.
    this->PrepareForAllocationsDuringConcurrentSweep(currentHeapBlockList);
#endif

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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    Recycler * recycler = this->GetRecycler();
    if (!CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) || !recycler->AllowAllocationsDuringConcurrentSweep() || !this->concurrentSweepAllocationsThresholdExceeded)
    {
        return false;
    }

#if ENABLE_PARTIAL_GC
    bool isPartialGC = (recycler->recyclerSweepManager != nullptr) && recycler->recyclerSweepManager->InPartialCollect();
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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    this->allocationsStartedDuringConcurrentSweep = false;
#if SUPPORT_WIN32_SLIST
    this->lastKnownNextAllocableBlockHead = this->nextAllocableBlockHead;
#endif
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
template <typename TBlockType>
void
HeapBucketT<TBlockType>::StartAllocationDuringConcurrentSweep()
{
    Recycler * recycler = this->GetRecycler();
    Assert(!recycler->recyclerSweepManager->InPartialCollect());
    Assert(!this->IsAnyFinalizableBucket());

    Assert(this->IsAllocationStopped());
    this->isAllocationStopped = false;
    Assert(!this->allocationsStartedDuringConcurrentSweep);
    this->allocationsStartedDuringConcurrentSweep = true;

#if SUPPORT_WIN32_SLIST
    // When allocations are allowed during concurrent sweep we set nextAllocableBlockHead to NULL as the allocator will pick heap blocks from the
    // interlocked SLIST. During that time, the heap block at the top of the SLIST is always the nextAllocableBlockHead.
    this->nextAllocableBlockHead = nullptr;
    this->lastKnownNextAllocableBlockHead = nullptr;
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

/*//////////////////////////////////////////////////////////////////////////////////////////////////////
If allocations are to be allowed to existing heap blocks during concurrent sweep, we set aside a few
heap blocks from the heapBlockList prior to beginning sweep. However, we eed to then go back and make
sure these blocks also swept before this sweep finishes. In order to do this we clearly define concurrent
sweep having 2 passes now. These passes existed before but were not distiguished as they would always start
and finish in one go on the background thread. However, whenever allocations are allowed during concurrent
sweep; the concurrent sweep will start Pass1 on the background thread, wait to finish Pass1 of the blocks
we set aside to allocate from on the main thread and then go back to finish Pass2 for all heap blocks on
the background thread. Note that, due to this need to finish Pass1 on the foreground thread the overall
background sweep will now appear to take longer whenever we chose to do such a two-pass sweep.

The sequence of things we do to allow allocations during concurrent sweep is described below:
1. At the beginning of concurrrent sweep we decide if we will benefit from allowing allocations during concurrent
sweep for any of the buckets. If there is at-least one bucket for which we think we will benefit we will turn on
allocations during concurrent sweep. Once turned on we will attempt to enable allocations during concurrent sweep
for all supported buckets (i.e. small/medium, normal/leaf, non-finalizable buckets.write barrrier bickets are supported
as well.).
2. If allocations are turned on during concurrent sweep, we will see if there are any allocable blocks in the
heapBlockList after the nextAllocableBlockHead. If we find any such blocks, we move them to a SLIST that the
allocator can pick these blocks from during sweep.
3. CollectionStateConcurrentSweepPass1: We will finish Pass1 of the sweep for all the remaining blocks (other than the
ones we put in the SLIST in step 2 above) This will generally happen on the background thread unless we are forcing
in-thread sweep. This state is now specifically identified as CollectionStateConcurrentSweepPass1.
4. CollectionStateConcurrentSweepPass1Wait: At this point we need to wait for all the blocks that we put in the SLIST
to also finish the Pass1 of the sweep. This needs to happen on the foreground thread so we prevent the allocator from
picking up the blocks from SLIST while we do this. This state is now identified as CollectionStateConcurrentSweepPass1Wait.
5. CollectionStateConcurrentSweepPass2: At this point we will do the actual sweeping of all the blocks that are not yet swept,
for eaxample, any blocks that were put onto the pendingSweepList. As these blocks get swept we keep adding them to the
SLIST again to allow allocators to allocate from them as soon as they are swept.
6. Before the Pass2 can finish we can call this concurrent sweep done we need to move all the blocks off of the SLIST so
that normal allocations can begin after the sweep. This is the last step of the concurrent sweep.
//////////////////////////////////////////////////////////////////////////////////////////////////////*/
template<typename TBlockType>
void
HeapBucketT<TBlockType>::PrepareForAllocationsDuringConcurrentSweep(TBlockType * &currentHeapBlockList)
{
#if SUPPORT_WIN32_SLIST
    if (this->AllowAllocationsDuringConcurrentSweep())
    {
        this->EnsureAllocableHeapBlockList();

        Assert(!this->IsAnyFinalizableBucket());
        Assert(HeapBucketT<TBlockType>::QueryDepthInterlockedSList(this->allocableHeapBlockListHead) == 0);
        Assert(HeapBlockList::Count(this->sweepableHeapBlockList) == 0);

        TBlockType* startingNextAllocableBlockHead = this->lastKnownNextAllocableBlockHead;
        bool allocationsStarted = false;
        if (startingNextAllocableBlockHead != nullptr)
        {
            // To avoid a race condition between the allocator attempting to allocate from the lastKnownNextAllocableBlockHead and this code
            // where we are adding it to the SLIST we skip the lastKnownNextAllocableBlockHead and pick up the next block to start with.
            // Allocations should have stopped by then; so allocator shouldn't pick up the lastKnownNextAllocableBlockHead->Next block.
            TBlockType* savedNextAllocableBlockHead = startingNextAllocableBlockHead->GetNextBlock();
            startingNextAllocableBlockHead->SetNextBlock(nullptr);
            startingNextAllocableBlockHead = savedNextAllocableBlockHead;

            if (startingNextAllocableBlockHead != nullptr)
            {
                // The allocable blocks, if any are available, will now be added to the allocable blocks SLIST at this time; start allocations now.
                this->StartAllocationDuringConcurrentSweep();
                allocationsStarted = true;

                HeapBlockList::ForEachEditing(startingNextAllocableBlockHead, [this, &allocationsStarted](TBlockType * heapBlock)
                {
                    // This heap block is NOT ready to be swept concurrently as it hasn't yet been through sweep prep (i.e. Pass1 of sweep).
                    heapBlock->isPendingConcurrentSweepPrep = true;
                    DebugOnly(this->AssertCheckHeapBlockNotInAnyList(heapBlock));
                    bool blockAddedToSList = HeapBucketT<TBlockType>::PushHeapBlockToSList(this->allocableHeapBlockListHead, heapBlock);

                    // If we encountered OOM while pushing the heapBlock to the SLIST we must add it to the heapBlockList so we don't lose track of it.
                    if (!blockAddedToSList)
                    {
                        //TODO: akatti: We should handle this gracefully and try to recover from this state.
                        AssertOrFailFastMsg(false, "OOM while adding a heap block to the SLIST during concurrent sweep.");
                    }
                    else
                    {
#ifdef RECYCLER_TRACE
                        this->GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**5**] added to SLIST before Pass1."));
#endif
                    }
                });
#ifdef RECYCLER_TRACE
                if (this->GetRecycler()->GetRecyclerFlagsTable().Trace.IsEnabled(Js::ConcurrentSweepPhase) && CONFIG_FLAG_RELEASE(Verbose))
                {
                    size_t currentHeapBlockCount = QueryDepthInterlockedSList(allocableHeapBlockListHead);
                    CollectionState collectionState = this->GetRecycler()->collectionState;
                    Output::Print(_u("[GC #%d] [HeapBucket 0x%p] Starting allocations during  concurrent sweep with %d blocks. [CollectionState: %d] \n"), this->GetRecycler()->collectionCount, this, currentHeapBlockCount, collectionState);
                    Output::Print(_u("[GC #%d] [HeapBucket 0x%p] The heapBlockList has %d blocks. Total heapBlockCount is %d.\n\n"), this->GetRecycler()->collectionCount, this, HeapBlockList::Count(this->heapBlockList), this->heapBlockCount);
                }
#endif
            }
        }

        if (!allocationsStarted)
        {
            // If we didn't start allocations yet, start them now in anticipation of blocks becoming available later as blocks complete sweep.
            this->StartAllocationDuringConcurrentSweep();
            allocationsStarted = true;
        }

        Assert(!this->IsAllocationStopped());
    }
#endif
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
#if defined(RECYCLER_SLOW_CHECK_ENABLED) || ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    ::InterlockedDecrement(&this->newHeapBlockCount);
    this->heapBlockCount++;
#endif
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
HeapBucketT<TBlockType>::FinishConcurrentSweepPass1(RecyclerSweep& recyclerSweep)
{
    if (this->concurrentSweepAllocationsThresholdExceeded)
    {
        AssertMsg(this->AllowAllocationsDuringConcurrentSweep(), "Why are we in two pass concurrent sweep?");
        Assert(!this->IsAnyFinalizableBucket());

        // Rebuild the free bit vectors for the blocks we allocated from during concurrent sweep.
        TBlockType * currentPendingSweepPrepHeapBlockList = nullptr;
        TBlockType * currentSweepableHeapBlockList = this->sweepableHeapBlockList;
        this->sweepableHeapBlockList = nullptr;

        HeapBlockList::ForEachEditing(currentSweepableHeapBlockList, [this, &currentPendingSweepPrepHeapBlockList](TBlockType * heapBlock)
        {
            if (heapBlock->isPendingConcurrentSweepPrep)
            {
                ushort previousFreeCount = heapBlock->freeCount;
                heapBlock->BuildFreeBitVector();

#if ENABLE_PARTIAL_GC
                heapBlock->oldFreeCount = heapBlock->lastFreeCount = heapBlock->freeCount;
#else
                heapBlock->lastFreeCount = heapBlock->freeCount;
#endif
                ushort newAllocatedObjects = previousFreeCount - heapBlock->freeCount;
                AssertMsg(newAllocatedObjects == heapBlock->objectsMarkedDuringSweep, "The counts of objects allocated during sweep should match the objects marked during sweep.");
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
                heapBlock->objectsAllocatedDuringConcurrentSweepCount = newAllocatedObjects;
#endif

                ushort currentMarkCount = (ushort)heapBlock->GetMarkCountForSweep();
                heapBlock->markCount = currentMarkCount;
#if DBG
                heapBlock->GetRecycler()->heapBlockMap.SetPageMarkCount(heapBlock->GetAddress(), currentMarkCount);
#endif
#ifdef RECYCLER_TRACE
                heapBlock->GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**13**] ending sweep Pass1, rebuilt free bit vector and set page mark count to match."));
#endif

                heapBlock->SetNextBlock(currentPendingSweepPrepHeapBlockList);
                currentPendingSweepPrepHeapBlockList = heapBlock;
            }
            else
            {
                heapBlock->SetNextBlock(this->sweepableHeapBlockList);
                this->sweepableHeapBlockList = heapBlock;
            }
        });

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        // This lock is needed only in the debug mode while we verify block counts. Not needed otherwise, as this list is never accessed concurrently.
        // Items are added to it by the allocator when allocations are allowed during concurrent sweep. The list is drained during the next sweep while
        // allocation are stopped.
        debugSweepableHeapBlockListLock.Enter();
#endif

        // Pull the blocks from the allocable SLIST that we didn't use. We need to finish the Pass-1 sweep of these blocks too.
        TBlockType * heapBlock = PopHeapBlockFromSList(this->allocableHeapBlockListHead);
        while (heapBlock != nullptr)
        {
            DebugOnly(this->AssertCheckHeapBlockNotInAnyList(heapBlock));
            if (heapBlock->isPendingConcurrentSweepPrep)
            {
#ifdef RECYCLER_TRACE
                heapBlock->GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**19**] ending sweep Pass1, removed from SLIST."));
#endif
                heapBlock->SetNextBlock(currentPendingSweepPrepHeapBlockList);
                currentPendingSweepPrepHeapBlockList = heapBlock;
            }
            else
            {
#ifdef RECYCLER_TRACE
                heapBlock->GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**23**] ending sweep Pass1, removed from SLIST and added to sweepableHeapBlockList."));
#endif
                // Already swept, put it back to the sweepableHeapBlockList list; so it can be processed later.
                heapBlock->SetNextBlock(this->sweepableHeapBlockList);
                this->sweepableHeapBlockList = heapBlock;
            }

            heapBlock = PopHeapBlockFromSList(this->allocableHeapBlockListHead);
        }
        Assert(QueryDepthInterlockedSList(this->allocableHeapBlockListHead) == 0);

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        debugSweepableHeapBlockListLock.Leave();
#endif

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

        // Start allocations now as we may start adding blocks to the SLIST during Pass1 sweep below.
        this->StartAllocationDuringConcurrentSweep();
        this->SweepHeapBlockList(recyclerSweep, currentPendingSweepPrepHeapBlockList, true /*allocable*/);
    }
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::EnsureAllocableHeapBlockList()
{
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        if (allocableHeapBlockListHead == nullptr)
        {
            allocableHeapBlockListHead = ((PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));

            if (allocableHeapBlockListHead == nullptr)
            {
                this->heapInfo->recycler->OutOfMemory();
            }
            else
            {
                ::InitializeSListHead(allocableHeapBlockListHead);
            }
        }
    }
#endif
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::FinishSweepPrep(RecyclerSweep& recyclerSweep)
{
    if (this->AllocationsStartedDuringConcurrentSweep())
    {
        AssertMsg(this->AllowAllocationsDuringConcurrentSweep(), "Why are allocations started during concurrent sweep, if not allowed?");
        Assert(!this->IsAnyFinalizableBucket());

        this->StopAllocationBeforeSweep();
        this->ClearAllocators();
    }
}

template <typename TBlockType>
void
HeapBucketT<TBlockType>::FinishConcurrentSweep()
{
    if (this->AllocationsStartedDuringConcurrentSweep())
    {
#if SUPPORT_WIN32_SLIST
        Assert(!this->IsAnyFinalizableBucket());
        Assert(this->allocableHeapBlockListHead != nullptr);

#ifdef RECYCLER_TRACE
        if (this->GetRecycler()->GetRecyclerFlagsTable().Trace.IsEnabled(Js::ConcurrentSweepPhase) && CONFIG_FLAG_RELEASE(Verbose))
        {
            CollectionState collectionState = this->GetRecycler()->collectionState;
            Output::Print(_u("[GC #%d] [HeapBucket 0x%p] starting FinishConcurrentSweep [CollectionState: %d] \n"), this->GetRecycler()->collectionCount, this, collectionState);
        }
#endif

        TBlockType * newNextAllocableBlockHead = nullptr;
        // Put the blocks from the allocable SLIST into the heapBlockList.
        TBlockType * heapBlock = PopHeapBlockFromSList(this->allocableHeapBlockListHead);
        while (heapBlock != nullptr)
        {
            DebugOnly(this->AssertCheckHeapBlockNotInAnyList(heapBlock));
            AssertMsg(!heapBlock->isPendingConcurrentSweepPrep, "The blocks in the SLIST at this time should NOT have sweep prep i.e. sweep-Pass1 pending.");
            newNextAllocableBlockHead = heapBlock;
            heapBlock->SetNextBlock(this->heapBlockList);
            this->heapBlockList = heapBlock;
#ifdef RECYCLER_TRACE
            this->GetRecycler()->PrintBlockStatus(this, heapBlock, _u("[**40**] finished FinishConcurrentSweep, heapblock removed from SLIST and added to heapBlockList."));
#endif
            heapBlock = PopHeapBlockFromSList(this->allocableHeapBlockListHead);
        }

        Assert(QueryDepthInterlockedSList(this->allocableHeapBlockListHead) == 0);

        this->ResumeNormalAllocationAfterConcurrentSweep(newNextAllocableBlockHead);
#endif

        Assert(!this->IsAllocationStopped());
    }
}
#endif

template <typename TBlockType>
void
HeapBucketT<TBlockType>::AppendAllocableHeapBlockList(TBlockType * list)
{
#ifdef RECYCLER_TRACE
    if (this->GetRecycler()->GetRecyclerFlagsTable().Trace.IsEnabled(Js::ConcurrentSweepPhase) && CONFIG_FLAG_RELEASE(Verbose))
    {
        CollectionState collectionState = this->GetRecycler()->collectionState;
        Output::Print(_u("[GC #%d] [HeapBucket 0x%p] in AppendAllocableHeapBlockList [CollectionState: %d] \n"), this->GetRecycler()->collectionCount, this, collectionState);
    }
#endif
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
        // Find the last block and append the list
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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && !this->IsAnyFinalizableBucket())
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
    Assert(this->GetRecycler()->recyclerSweepManager == nullptr);
    UpdateAllocators();
    size_t smallHeapBlockCount = HeapInfo::Check(true, false, this->fullBlockList);
    bool allocatingDuringConcurrentSweep = false;
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && !this->IsAnyFinalizableBucket())
    {
        allocatingDuringConcurrentSweep = true;
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
    Assert(!checkCount || this->heapBlockCount == smallHeapBlockCount || (this->heapBlockCount >= 65535 && allocatingDuringConcurrentSweep));
    return smallHeapBlockCount;
}
#endif

#if ENABLE_MEM_STATS
template <typename TBlockType>
void
HeapBucketT<TBlockType>::AggregateBucketStats()
{
    HeapBucket::AggregateBucketStats();  // call super

    auto allocatorHead = &this->allocatorHead;
    auto allocatorCurr = allocatorHead;

    do
    {
        TBlockType* allocatorHeapBlock = allocatorCurr->GetHeapBlock();
        if (allocatorHeapBlock)
        {
            allocatorHeapBlock->AggregateBlockStats(this->memStats, true, allocatorCurr->freeObjectList, allocatorCurr->endAddress != 0);
        }
        allocatorCurr = allocatorCurr->GetNext();
    } while (allocatorCurr != allocatorHead);

    auto blockStatsAggregator = [this](TBlockType* heapBlock) {
        heapBlock->AggregateBlockStats(this->memStats);
    };

    HeapBlockList::ForEach(fullBlockList, blockStatsAggregator);
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && !this->IsAnyFinalizableBucket())
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && !this->IsAnyFinalizableBucket())
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

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && !this->IsAnyFinalizableBucket())
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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.Initialize(heapInfo, sizeCat);
#endif
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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.ResetMarks(flags);
#endif
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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.Sweep(recyclerSweep);
#endif
#ifdef RECYCLER_WRITE_BARRIER
    smallFinalizableWithBarrierHeapBucket.Sweep(recyclerSweep);
#endif
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::DisposeObjects()
{
    finalizableHeapBucket.DisposeObjects();
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.DisposeObjects();
#endif
#ifdef RECYCLER_WRITE_BARRIER
    smallFinalizableWithBarrierHeapBucket.DisposeObjects();
#endif
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::TransferDisposedObjects()
{
    finalizableHeapBucket.TransferDisposedObjects();
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.TransferDisposedObjects();
#endif
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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.EnumerateObjects(infoBits, CallBackFunction);
#endif
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::FinalizeAllObjects()
{
    finalizableHeapBucket.FinalizeAllObjects();
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.FinalizeAllObjects();
#endif
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
#ifdef RECYCLER_VISITED_HOST
        recyclerVisitedHostHeapBucket.Rescan(recycler, flags) +
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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.PrepareSweep();
#endif
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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.SweepPartialReusePages(recyclerSweep);
#endif
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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.FinishPartialCollect(recyclerSweep);
#endif

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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.SweepPendingObjects(recyclerSweep);
#endif
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
#ifdef RECYCLER_VISITED_HOST
    recyclerSweep.TransferPendingEmptyHeapBlocks(&recyclerVisitedHostHeapBucket);
#endif
}
#endif

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
template <class TBlockAttributes>
size_t
HeapBucketGroup<TBlockAttributes>::GetNonEmptyHeapBlockCount(bool checkCount) const
{
    return heapBucket.GetNonEmptyHeapBlockCount(checkCount) +
        finalizableHeapBucket.GetNonEmptyHeapBlockCount(checkCount) +
#ifdef RECYCLER_VISITED_HOST
        recyclerVisitedHostHeapBucket.GetNonEmptyHeapBlockCount(checkCount) +
#endif
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
#ifdef RECYCLER_VISITED_HOST
        recyclerVisitedHostHeapBucket.GetEmptyHeapBlockCount() +
#endif
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
#ifdef RECYCLER_VISITED_HOST
        + recyclerVisitedHostHeapBucket.Check()
#endif
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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.Verify();
#endif
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
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostHeapBucket.VerifyMark();
#endif
    leafHeapBucket.VerifyMark();
#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.VerifyMark();
    smallFinalizableWithBarrierHeapBucket.VerifyMark();
#endif
}
#endif

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::StartAllocationDuringConcurrentSweep()
{
    // If there were no allocable heap blocks we would not have started allocations. Stop allocations, only if we started allocations for each of these buckets.
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
bool
HeapBucketGroup<TBlockAttributes>::DoTwoPassConcurrentSweepPreCheck()
{
    return heapBucket.DoTwoPassConcurrentSweepPreCheck() || 
    leafHeapBucket.DoTwoPassConcurrentSweepPreCheck()

#ifdef RECYCLER_WRITE_BARRIER
    || smallNormalWithBarrierHeapBucket.DoTwoPassConcurrentSweepPreCheck();
#endif
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::FinishConcurrentSweepPass1(RecyclerSweep& recyclerSweep)
{
    heapBucket.FinishConcurrentSweepPass1(recyclerSweep);
    leafHeapBucket.FinishConcurrentSweepPass1(recyclerSweep);

#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.FinishConcurrentSweepPass1(recyclerSweep);
#endif
}

template <class TBlockAttributes>
void
HeapBucketGroup<TBlockAttributes>::FinishSweepPrep(RecyclerSweep& recyclerSweep)
{
    heapBucket.FinishSweepPrep(recyclerSweep);
    leafHeapBucket.FinishSweepPrep(recyclerSweep);

#ifdef RECYCLER_WRITE_BARRIER
    smallNormalWithBarrierHeapBucket.FinishSweepPrep(recyclerSweep);
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
#ifdef RECYCLER_VISITED_HOST
        && recyclerVisitedHostHeapBucket.AllocatorsAreEmpty()
#endif
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

