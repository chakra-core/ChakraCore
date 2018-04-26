//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{

class RecyclerSweepManager;
class HeapInfoManager
{
public:
    HeapInfoManager();

    HeapInfo * GetDefaultHeap()
    {
        return &defaultHeap;
    }

    HeapInfo * GetIsolatedHeap()
    {
        return &isolatedHeap;
    }

    void Initialize(Recycler * recycler
#ifdef RECYCLER_PAGE_HEAP
        , PageHeapMode pageheapmode = PageHeapMode::PageHeapModeOff
        , bool captureAllocCallStack = false
        , bool captureFreeCallStack = false
#endif
    );

#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY) || defined(MEMSPECT_TRACKING) || defined(ETW_MEMORY_TRACKING)
    void Initialize(Recycler * recycler, void(*trackNativeAllocCallBack)(Recycler *, void *, size_t)
#ifdef RECYCLER_PAGE_HEAP
        , PageHeapMode pageheapmode = PageHeapMode::PageHeapModeOff
        , bool captureAllocCallStack = false
        , bool captureFreeCallStack = false
#endif
    );
#endif

    bool IsPageHeapEnabled();

    void ScanInitialImplicitRoots();
    void ScanNewImplicitRoots();
    void ResetMarks(ResetMarkFlags flags);
    size_t Rescan(RescanFlags flags);
    
    void FinalizeAndSweep(RecyclerSweepManager& recyclerSweepManager, bool concurrent);

    void SweepSmallNonFinalizable(RecyclerSweepManager& recyclerSweepManager);

#if ENABLE_PARTIAL_GC || ENABLE_CONCURRENT_GC
    void SweepPendingObjects(RecyclerSweepManager& recyclerSweepManager);
#endif
    
#if ENABLE_PARTIAL_GC
    void SweepPartialReusePages(RecyclerSweepManager& recyclerSweepManager);
    void FinishPartialCollect(RecyclerSweepManager * recyclerSweepManager);
#endif
#if ENABLE_CONCURRENT_GC
    void PrepareSweep();
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    void StartAllocationsDuringConcurrentSweep();
    bool DoTwoPassConcurrentSweepPreCheck();
    void FinishSweepPrep(RecyclerSweepManager& recyclerSweep);
    void FinishConcurrentSweepPass1(RecyclerSweepManager& recyclerSweep);
    void FinishConcurrentSweep();
#endif

    void ConcurrentTransferSweptObjects(RecyclerSweepManager& recyclerSweepManager);

#if ENABLE_PARTIAL_GC
    void ConcurrentPartialTransferSweptObjects(RecyclerSweepManager& recyclerSweepManager);
#endif
#endif

    void DisposeObjects();
    void TransferDisposedObjects();

    void EnumerateObjects(ObjectInfoBits infoBits, void(*CallBackFunction)(void * address, size_t size));
#if DBG
    bool AllocatorsAreEmpty();
#endif
#ifdef RECYCLER_FINALIZE_CHECK
    size_t GetFinalizeCount();
#endif
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    void Check();
#endif
#if ENABLE_MEM_STATS
    void ReportMemStats(Recycler * recycler);
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    void Verify();
#endif
#ifdef RECYCLER_VERIFY_MARK
    void VerifyMark();
#endif
private:
    template <class Fn>
    void ForEachHeapInfo(Fn fn);
    template <class Fn>
    void ForEachHeapInfo(RecyclerSweepManager& recyclerSweepManager, Fn fn);
    template <class Fn>
    void ForEachHeapInfo(RecyclerSweepManager * recyclerSweepManager, Fn fn);
    template <class Fn>
    bool AreAllHeapInfo(Fn fn);
    template <class Fn>
    bool IsAnyHeapInfo(Fn fn);

    friend class Recycler;
    friend class HeapBucket;
    friend class HeapBlockMap32;
    friend class LargeHeapBucket;
    template <typename TBlockType>
    friend class HeapBucketT;
    template <typename TBlockType>
    friend class SmallHeapBlockAllocator;
    template <typename TBlockAttributes>
    friend class SmallFinalizableHeapBucketT;

    template <typename TBlockAttributes>
    friend class SmallLeafHeapBucketBaseT;

    template <typename TBlockAttributes>
    friend class SmallHeapBlockT;
    template <typename TBlockAttributes>
    friend class SmallLeafHeapBlockT;
    template <typename TBlockAttributes>
    friend class SmallFinalizableHeapBlockT;
#ifdef RECYCLER_VISITED_HOST
    template <typename TBlockAttributes>
    friend class SmallRecyclerVisitedHostHeapBlockT;
#endif
    friend class LargeHeapBlock;
    friend class HeapInfo;
    friend class RecyclerSweepManager;

    HeapInfo defaultHeap;
    HeapInfo isolatedHeap;


    size_t uncollectedAllocBytes;
    size_t lastUncollectedAllocBytes;
    size_t uncollectedExternalBytes;
    uint pendingZeroPageCount;
#if ENABLE_PARTIAL_GC
    size_t uncollectedNewPageCount;
    size_t unusedPartialCollectFreeBytes;
#endif
};
}