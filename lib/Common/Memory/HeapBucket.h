//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{
// a size bucket in the heap for small object

class HeapInfo;
#if DBG
template <class TBlockAttributes>
class GenericRecyclerVerifyListConsistencyData
{
public:
    GenericRecyclerVerifyListConsistencyData() {};

    // Temporary data for Sweep list consistency checks
    bool expectFull;
    bool expectDispose;
    bool hasSetupVerifyListConsistencyData;
    SmallHeapBlockT<TBlockAttributes> * nextAllocableBlockHead;

    template <typename TBlockAttr>
    void SetupVerifyListConsistencyData(SmallHeapBlockT<TBlockAttr>* block, bool expectFull, bool expectDispose)
    {
        this->nextAllocableBlockHead = block;
        this->expectFull = expectFull;
        this->expectDispose = expectDispose;
        this->hasSetupVerifyListConsistencyData = true;
    }
};

class RecyclerVerifyListConsistencyData
{
public:
    RecyclerVerifyListConsistencyData() {};
    void SetupVerifyListConsistencyDataForSmallBlock(SmallHeapBlock* block, bool expectFull, bool expectDispose)
    {
        this->smallBlockVerifyListConsistencyData.nextAllocableBlockHead = block;
        this->smallBlockVerifyListConsistencyData.expectFull = expectFull;
        this->smallBlockVerifyListConsistencyData.expectDispose = expectDispose;
        this->smallBlockVerifyListConsistencyData.hasSetupVerifyListConsistencyData = true;
    }

#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    void SetupVerifyListConsistencyDataForMediumBlock(MediumHeapBlock* block, bool expectFull, bool expectDispose)
    {
        this->mediumBlockVerifyListConsistencyData.nextAllocableBlockHead = block;
        this->mediumBlockVerifyListConsistencyData.expectFull = expectFull;
        this->mediumBlockVerifyListConsistencyData.expectDispose = expectDispose;
        this->mediumBlockVerifyListConsistencyData.hasSetupVerifyListConsistencyData = true;
    }
#endif

    GenericRecyclerVerifyListConsistencyData<SmallAllocationBlockAttributes>  smallBlockVerifyListConsistencyData;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    GenericRecyclerVerifyListConsistencyData<MediumAllocationBlockAttributes> mediumBlockVerifyListConsistencyData;
#endif
};

#endif

class HeapBucketObjectGranularityInfo
{
public:
    ushort bucketSizeCat;
    ushort objectGranularity;
};

// NOTE: HeapBucket can't have vtable, because we allocate them inline with recycler with custom initializer
class HeapBucket
{
public:
    HeapBucket();

    HeapInfo * GetHeapInfo() const;
    uint GetSizeCat() const;
    ushort GetBucketIndex() const;
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    uint GetMediumBucketIndex() const;
#endif

    template <typename TBlockType>
    static void EnumerateObjects(TBlockType * heapBlockList, ObjectInfoBits infoBits, void(*CallBackFunction)(void * address, size_t size));

protected:
    HeapInfo * heapInfo;
    uint sizeCat;
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    ushort bucketIndex;
    ushort objectAlignment;
#endif

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    bool allocationsStartedDuringConcurrentSweep;
    bool concurrentSweepAllocationsThresholdExceeded;
#endif

#if defined(RECYCLER_SLOW_CHECK_ENABLED) || ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    uint32 heapBlockCount;
    uint32 newHeapBlockCount;       // count of heap bock that is in the heap info and not in the heap bucket yet
#endif

#if defined(RECYCLER_SLOW_CHECK_ENABLED)
    uint32 emptyHeapBlockCount;
#endif

#ifdef RECYCLER_PAGE_HEAP
protected:
    bool isPageHeapEnabled;
public:
    inline bool IsPageHeapEnabled(ObjectInfoBits attributes) const
    {
        // LargeHeapBlock does not support TrackBit today
        return isPageHeapEnabled && ((attributes & ClientTrackableObjectBits) == 0);
    }
#endif

#if ENABLE_MEM_STATS
protected:
    HeapBucketStats memStats;  // mem stats per bucket
public:
    const HeapBucketStats& GetMemStats() const { return memStats; }

    template <typename TBlockType>
    void PreAggregateBucketStats(TBlockType* heapBlock)
    {
        Assert(heapBlock->heapBucket == this);
        memStats.PreAggregate();
        heapBlock->AggregateBlockStats(memStats);
    }

    void AggregateBucketStats()
    {
        memStats.BeginAggregate();  // Begin aggregate, clear if needed
    }
#endif

    Recycler * GetRecycler() const;
    bool AllocationsStartedDuringConcurrentSweep() const;
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    bool ConcurrentSweepAllocationsThresholdExceeded() const;
    bool DoTwoPassConcurrentSweepPreCheck();
#endif

    template <typename TBlockType>
    friend class SmallHeapBlockAllocator;

    template <typename TBlockAttributes>
    friend class SmallHeapBlockT;

    template <typename TBlockAttributes>
    friend class SmallFinalizableHeapBlockT;

#ifdef RECYCLER_VISITED_HOST
    template <typename TBlockAttributes>
    friend class SmallRecyclerVisitedHostHeapBlockT;
#endif

    friend class LargeHeapBlock;
#ifdef RECYCLER_WRITE_BARRIER
    template <typename TBlockAttributes>
    friend class SmallFinalizableWithBarrierHeapBlockT;

    template <typename TBlockAttributes>
    friend class SmallNormalWithBarrierHeapBlockT;
#endif
};

template <typename TBlockType>
class HeapBucketT : public HeapBucket
{
    typedef SmallHeapBlockAllocator<TBlockType> TBlockAllocatorType;

public:
    HeapBucketT();
    ~HeapBucketT();

    bool IntegrateBlock(char * blockAddress, PageSegment * segment, Recycler * recycler);

    template <ObjectInfoBits attributes, bool nothrow>
    inline char * RealAlloc(Recycler * recycler, size_t sizeCat, size_t size);

#ifdef RECYCLER_PAGE_HEAP
    char * PageHeapAlloc(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, size_t size, ObjectInfoBits attributes, PageHeapMode mode, bool nothrow);
#endif

    void ExplicitFree(void* object, size_t sizeCat);

    char * SnailAlloc(Recycler * recycler, TBlockAllocatorType * allocator, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, size_t size, ObjectInfoBits attributes, bool nothrow);

    void ResetMarks(ResetMarkFlags flags);
    void ScanNewImplicitRoots(Recycler * recycler);

#if ENABLE_MEM_STATS
    void AggregateBucketStats();
#endif

    uint Rescan(Recycler * recycler, RescanFlags flags);
#if ENABLE_CONCURRENT_GC
    void MergeNewHeapBlock(TBlockType * heapBlock);
    void PrepareSweep();
    void SetupBackgroundSweep(RecyclerSweep& recyclerSweep);
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    friend class ::ScriptMemoryDumper;
#endif

    static bool IsAnyFinalizableBucket()
    {
        return IsFinalizableBucket
#ifdef RECYCLER_WRITE_BARRIER
            || IsFinalizableWriteBarrierBucket
#endif
            ;
    }

    TBlockAllocatorType * GetAllocator() { return &allocatorHead;}

    static unsigned int GetAllocatorHeadOffset() { return offsetof(HeapBucketT<TBlockType>, allocatorHead); }

protected:
    static bool const IsLeafBucket = TBlockType::RequiredAttributes == LeafBit;
    // Not all objects in the recycler visited host heap block are finalizable, but we still require finalizable semantics
    static bool const IsFinalizableBucket = TBlockType::RequiredAttributes == FinalizeBit
#ifdef RECYCLER_VISITED_HOST
        || ((TBlockType::RequiredAttributes & RecyclerVisitedHostBit) == (RecyclerVisitedHostBit))
#endif
    ;
    static bool const IsNormalBucket = TBlockType::RequiredAttributes == NoBit;
#ifdef RECYCLER_WRITE_BARRIER
    static bool const IsWriteBarrierBucket = TBlockType::RequiredAttributes == WithBarrierBit;
    static bool const IsFinalizableWriteBarrierBucket = TBlockType::RequiredAttributes == FinalizableWithBarrierBit;
#endif

#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    void Initialize(HeapInfo * heapInfo, DECLSPEC_GUARD_OVERFLOW uint sizeCat, DECLSPEC_GUARD_OVERFLOW ushort objectAlignment);
#else
    void Initialize(HeapInfo * heapInfo, DECLSPEC_GUARD_OVERFLOW uint sizeCat);
#endif
    void AppendAllocableHeapBlockList(TBlockType * list);
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    void EnsureAllocableHeapBlockList();
    void FinishSweepPrep(RecyclerSweep& recyclerSweep);
    void FinishConcurrentSweepPass1(RecyclerSweep& recyclerSweep);
    void FinishConcurrentSweep();
#endif
    void DeleteHeapBlockList(TBlockType * list);
    static void DeleteEmptyHeapBlockList(TBlockType * list);
    static void DeleteHeapBlockList(TBlockType * list, Recycler * recycler);
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
    static bool PushHeapBlockToSList(PSLIST_HEADER list, TBlockType * heapBlock);
    static TBlockType * PopHeapBlockFromSList(PSLIST_HEADER list);
    static ushort QueryDepthInterlockedSList(PSLIST_HEADER list);
    static void FlushInterlockedSList(PSLIST_HEADER list);
#endif

    // Small allocators
    void UpdateAllocators();
    void ClearAllocators();
    void AddAllocator(TBlockAllocatorType * allocator);
    void RemoveAllocator(TBlockAllocatorType * allocator);
    static void ClearAllocator(TBlockAllocatorType * allocator);
    template <class Fn> void ForEachAllocator(Fn fn);

    // Allocations
    char * TryAllocFromNewHeapBlock(Recycler * recycler, TBlockAllocatorType * allocator, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, size_t size, ObjectInfoBits attributes);
    char * TryAlloc(Recycler * recycler, TBlockAllocatorType * allocator, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, ObjectInfoBits attributes);
    TBlockType * CreateHeapBlock(Recycler * recycler);
    TBlockType * GetUnusedHeapBlock();

    void FreeHeapBlock(TBlockType * heapBlock);

    // GC
    void SweepBucket(RecyclerSweep& recyclerSweep);
    template <typename Fn>
    void SweepBucket(RecyclerSweep& recyclerSweep, Fn sweepFn);

#if ENABLE_CONCURRENT_GC  && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    void PrepareForAllocationsDuringConcurrentSweep(TBlockType * &currentHeapBlockList);
    void StartAllocationDuringConcurrentSweep();
    void ResumeNormalAllocationAfterConcurrentSweep(TBlockType * newNextAllocableBlockHead = nullptr);
#endif

    bool AllowAllocationsDuringConcurrentSweep();
    void StopAllocationBeforeSweep();
    void StartAllocationAfterSweep();
    bool IsAllocationStopped() const;

    void SweepHeapBlockList(RecyclerSweep& recyclerSweep, TBlockType * heapBlockList, bool allocable);
#if ENABLE_PARTIAL_GC
    bool DoQueuePendingSweep(Recycler * recycler);
    bool DoPartialReuseSweep(Recycler * recycler);
#endif

    // Partial/Concurrent GC
    void EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size));


    void AssertCheckHeapBlockNotInAnyList(TBlockType * heapBlock);
#if DBG
    bool AllocatorsAreEmpty() const;
    bool HasPendingDisposeHeapBlocks() const;

    static void VerifyBlockConsistencyInList(TBlockType * heapBlock, RecyclerVerifyListConsistencyData& recyclerSweep);
    static void VerifyBlockConsistencyInList(TBlockType * heapBlock, RecyclerVerifyListConsistencyData const& recyclerSweep, SweepState state);
#endif
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    // TODO-REFACTOR: This really should be virtual
    size_t GetNonEmptyHeapBlockCount(bool checkCount) const;
    size_t GetEmptyHeapBlockCount() const;
#endif
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    size_t Check(bool checkCount = true);
    void VerifyHeapBlockCount(bool background);
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    void Verify();
#endif
#ifdef RECYCLER_VERIFY_MARK
    void VerifyMark();
#endif
    TBlockAllocatorType allocatorHead;
    TBlockType * nextAllocableBlockHead;
    TBlockType * emptyBlockList;     // list of blocks that is empty and has it's page freed

    TBlockType * fullBlockList;      // list of blocks that are fully allocated
    TBlockType * heapBlockList;      // list of blocks that has free objects

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if SUPPORT_WIN32_SLIST
    PSLIST_HEADER allocableHeapBlockListHead;
    TBlockType * lastKnownNextAllocableBlockHead;
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    // This lock is needed only in the debug mode while we verify block counts. Not needed otherwise, as this list is never accessed concurrently.
    // Items are added to it by the allocator when allocations are allowed during concurrent sweep. The list is drained during the next sweep while
    // allocation are stopped.
    mutable CriticalSection debugSweepableHeapBlockListLock;
#endif
    // This is the list of blocks that we allocated from during concurrent sweep. These blocks will eventually get processed during the next sweep and either go into
    // the fullBlockList.
    TBlockType * sweepableHeapBlockList;
#endif
#endif

    FreeObject* explicitFreeList; // List of objects that have been explicitly freed
    TBlockAllocatorType * lastExplicitFreeListAllocator;
#ifdef RECYCLER_PAGE_HEAP
    SmallHeapBlock* explicitFreeLockBlockList; // List of heap blocks which have been locked upon explicit free
#endif

    bool isAllocationStopped;                 // whether the bucket has it's allocations stopped

    template <class TBlockAttributes>
    friend class HeapBucketGroup;

    friend class HeapInfo;
    friend TBlockType;

    template <class TBucketAttributes>
    friend class SmallHeapBlockT;
    friend class RecyclerSweep;
};

template <typename TBlockType>
void
HeapBucket::EnumerateObjects(TBlockType * heapBlockList, ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size))
{
    HeapBlockList::ForEach(heapBlockList, [=](TBlockType * heapBlock)
    {
        heapBlock->EnumerateObjects(infoBits, CallBackFunction);
    });
}


template <typename TBlockType>
template <typename Fn>
void
HeapBucketT<TBlockType>::SweepBucket(RecyclerSweep& recyclerSweep, Fn sweepFn)
{
    this->SweepBucket(recyclerSweep);

    // Continue to sweep other list from derived class
    sweepFn(recyclerSweep);

#if ENABLE_PARTIAL_GC
    if (!this->DoPartialReuseSweep(recyclerSweep.GetRecycler()))
#endif
    {
#if ENABLE_CONCURRENT_GC
        // We should only queue up pending sweep if we are doing partial collect
        Assert(recyclerSweep.GetPendingSweepBlockList(this) == nullptr);

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
        if (!this->AllocationsStartedDuringConcurrentSweep())
#endif
#endif
        {
            // Every thing is swept immediately in non partial collect, so we can allocate
            // from the heap block list now
            StartAllocationAfterSweep();
        }
    }

    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(recyclerSweep.IsBackground()));
}
}

