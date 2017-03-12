//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{
template <class TBlockAttributes>
class SmallNormalHeapBlockT : public SmallHeapBlockT<TBlockAttributes>
{
    typedef SmallHeapBlockT<TBlockAttributes> Base;
    using Base::SmallNormalBlockType;
    using Base::MediumNormalBlockType;
    friend class HeapBucketT<SmallNormalHeapBlockT>;
public:
    typedef typename Base::HeapBlockType HeapBlockType;
    typedef typename Base::SmallHeapBlockBitVector SmallHeapBlockBitVector;
    typedef TBlockAttributes HeapBlockAttributes;
    static const ObjectInfoBits RequiredAttributes = NoBit;
    static const bool IsLeafOnly = false;

    static SmallNormalHeapBlockT * New(HeapBucketT<SmallNormalHeapBlockT> * bucket);
    static void Delete(SmallNormalHeapBlockT * block);

    SmallNormalHeapBlockT * GetNextBlock() const
    {
        HeapBlock* block = Base::GetNextBlock();
        return block ? block->template AsNormalBlock<TBlockAttributes>() : nullptr;
    }
    void SetNextBlock(SmallNormalHeapBlockT * next) { Base::SetNextBlock(next); }

    void ScanInitialImplicitRoots(Recycler * recycler);
    void ScanNewImplicitRoots(Recycler * recycler);

    static uint CalculateMarkCountForPage(SmallHeapBlockBitVector* markBits, uint bucketIndex, uint pageStartBitIndex);

    static bool CanRescanFullBlock();
    static bool RescanObject(SmallNormalHeapBlockT<TBlockAttributes> * block, __in_ecount(localObjectSize) char * objectAddress, uint localObjectSize, uint objectIndex, Recycler * recycler);
#if ENABLE_PARTIAL_GC || ENABLE_CONCURRENT_GC
    void FinishPartialCollect();
#endif

#ifdef RECYCLER_SLOW_CHECK_ENABLED
    virtual bool GetFreeObjectListOnAllocator(FreeObject ** freeObjectList) override;
#endif

    virtual bool FindHeapObject(void* objectAddress, Recycler * recycler, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject) override
    {
        return this->template FindHeapObjectImpl<SmallNormalHeapBlockT<TBlockAttributes>>(objectAddress, recycler, flags, heapObject);
    }
protected:
    SmallNormalHeapBlockT(HeapBucket * bucket, ushort objectSize, ushort objectCount, HeapBlockType heapBlockType);

private:
    SmallNormalHeapBlockT(HeapBucketT<SmallNormalHeapBlockT> * bucket, ushort objectSize, ushort objectCount);

};

#ifdef RECYCLER_WRITE_BARRIER
template <class TBlockAttributes>
class SmallNormalWithBarrierHeapBlockT : public SmallNormalHeapBlockT<TBlockAttributes>
{
    typedef SmallNormalHeapBlockT<TBlockAttributes> Base;
    typedef typename Base::HeapBlockType HeapBlockType;
    friend class HeapBucketT<SmallNormalWithBarrierHeapBlockT>;
public:
    typedef TBlockAttributes HeapBlockAttributes;
    static const ObjectInfoBits RequiredAttributes = WithBarrierBit;
    static const bool IsLeafOnly = false;

    static SmallNormalWithBarrierHeapBlockT * New(HeapBucketT<SmallNormalWithBarrierHeapBlockT> * bucket);
    static void Delete(SmallNormalWithBarrierHeapBlockT * heapBlock);

    SmallNormalWithBarrierHeapBlockT * GetNextBlock() const
    {
        HeapBlock* block = SmallHeapBlockT<TBlockAttributes>::GetNextBlock();
        return block ? block->template AsNormalWriteBarrierBlock<TBlockAttributes>() : nullptr;
    }

    virtual bool FindHeapObject(void* objectAddress, Recycler * recycler, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject) override sealed
    {
        return this->template FindHeapObjectImpl<SmallNormalWithBarrierHeapBlockT<TBlockAttributes>>(objectAddress, recycler, flags, heapObject);
    }
protected:
    SmallNormalWithBarrierHeapBlockT(HeapBucket * bucket, ushort objectSize, ushort objectCount, HeapBlockType heapBlockType) :
        SmallNormalHeapBlockT<TBlockAttributes>(bucket, objectSize, objectCount, heapBlockType)
    {}

};
#endif

typedef SmallNormalHeapBlockT<SmallAllocationBlockAttributes> SmallNormalHeapBlock;
typedef SmallNormalHeapBlockT<MediumAllocationBlockAttributes> MediumNormalHeapBlock;

#ifdef RECYCLER_WRITE_BARRIER
typedef SmallNormalWithBarrierHeapBlockT<SmallAllocationBlockAttributes> SmallNormalWithBarrierHeapBlock;
typedef SmallNormalWithBarrierHeapBlockT<MediumAllocationBlockAttributes> MediumNormalWithBarrierHeapBlock;
#endif
}
