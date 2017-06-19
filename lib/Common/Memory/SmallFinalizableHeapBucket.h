//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Memory
{
template <class THeapBlockType>
class SmallFinalizableHeapBucketBaseT : public SmallNormalHeapBucketBase<THeapBlockType>
{
    typedef SmallNormalHeapBucketBase<THeapBlockType> BaseT;

public:
    typedef typename THeapBlockType::HeapBlockAttributes TBlockAttributes;

    SmallFinalizableHeapBucketBaseT();
    ~SmallFinalizableHeapBucketBaseT();
    void DisposeObjects();
    void TransferDisposedObjects();
    void FinalizeAllObjects();
    static void FinalizeHeapBlockList(THeapBlockType * list);

#ifdef DUMP_FRAGMENTATION_STATS
    void AggregateBucketStats(HeapBucketStats& stats);
#endif
protected:
    void EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size));

    friend class HeapBucket;
    template <class TBlockAttributes>
    friend class HeapBucketGroup;

    void ResetMarks(ResetMarkFlags flags);
    void Sweep(RecyclerSweep& recyclerSweep);
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    size_t GetNonEmptyHeapBlockCount(bool checkCount) const;
#endif
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    size_t Check();
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    void Verify();
#endif
#ifdef RECYCLER_VERIFY_MARK
    void VerifyMark();
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    friend class ::ScriptMemoryDumper;
#endif
    template <typename TBlockType>
    friend class HeapBucketT;

protected:
    THeapBlockType * pendingDisposeList;    // list of block that has finalizable object that needs to be disposed

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    THeapBlockType * tempPendingDisposeList;
#endif
};

template <class TBlockAttributes> 
class SmallFinalizableHeapBucketT : public SmallFinalizableHeapBucketBaseT<SmallFinalizableHeapBlockT<TBlockAttributes> >
{
};
#ifdef RECYCLER_WRITE_BARRIER
template <class TBlockAttributes> 
class SmallFinalizableWithBarrierHeapBucketT : public SmallFinalizableHeapBucketBaseT<SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes> >
{
};
#endif

typedef SmallFinalizableHeapBucketT<MediumAllocationBlockAttributes> MediumFinalizableHeapBucket;
typedef SmallFinalizableHeapBucketT<SmallAllocationBlockAttributes> SmallFinalizableHeapBucket;
#ifdef RECYCLER_WRITE_BARRIER
typedef SmallFinalizableWithBarrierHeapBucketT<MediumAllocationBlockAttributes> MediumFinalizableWithBarrierHeapBucket;
typedef SmallFinalizableWithBarrierHeapBucketT<SmallAllocationBlockAttributes> SmallFinalizableWithBarrierHeapBucket;
#endif

// GC-TODO: Move this away

template <ObjectInfoBits attributes, class TBlockAttributes>
class SmallHeapBlockType
{
public:
    CompileAssert(attributes & FinalizeBit);
    typedef SmallFinalizableHeapBlockT<TBlockAttributes> BlockType;
    typedef SmallFinalizableHeapBucketT<TBlockAttributes> BucketType;
};

template <>
class SmallHeapBlockType<LeafBit, SmallAllocationBlockAttributes>
{
public:
    typedef SmallLeafHeapBlock BlockType;
    typedef SmallLeafHeapBucketT<SmallAllocationBlockAttributes> BucketType;
};

template <>
class SmallHeapBlockType<NoBit, SmallAllocationBlockAttributes>
{
public:
    typedef SmallNormalHeapBlock BlockType;
    typedef SmallNormalHeapBucket BucketType;
};

#ifdef RECYCLER_WRITE_BARRIER
template <>
class SmallHeapBlockType<WithBarrierBit, SmallAllocationBlockAttributes>
{
public:
    typedef SmallNormalWithBarrierHeapBlock BlockType;
    typedef SmallNormalWithBarrierHeapBucket BucketType;
};

template <>
class SmallHeapBlockType<(ObjectInfoBits)(WithBarrierBit|LeafBit), SmallAllocationBlockAttributes>
{
public:
    typedef SmallLeafHeapBlock BlockType;
    typedef SmallLeafHeapBucketT<SmallAllocationBlockAttributes> BucketType;
};

template <>
class SmallHeapBlockType<FinalizableWithBarrierBit, SmallAllocationBlockAttributes>
{
public:
    typedef SmallFinalizableWithBarrierHeapBlock BlockType;
    typedef SmallFinalizableWithBarrierHeapBucket BucketType;
};
#endif

#if SMALLBLOCK_MEDIUM_ALLOC
template <>
class SmallHeapBlockType<FinalizeBit, MediumAllocationBlockAttributes>
{
public:
    typedef MediumFinalizableHeapBlock BlockType;
    typedef MediumFinalizableHeapBucket BucketType;
};

template <>
class SmallHeapBlockType<LeafBit, MediumAllocationBlockAttributes>
{
public:
    typedef MediumLeafHeapBlock BlockType;
    typedef SmallLeafHeapBucketT<MediumAllocationBlockAttributes> BucketType;
};

template <>
class SmallHeapBlockType<NoBit, MediumAllocationBlockAttributes>
{
public:
    typedef MediumNormalHeapBlock BlockType;
    typedef MediumNormalHeapBucket BucketType;
};

#ifdef RECYCLER_WRITE_BARRIER
template <>
class SmallHeapBlockType<WithBarrierBit, MediumAllocationBlockAttributes>
{
public:
    typedef MediumNormalWithBarrierHeapBlock BlockType;
    typedef MediumNormalWithBarrierHeapBucket BucketType;
};

template <>
class SmallHeapBlockType<(ObjectInfoBits)(WithBarrierBit | LeafBit), MediumAllocationBlockAttributes>
{
public:
    typedef MediumLeafHeapBlock BlockType;
    typedef SmallLeafHeapBucketT<MediumAllocationBlockAttributes> BucketType;
};

template <>
class SmallHeapBlockType<FinalizableWithBarrierBit, MediumAllocationBlockAttributes>
{
public:
    typedef MediumFinalizableWithBarrierHeapBlock BlockType;
    typedef MediumFinalizableWithBarrierHeapBucket BucketType;
};
#endif
#endif

template <class TBlockAttributes>
class HeapBucketGroup
{
    template <ObjectInfoBits objectAttributes>
    class BucketGetter
    {
    public:
        typedef typename SmallHeapBlockType<objectAttributes, TBlockAttributes>::BucketType BucketType;
        static BucketType& GetBucket(HeapBucketGroup<TBlockAttributes> * heapBucketGroup)
        {
            CompileAssert(objectAttributes & FinalizeBit);
            return heapBucketGroup->finalizableHeapBucket;
        }
    };

    template <>
    class BucketGetter<LeafBit>
    {
    public:
        typedef typename SmallHeapBlockType<LeafBit, TBlockAttributes>::BucketType BucketType;
        static BucketType& GetBucket(HeapBucketGroup<TBlockAttributes> * heapBucketGroup)
        {            
            return heapBucketGroup->leafHeapBucket;
        }
    };

    template <>
    class BucketGetter<NoBit>
    {
    public:
        typedef typename SmallHeapBlockType<NoBit, TBlockAttributes>::BucketType BucketType;
        static BucketType& GetBucket(HeapBucketGroup<TBlockAttributes> * heapBucketGroup)
        {
            return heapBucketGroup->heapBucket;
        }
    };

    template <>
    class BucketGetter<(ObjectInfoBits)(FinalizeBit | LeafBit)>
    {
    public:
        typedef typename SmallHeapBlockType<(ObjectInfoBits)(FinalizeBit | LeafBit), TBlockAttributes>::BucketType BucketType;
        static BucketType& GetBucket(HeapBucketGroup<TBlockAttributes> * heapBucketGroup)
        {
            // TODO: SWB implemente finalizable leaf bucket
            return heapBucketGroup->finalizableHeapBucket;
        }
    };

#ifdef RECYCLER_WRITE_BARRIER
    template <>
    class BucketGetter<WithBarrierBit>
    {
    public:
        typedef typename SmallHeapBlockType<WithBarrierBit, TBlockAttributes>::BucketType BucketType;
        static BucketType& GetBucket(HeapBucketGroup<TBlockAttributes> * heapBucketGroup)
        {
            return heapBucketGroup->smallNormalWithBarrierHeapBucket;
        }
    };

    template <>
    class BucketGetter<(ObjectInfoBits)(WithBarrierBit | LeafBit)>
    {
    public:
        typedef typename SmallHeapBlockType<(ObjectInfoBits)(WithBarrierBit | LeafBit), TBlockAttributes>::BucketType BucketType;
        static BucketType& GetBucket(HeapBucketGroup<TBlockAttributes> * heapBucketGroup)
        {
            // WithBarrierBit | LeafBit combination should not exist, this is only for compilation purpose
            Assert(false);
            return heapBucketGroup->leafHeapBucket;
        }
    };

    template <>
    class BucketGetter<FinalizableWithBarrierBit>
    {
    public:
        typedef typename SmallHeapBlockType<FinalizableWithBarrierBit, TBlockAttributes>::BucketType BucketType;
        static BucketType& GetBucket(HeapBucketGroup<TBlockAttributes> * heapBucketGroup)
        {
            return heapBucketGroup->smallFinalizableWithBarrierHeapBucket;
        }
    };
#endif
public:

    template <ObjectInfoBits objectAttributes>
    typename SmallHeapBlockType<objectAttributes, TBlockAttributes>::BucketType& GetBucket()
    {
        return BucketGetter<objectAttributes>::GetBucket(this);
    }

    void Initialize(HeapInfo * heapInfo, uint sizeCat);
    void ResetMarks(ResetMarkFlags flags);
    void ScanInitialImplicitRoots(Recycler * recycler);
    void ScanNewImplicitRoots(Recycler * recycler);

    void Sweep(RecyclerSweep& recyclerSweep);
    uint Rescan(Recycler * recycler, RescanFlags flags);
#if ENABLE_CONCURRENT_GC
    void SweepPendingObjects(RecyclerSweep& recyclerSweep);
#endif
#if ENABLE_PARTIAL_GC
    void SweepPartialReusePages(RecyclerSweep& recyclerSweep);
    void FinishPartialCollect(RecyclerSweep * recyclerSweep);
#endif
#if ENABLE_CONCURRENT_GC
    void PrepareSweep();
    void SetupBackgroundSweep(RecyclerSweep& recyclerSweep);
    void TransferPendingEmptyHeapBlocks(RecyclerSweep& recyclerSweep);
#endif
    void SweepFinalizableObjects(RecyclerSweep& recyclerSweep);
    void DisposeObjects();
    void TransferDisposedObjects();
    void EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size));
    void FinalizeAllObjects();
    static unsigned int GetHeapBucketOffset() { return offsetof(HeapBucketGroup<TBlockAttributes>, heapBucket); }

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    size_t GetNonEmptyHeapBlockCount(bool checkCount) const;
    size_t GetEmptyHeapBlockCount() const;
#endif
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    size_t Check();
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    void Verify();
#endif
#ifdef RECYCLER_VERIFY_MARK
    void VerifyMark();
#endif
#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    void StartAllocationDuringConcurrentSweep();
    void FinishConcurrentSweep();
#endif
#if DBG
    bool AllocatorsAreEmpty();
#endif
private:
    SmallNormalHeapBucketT<TBlockAttributes>       heapBucket;
    SmallLeafHeapBucketT<TBlockAttributes>         leafHeapBucket;
    SmallFinalizableHeapBucketT<TBlockAttributes>  finalizableHeapBucket;
#ifdef RECYCLER_WRITE_BARRIER
    SmallNormalWithBarrierHeapBucketT<TBlockAttributes> smallNormalWithBarrierHeapBucket;
    SmallFinalizableWithBarrierHeapBucketT<TBlockAttributes> smallFinalizableWithBarrierHeapBucket;
#endif
};

extern template class HeapBucketGroup<SmallAllocationBlockAttributes>;
extern template class HeapBucketGroup<MediumAllocationBlockAttributes>;
}
