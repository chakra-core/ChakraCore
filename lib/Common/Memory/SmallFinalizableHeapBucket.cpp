//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

template <class TBlockType>
SmallFinalizableHeapBucketBaseT<TBlockType>::SmallFinalizableHeapBucketBaseT() :
    pendingDisposeList(nullptr)
{
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    tempPendingDisposeList = nullptr;
#endif
}

template <class TBlockType>
SmallFinalizableHeapBucketBaseT<TBlockType>::~SmallFinalizableHeapBucketBaseT()
{
    Assert(this->AllocatorsAreEmpty());
    this->DeleteHeapBlockList(this->pendingDisposeList);
    Assert(this->tempPendingDisposeList == nullptr);
}

template <class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::FinalizeAllObjects()
{
    // Finalize all objects on shutdown.

    // Clear allocators to update the information on the heapblock
    // Walk through the allocated object and call finalize and dispose on them
    this->ClearAllocators();
    FinalizeHeapBlockList(this->pendingDisposeList);
#if ENABLE_PARTIAL_GC
    FinalizeHeapBlockList(this->partialHeapBlockList);
#endif
    FinalizeHeapBlockList(this->heapBlockList);
    FinalizeHeapBlockList(this->fullBlockList);

#if ENABLE_PARTIAL_GC && ENABLE_CONCURRENT_GC
    FinalizeHeapBlockList(this->partialSweptHeapBlockList);
#endif
}

template <class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::FinalizeHeapBlockList(TBlockType * list)
{
    HeapBlockList::ForEach(list, [](TBlockType * heapBlock)
    {
        heapBlock->FinalizeAllObjects();
    });
}

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
template <class TBlockType>
size_t
SmallFinalizableHeapBucketBaseT<TBlockType>::GetNonEmptyHeapBlockCount(bool checkCount) const
{
    size_t currentHeapBlockCount =  __super::GetNonEmptyHeapBlockCount(false)
        + HeapBlockList::Count(pendingDisposeList)
        + HeapBlockList::Count(tempPendingDisposeList);
    RECYCLER_SLOW_CHECK(Assert(!checkCount || this->heapBlockCount == currentHeapBlockCount));
    return currentHeapBlockCount;
}
#endif

template <class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::ResetMarks(ResetMarkFlags flags)
{
    __super::ResetMarks(flags);

    if ((flags & ResetMarkFlags_ScanImplicitRoot) != 0)
    {
        HeapBlockList::ForEach(this->pendingDisposeList, [flags](TBlockType * heapBlock)
        {
            heapBlock->MarkImplicitRoots();
        });
    }
}

#if ENABLE_MEM_STATS
template <class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::AggregateBucketStats()
{
    __super::AggregateBucketStats();

    HeapBlockList::ForEach(pendingDisposeList, [this](TBlockType* heapBlock) {
        heapBlock->AggregateBlockStats(this->memStats);
    });
}
#endif

template<class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::Sweep(RecyclerSweep& recyclerSweep)
{
    Assert(!recyclerSweep.IsBackground());

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    Assert(this->tempPendingDisposeList == nullptr);
    this->tempPendingDisposeList = pendingDisposeList;
#endif

    TBlockType * currentDisposeList = pendingDisposeList;
    this->pendingDisposeList = nullptr;

    BaseT::SweepBucket(recyclerSweep, [=](RecyclerSweep& recyclerSweep)
    {
#if DBG
        if (TBlockType::HeapBlockAttributes::IsSmallBlock)
        {
            recyclerSweep.SetupVerifyListConsistencyDataForSmallBlock(nullptr, false, true);
        }
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
        else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
        {
            recyclerSweep.SetupVerifyListConsistencyDataForMediumBlock(nullptr, false, true);
        }
#endif
        else
        {
            Assert(false);
        }
#endif

        HeapBucketT<TBlockType>::SweepHeapBlockList(recyclerSweep, currentDisposeList, false);

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        Assert(this->tempPendingDisposeList == currentDisposeList);
        this->tempPendingDisposeList = nullptr;
#endif
        RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(recyclerSweep.IsBackground()));
    });

}

template <class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::DisposeObjects()
{
    HeapBlockList::ForEach(this->pendingDisposeList, [](TBlockType * heapBlock)
    {
        Assert(heapBlock->HasAnyDisposeObjects());
        heapBlock->DisposeObjects();
    });
}

template <class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::TransferDisposedObjects()
{
    Assert(!this->IsAllocationStopped());
    TBlockType * currentPendingDisposeList = this->pendingDisposeList;
    if (currentPendingDisposeList != nullptr)
    {
        this->pendingDisposeList = nullptr;

        HeapBlockList::ForEach(currentPendingDisposeList, [=](TBlockType * heapBlock)
        {
            heapBlock->TransferDisposedObjects();
            Assert(heapBlock->HasFreeObject());
        });

        // For partial collect, dispose will modify the object, and we
        // also touch the page by chaining the object through the free list
        // might as well reuse the block for partial collect
        this->AppendAllocableHeapBlockList(currentPendingDisposeList);
    }

    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(false));
}

template <class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::EnumerateObjects(ObjectInfoBits infoBits, void(*CallBackFunction)(void * address, size_t size))
{
    __super::EnumerateObjects(infoBits, CallBackFunction);
    HeapBucket::EnumerateObjects(this->pendingDisposeList, infoBits, CallBackFunction);
}

#ifdef RECYCLER_SLOW_CHECK_ENABLED
template <class TBlockType>
size_t
SmallFinalizableHeapBucketBaseT<TBlockType>::Check()
{
    size_t smallHeapBlockCount = __super::Check(false) + HeapInfo::Check(false, true, this->pendingDisposeList);
    Assert(this->heapBlockCount == smallHeapBlockCount);
    return smallHeapBlockCount;
}
#endif

#ifdef RECYCLER_MEMORY_VERIFY
template <class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::Verify()
{
    BaseT::Verify();

#if DBG
    RecyclerVerifyListConsistencyData recyclerVerifyListConsistencyData;
    if (TBlockType::HeapBlockAttributes::IsSmallBlock)
    {
        recyclerVerifyListConsistencyData.SetupVerifyListConsistencyDataForSmallBlock(nullptr, false, true);
    }
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    else if (TBlockType::HeapBlockAttributes::IsMediumBlock)
    {
        recyclerVerifyListConsistencyData.SetupVerifyListConsistencyDataForMediumBlock(nullptr, false, true);
    }
#endif
    else
    {
        Assert(false);
    }

    HeapBlockList::ForEach(this->pendingDisposeList, [this, &recyclerVerifyListConsistencyData](TBlockType * heapBlock)
    {
        DebugOnly(this->VerifyBlockConsistencyInList(heapBlock, recyclerVerifyListConsistencyData));
        heapBlock->Verify(true);
    });
#endif

}
#endif

#ifdef RECYCLER_VERIFY_MARK
template <class TBlockType>
void
SmallFinalizableHeapBucketBaseT<TBlockType>::VerifyMark()
{
    __super::VerifyMark();
    HeapBlockList::ForEach(this->pendingDisposeList, [](TBlockType * heapBlock)
    {
        Assert(heapBlock->HasAnyDisposeObjects());
        heapBlock->VerifyMark();
    });
}
#endif

namespace Memory
{
    template class SmallFinalizableHeapBucketBaseT<SmallFinalizableHeapBlock>;
#ifdef RECYCLER_VISITED_HOST
    template class SmallFinalizableHeapBucketBaseT<SmallRecyclerVisitedHostHeapBlock>;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    template class SmallFinalizableHeapBucketBaseT<MediumRecyclerVisitedHostHeapBlock>;
#endif
#endif
#ifdef RECYCLER_WRITE_BARRIER
    template class SmallFinalizableHeapBucketBaseT<SmallFinalizableWithBarrierHeapBlock>;
#endif

#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    template class SmallFinalizableHeapBucketBaseT<MediumFinalizableHeapBlock>;
#ifdef RECYCLER_WRITE_BARRIER
    template class SmallFinalizableHeapBucketBaseT<MediumFinalizableWithBarrierHeapBlock>;
#endif
#endif
}
