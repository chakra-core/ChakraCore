//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Memory
{
template <class TBlockAttributes> class SmallFinalizableHeapBucketT;
#ifdef RECYCLER_WRITE_BARRIER
template <class TBlockAttributes> class SmallFinalizableWithBarrierHeapBlockT;
#endif

template <class TBlockAttributes>
class SmallFinalizableHeapBlockT : public SmallNormalHeapBlockT<TBlockAttributes>
{
    typedef SmallNormalHeapBlockT<TBlockAttributes> Base;
    typedef typename Base::SmallHeapBlockBitVector SmallHeapBlockBitVector;
    typedef typename Base::HeapBlockType HeapBlockType;
    friend class HeapBucketT<SmallFinalizableHeapBlockT>;
    using Base::MediumFinalizableBlockType;
public:
    typedef TBlockAttributes HeapBlockAttributes;

    static const ObjectInfoBits RequiredAttributes = FinalizeBit;
    static SmallFinalizableHeapBlockT * New(HeapBucketT<SmallFinalizableHeapBlockT> * bucket);
    static void Delete(SmallFinalizableHeapBlockT * block);

    SmallFinalizableHeapBlockT * GetNextBlock() const
    {
        HeapBlock* block = SmallHeapBlockT<TBlockAttributes>::GetNextBlock();
        return block ? block->template AsFinalizableBlock<TBlockAttributes>() : nullptr;
    }
    void SetNextBlock(SmallFinalizableHeapBlockT * next) { Base::SetNextBlock(next); }

    bool TryGetAddressOfAttributes(void* objectAddress, unsigned char **ppAttr);
    bool TryGetAttributes(void* objectAddress, unsigned char *pAttr);

    template <bool doSpecialMark>
    void ProcessMarkedObject(void* candidate, MarkContext * markContext);

    void SetAttributes(void * address, unsigned char attributes);

#if ENABLE_PARTIAL_GC || ENABLE_CONCURRENT_GC
    static bool CanRescanFullBlock();
    static bool RescanObject(SmallFinalizableHeapBlockT<TBlockAttributes> * block, __in_ecount(localObjectSize) char * objectAddress, uint localObjectSize, uint objectIndex, Recycler * recycler);
    bool RescanTrackedObject(FinalizableObject * object, uint objectIndex,  Recycler * recycler);
#endif
    SweepState Sweep(RecyclerSweep& sweepeData, bool queuePendingSweep, bool allocable);
    void DisposeObjects();
    void TransferDisposedObjects();

    bool HasPendingDisposeObjects() const
    {
        return (this->pendingDisposeCount != 0);
    }

    void AddPendingDisposeObject()
    {
        this->pendingDisposeCount++;
        Assert(this->pendingDisposeCount <= this->objectCount);
    }

    bool HasDisposedObjects() const { return this->disposedObjectList != nullptr; }
    bool HasAnyDisposeObjects() const { return this->HasPendingDisposeObjects() || this->HasDisposedObjects(); }

    template <typename Fn>
    void ForEachPendingDisposeObject(Fn fn)
    {
        if (this->HasPendingDisposeObjects())
        {
            for (uint i = 0; i < this->objectCount; i++)
            {
                if ((this->ObjectInfo(i) & PendingDisposeBit) != 0)
                {
                    // When pending dispose, exactly the PendingDisposeBits should be set
                    Assert(this->ObjectInfo(i) == PendingDisposeObjectBits);

                    fn(i);
                }
            }
        }
        else
        {
#if DBG
            for (uint i = 0; i < this->objectCount; i++)
            {
                Assert((this->ObjectInfo(i) & PendingDisposeBit) == 0);
            }
#endif
        }
    }

    ushort AddDisposedObjectFreeBitVector(SmallHeapBlockBitVector * free);
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    uint CheckDisposedObjectFreeBitVector();
    virtual bool GetFreeObjectListOnAllocator(FreeObject ** freeObjectList) override;
#endif
#if DBG
#if ENABLE_PARTIAL_GC
    void FinishPartialCollect();
#endif
    bool IsPendingDispose() const { return isPendingDispose; }
    void SetIsPendingDispose() { isPendingDispose = true; }
#endif
    void FinalizeAllObjects();

#ifdef DUMP_FRAGMENTATION_STATS
    ushort GetFinalizeCount() {
        return finalizeCount;
    }
#endif

    virtual bool FindHeapObject(void* objectAddress, Recycler * recycler, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject) override
    {
        return this->template FindHeapObjectImpl<SmallFinalizableHeapBlockT<TBlockAttributes>>(objectAddress, recycler, flags, heapObject);
    }
protected:
    SmallFinalizableHeapBlockT(HeapBucketT<SmallFinalizableHeapBlockT>  * bucket, ushort objectSize, ushort objectCount);
#ifdef RECYCLER_WRITE_BARRIER
    SmallFinalizableHeapBlockT(HeapBucketT<SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes>> * bucket, ushort objectSize, ushort objectCount, HeapBlockType blockType);
#endif

#if DBG
    void Init(ushort objectSize, ushort objectCount);
    bool isPendingDispose;
#endif
    ushort finalizeCount;
    ushort pendingDisposeCount;
    FreeObject* disposedObjectList;
    FreeObject* disposedObjectListTail;

    friend class ::ScriptMemoryDumper;
#ifdef RECYCLER_MEMORY_VERIFY
    friend void SmallHeapBlockT<TBlockAttributes>::Verify(bool pendingDispose);
#endif
};

#ifdef RECYCLER_WRITE_BARRIER
template <class TBlockAttributes>
class SmallFinalizableWithBarrierHeapBlockT : public SmallFinalizableHeapBlockT<TBlockAttributes>
{
    typedef SmallFinalizableHeapBlockT<TBlockAttributes> Base;
    friend class HeapBucketT<SmallFinalizableWithBarrierHeapBlockT>;
public:
    typedef TBlockAttributes HeapBlockAttributes;

    static const ObjectInfoBits RequiredAttributes = FinalizableWithBarrierBit;
    static const bool IsLeafOnly = false;

    static SmallFinalizableWithBarrierHeapBlockT * New(HeapBucketT<SmallFinalizableWithBarrierHeapBlockT> * bucket);
    static void Delete(SmallFinalizableWithBarrierHeapBlockT * block);

    SmallFinalizableWithBarrierHeapBlockT * GetNextBlock() const
    {
        HeapBlock* block = SmallHeapBlockT<TBlockAttributes>::GetNextBlock();
        return block ? block->template AsFinalizableWriteBarrierBlock<TBlockAttributes>() : nullptr;
    }

    virtual bool FindHeapObject(void* objectAddress, Recycler * recycler, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject) override sealed
    {
        return this->template FindHeapObjectImpl<SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes>>(objectAddress, recycler, flags, heapObject);
    }
protected:
    SmallFinalizableWithBarrierHeapBlockT(HeapBucketT<SmallFinalizableWithBarrierHeapBlockT> * bucket, ushort objectSize, ushort objectCount)
        : SmallFinalizableHeapBlockT<TBlockAttributes>(bucket, objectSize, objectCount, TBlockAttributes::IsSmallBlock ? Base::SmallFinalizableBlockWithBarrierType : Base::MediumFinalizableBlockWithBarrierType)
    {
    }
};
#endif

typedef SmallFinalizableHeapBlockT<SmallAllocationBlockAttributes>  SmallFinalizableHeapBlock;
typedef SmallFinalizableHeapBlockT<MediumAllocationBlockAttributes>    MediumFinalizableHeapBlock;

#ifdef RECYCLER_WRITE_BARRIER
typedef SmallFinalizableWithBarrierHeapBlockT<SmallAllocationBlockAttributes>   SmallFinalizableWithBarrierHeapBlock;
typedef SmallFinalizableWithBarrierHeapBlockT<MediumAllocationBlockAttributes>     MediumFinalizableWithBarrierHeapBlock;
#endif
}
