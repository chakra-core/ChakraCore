//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{
class HeapInfo;
class RecyclerSweep;

// CONSIDER: Templatizing this so that we can have separate leaf large allocations
// and finalizable allocations
// CONSIDER: Templatizing this so that we don't have free list support if we don't need it
class LargeHeapBucket: public HeapBucket
{
public:
    LargeHeapBucket():
        supportFreeList(false),
        freeList(nullptr),
        explicitFreeList(nullptr),
        fullLargeBlockList(nullptr),
        largeBlockList(nullptr),
#ifdef RECYCLER_PAGE_HEAP
        largePageHeapBlockList(nullptr),
#endif
        pendingDisposeLargeBlockList(nullptr)
#if ENABLE_CONCURRENT_GC
        , pendingSweepLargeBlockList(nullptr)
#if ENABLE_PARTIAL_GC
        , partialSweptLargeBlockList(nullptr)
#endif
#endif
    {
    }

    ~LargeHeapBucket();

    void Initialize(HeapInfo * heapInfo, DECLSPEC_GUARD_OVERFLOW uint sizeCat, bool supportFreeList = false);

    LargeHeapBlock* AddLargeHeapBlock(DECLSPEC_GUARD_OVERFLOW size_t size, bool nothrow);

    bool SupportFreeList() { return supportFreeList; }

    template <ObjectInfoBits attributes, bool nothrow>
    char* Alloc(Recycler * recycler, size_t sizeCat);
#ifdef RECYCLER_PAGE_HEAP
    char *PageHeapAlloc(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, size_t size, ObjectInfoBits attributes, PageHeapMode mode, bool nothrow);
#endif
    void ExplicitFree(void * object, size_t sizeCat);

    void ResetMarks(ResetMarkFlags flags);
    void ScanInitialImplicitRoots(Recycler * recycler);
    void ScanNewImplicitRoots(Recycler * recycler);

    void Sweep(RecyclerSweep& recyclerSweep);
    void ReinsertLargeHeapBlock(LargeHeapBlock * heapBlock);

    void RegisterFreeList(LargeHeapBlockFreeList* freeList);
    void UnregisterFreeList(LargeHeapBlockFreeList* freeList);

    void FinalizeAllObjects();
    void Finalize();
    void DisposeObjects();
    void TransferDisposedObjects();

    void EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size));

    void Verify();
    void VerifyMark();
    void VerifyLargeHeapBlockCount();

    size_t Rescan(RescanFlags flags);
#if ENABLE_PARTIAL_GC || ENABLE_CONCURRENT_GC
    void SweepPendingObjects(RecyclerSweep& recyclerSweep);
#if ENABLE_PARTIAL_GC
    void FinishPartialCollect(RecyclerSweep * recyclerSweep);
#endif

#if ENABLE_CONCURRENT_GC
    void ConcurrentTransferSweptObjects(RecyclerSweep& recyclerSweep);
#if ENABLE_PARTIAL_GC
    void ConcurrentPartialTransferSweptObjects(RecyclerSweep& recyclerSweep);
#endif
#endif
#endif

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    size_t GetLargeHeapBlockCount(bool checkCount = false) const;
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    size_t Check();
    template <typename TBlockType>
    size_t Check(bool expectFull, bool expectPending, TBlockType * list, TBlockType * tail = nullptr);
#endif
#endif

private:
    char * SnailAlloc(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, size_t size, ObjectInfoBits attributes, bool nothrow);
    char * TryAlloc(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, ObjectInfoBits attributes);
    char * TryAllocFromNewHeapBlock(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, size_t size, ObjectInfoBits attributes, bool nothrow);
    char * TryAllocFromFreeList(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, ObjectInfoBits attributes);
    char * TryAllocFromExplicitFreeList(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, ObjectInfoBits attributes);

    template <class Fn> void ForEachLargeHeapBlock(Fn fn);
    template <class Fn> void ForEachEditingLargeHeapBlock(Fn fn);
    void Finalize(Recycler* recycler, LargeHeapBlock* heapBlock);
    void SweepLargeHeapBlockList(RecyclerSweep& recyclerSweep, LargeHeapBlock * heapBlockList);

    void ConstructFreelist(LargeHeapBlock * heapBlock);

    size_t Rescan(LargeHeapBlock * list, Recycler * recycler, bool isPartialSwept, RescanFlags flags);

    LargeHeapBlock * fullLargeBlockList;
    LargeHeapBlock * largeBlockList;
#ifdef RECYCLER_PAGE_HEAP
    LargeHeapBlock * largePageHeapBlockList;
#endif
    LargeHeapBlock * pendingDisposeLargeBlockList;
#if ENABLE_CONCURRENT_GC
    LargeHeapBlock * pendingSweepLargeBlockList;
#if ENABLE_PARTIAL_GC
    // Used for concurrent-partial GC
    LargeHeapBlock * partialSweptLargeBlockList;
#endif
#endif
    bool supportFreeList;
    LargeHeapBlockFreeList* freeList;
    FreeObject * explicitFreeList;


    friend class HeapInfo;
    friend class Recycler;
    friend class ::ScriptMemoryDumper;
};
}

