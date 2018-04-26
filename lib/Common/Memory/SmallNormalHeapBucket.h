//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{
template <typename TBlockType>
class SmallNormalHeapBucketBase : public HeapBucketT<TBlockType>
{
    typedef HeapBucketT<TBlockType> BaseT;
public:
    typedef typename TBlockType::HeapBlockAttributes TBlockAttributes;

    SmallNormalHeapBucketBase();

    CompileAssert(!BaseT::IsLeafBucket);
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    friend class ::ScriptMemoryDumper;
#endif

#if ENABLE_MEM_STATS
    void AggregateBucketStats();
#endif
protected:
    template <class TBlockAttributes>
    friend class HeapBucketGroup;
    friend class HeapInfo;
    friend class HeapBlockMap32;

    void ScanInitialImplicitRoots(Recycler * recycler);
    void ScanNewImplicitRoots(Recycler * recycler);

    static bool RescanObjectsOnPage(TBlockType * block, char * address, char * blockStartAddress, BVStatic<TBlockAttributes::BitVectorCount>* markBits, const uint localObjectSize, uint bucketIndex, __out_opt bool* anyObjectRescanned, Recycler* recycler);

#if ENABLE_CONCURRENT_GC
    void SweepPendingObjects(RecyclerSweep& recyclerSweep);
    template <SweepMode mode>
    TBlockType * SweepPendingObjects(Recycler * recycler, TBlockType * list);
#endif
    void Sweep(RecyclerSweep& recyclerSweep);
#if ENABLE_PARTIAL_GC
    ~SmallNormalHeapBucketBase();

    template <class Fn>
    void SweepPartialReusePages(RecyclerSweep& recyclerSweep, TBlockType * heapBlockList,
        TBlockType *& reuseBlocklist, TBlockType *&unusedBlockList, bool allocationsAllowedDuringConcurrentSweep, Fn callBack);
    void SweepPartialReusePages(RecyclerSweep& recyclerSweep);
    void FinishPartialCollect(RecyclerSweep * recyclerSweep);

    void EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size));

#if DBG
    void ResetMarks(ResetMarkFlags flags);
    static void SweepVerifyPartialBlocks(Recycler * recycler, TBlockType * heapBlockList);
#endif
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    size_t GetNonEmptyHeapBlockCount(bool checkCount) const;
#endif
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    size_t Check(bool checkCount = true);
    friend class HeapBucketT<TBlockType>;
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    void Verify();
#endif
#ifdef RECYCLER_VERIFY_MARK
    void VerifyMark();
#endif
protected:
    TBlockType * partialHeapBlockList;      // list of blocks that is partially collected
                                            // This list exists to keep track of heap blocks that
                                            // are not full but don't have a large amount of free space
                                            // where allocating from it causing a write watch to be triggered
                                            // is not worth the effort
#if ENABLE_CONCURRENT_GC
    TBlockType * partialSweptHeapBlockList; // list of blocks that is partially swept
#endif
#endif
};

template <typename TBlockAttributes>
class SmallNormalHeapBucketT : public SmallNormalHeapBucketBase<SmallNormalHeapBlockT<TBlockAttributes>> {};

typedef SmallNormalHeapBucketT<SmallAllocationBlockAttributes> SmallNormalHeapBucket;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
typedef SmallNormalHeapBucketT<MediumAllocationBlockAttributes> MediumNormalHeapBucket;
#endif

#ifdef RECYCLER_WRITE_BARRIER
template <typename TBlockAttributes>
class SmallNormalWithBarrierHeapBucketT : public SmallNormalHeapBucketBase<SmallNormalWithBarrierHeapBlockT<TBlockAttributes>>
{
public:
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    void Initialize(HeapInfo * heapInfo, uint sizeCat, ushort objectAlignment)
#else
    void Initialize(HeapInfo * heapInfo, uint sizeCat)
#endif
    {
        CompileAssert(SmallNormalWithBarrierHeapBucketT::IsLeafBucket == false);
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
        __super::Initialize(heapInfo, sizeCat, objectAlignment);
#else
        __super::Initialize(heapInfo, sizeCat);
#endif
    }
};

typedef SmallNormalWithBarrierHeapBucketT<SmallAllocationBlockAttributes> SmallNormalWithBarrierHeapBucket;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
typedef SmallNormalWithBarrierHeapBucketT<MediumAllocationBlockAttributes> MediumNormalWithBarrierHeapBucket;
#endif

#endif

extern template class SmallNormalHeapBucketBase<SmallNormalHeapBlock>;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
extern template class SmallNormalHeapBucketBase<MediumNormalHeapBlock>;
#endif

#ifdef RECYCLER_WRITE_BARRIER
extern template class SmallNormalHeapBucketBase<SmallNormalWithBarrierHeapBlock>;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
extern template class SmallNormalHeapBucketBase<MediumNormalWithBarrierHeapBlock>;
#endif
#endif
}
