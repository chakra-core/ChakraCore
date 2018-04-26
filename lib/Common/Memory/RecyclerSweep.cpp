//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

RecyclerSweepManager * 
RecyclerSweep::GetManager() const
{
    return this->recyclerSweepManager;
}

bool
RecyclerSweep::IsBackground() const
{
    return GetManager()->IsBackground();
}

Recycler *
RecyclerSweep::GetRecycler() const
{
    return recycler;
}

void
RecyclerSweep::BeginSweep(Recycler * recycler, RecyclerSweepManager * recyclerSweepManager, HeapInfo * heapInfo)
{
    // RecyclerSweep may not be initialized till later in this function but
    // GCETW relies on the recycler pointer being correctly set up
    this->recycler = recycler;
    this->recyclerSweepManager = recyclerSweepManager;
    this->heapInfo = heapInfo;


    // We might still have block that has disposed but not put back into the allocable
    // heap block list yet, which happens if we finish disposing object during concurrent
    // reset mark and can't
    // modify the heap block lists

    // CONCURRENT-TODO: Consider doing it during FinishDisposeObjects to get these block
    // available sooner as well. We will still need it here as we only always get to
    // finish dispose before sweep.
    this->FlushPendingTransferDisposedObjects();

#if ENABLE_CONCURRENT_GC
    // Take the small heap block new heap block list and store in RecyclerSweep temporary
    // We get merge later before we start sweeping the bucket.

    leafData.pendingMergeNewHeapBlockList = heapInfo->newLeafHeapBlockList;
    normalData.pendingMergeNewHeapBlockList = heapInfo->newNormalHeapBlockList;
#ifdef RECYCLER_WRITE_BARRIER
    withBarrierData.pendingMergeNewHeapBlockList = heapInfo->newNormalWithBarrierHeapBlockList;
    finalizableWithBarrierData.pendingMergeNewHeapBlockList = heapInfo->newFinalizableWithBarrierHeapBlockList;
#endif
    finalizableData.pendingMergeNewHeapBlockList = heapInfo->newFinalizableHeapBlockList;
#ifdef RECYCLER_VISITED_HOST
    recyclerVisitedHostData.pendingMergeNewHeapBlockList = heapInfo->newRecyclerVisitedHostHeapBlockList;
#endif

    mediumLeafData.pendingMergeNewHeapBlockList = heapInfo->newMediumLeafHeapBlockList;
    mediumNormalData.pendingMergeNewHeapBlockList = heapInfo->newMediumNormalHeapBlockList;
#ifdef RECYCLER_WRITE_BARRIER
    mediumWithBarrierData.pendingMergeNewHeapBlockList = heapInfo->newMediumNormalWithBarrierHeapBlockList;
    mediumFinalizableWithBarrierData.pendingMergeNewHeapBlockList = heapInfo->newMediumFinalizableWithBarrierHeapBlockList;
#endif
    mediumFinalizableData.pendingMergeNewHeapBlockList = heapInfo->newMediumFinalizableHeapBlockList;
#ifdef RECYCLER_VISITED_HOST
    mediumRecyclerVisitedHostData.pendingMergeNewHeapBlockList = heapInfo->newMediumRecyclerVisitedHostHeapBlockList;
#endif


    heapInfo->newLeafHeapBlockList = nullptr;
    heapInfo->newNormalHeapBlockList = nullptr;
    heapInfo->newFinalizableHeapBlockList = nullptr;
#ifdef RECYCLER_VISITED_HOST
    heapInfo->newRecyclerVisitedHostHeapBlockList = nullptr;
#endif
#ifdef RECYCLER_WRITE_BARRIER
    heapInfo->newNormalWithBarrierHeapBlockList = nullptr;
    heapInfo->newFinalizableWithBarrierHeapBlockList = nullptr;
#endif

    heapInfo->newMediumLeafHeapBlockList = nullptr;
    heapInfo->newMediumNormalHeapBlockList = nullptr;
    heapInfo->newMediumFinalizableHeapBlockList = nullptr;
#ifdef RECYCLER_VISITED_HOST
    heapInfo->newMediumRecyclerVisitedHostHeapBlockList = nullptr;
#endif
#ifdef RECYCLER_WRITE_BARRIER
    heapInfo->newMediumNormalWithBarrierHeapBlockList = nullptr;
    heapInfo->newMediumFinalizableWithBarrierHeapBlockList = nullptr;
#endif

#endif
}

void
RecyclerSweep::ShutdownCleanup()
{
    // REVIEW: Does this need to be controlled more granularly, say with ENABLE_PARTIAL_GC?
#if ENABLE_CONCURRENT_GC
    SmallLeafHeapBucketT<SmallAllocationBlockAttributes>::DeleteHeapBlockList(this->leafData.pendingMergeNewHeapBlockList, recycler);
    SmallNormalHeapBucket::DeleteHeapBlockList(this->normalData.pendingMergeNewHeapBlockList, recycler);
#ifdef RECYCLER_WRITE_BARRIER
    SmallNormalWithBarrierHeapBucket::DeleteHeapBlockList(this->withBarrierData.pendingMergeNewHeapBlockList, recycler);
    SmallFinalizableWithBarrierHeapBucket::DeleteHeapBlockList(this->finalizableWithBarrierData.pendingMergeNewHeapBlockList, recycler);
#endif
    SmallFinalizableHeapBucket::DeleteHeapBlockList(this->finalizableData.pendingMergeNewHeapBlockList, recycler);
    for (uint i = 0; i < HeapConstants::BucketCount; i++)
    {
        // For leaf, we can always reuse the page as we don't need to rescan them for partial GC
        // It should have been swept immediately during Sweep
        Assert(this->leafData.bucketData[i].pendingSweepList == nullptr);
        SmallNormalHeapBucket::DeleteHeapBlockList(this->normalData.bucketData[i].pendingSweepList, recycler);
        SmallFinalizableHeapBucket::DeleteHeapBlockList(this->finalizableData.bucketData[i].pendingSweepList, recycler);
#ifdef RECYCLER_WRITE_BARRIER
        SmallFinalizableWithBarrierHeapBucket::DeleteHeapBlockList(this->finalizableWithBarrierData.bucketData[i].pendingSweepList, recycler);
#endif

        SmallLeafHeapBucket::DeleteEmptyHeapBlockList(this->leafData.bucketData[i].pendingEmptyBlockList);
        SmallNormalHeapBucket::DeleteEmptyHeapBlockList(this->normalData.bucketData[i].pendingEmptyBlockList);
#ifdef RECYCLER_WRITE_BARRIER
        SmallNormalWithBarrierHeapBucket::DeleteEmptyHeapBlockList(this->withBarrierData.bucketData[i].pendingEmptyBlockList);
        Assert(this->finalizableWithBarrierData.bucketData[i].pendingEmptyBlockList == nullptr);
#endif
        Assert(this->finalizableData.bucketData[i].pendingEmptyBlockList == nullptr);
    }

    MediumLeafHeapBucket::DeleteHeapBlockList(this->mediumLeafData.pendingMergeNewHeapBlockList, recycler);
    MediumNormalHeapBucket::DeleteHeapBlockList(this->mediumNormalData.pendingMergeNewHeapBlockList, recycler);
#ifdef RECYCLER_WRITE_BARRIER
    MediumNormalWithBarrierHeapBucket::DeleteHeapBlockList(this->mediumWithBarrierData.pendingMergeNewHeapBlockList, recycler);
    MediumFinalizableWithBarrierHeapBucket::DeleteHeapBlockList(this->mediumFinalizableWithBarrierData.pendingMergeNewHeapBlockList, recycler);
#endif
    MediumFinalizableHeapBucket::DeleteHeapBlockList(this->mediumFinalizableData.pendingMergeNewHeapBlockList, recycler);
    for (uint i = 0; i < HeapConstants::MediumBucketCount; i++)
    {
        // For leaf, we can always reuse the page as we don't need to rescan them for partial GC
        // It should have been swept immediately during Sweep
        Assert(this->mediumLeafData.bucketData[i].pendingSweepList == nullptr);
        MediumNormalHeapBucket::DeleteHeapBlockList(this->mediumNormalData.bucketData[i].pendingSweepList, recycler);
        MediumFinalizableHeapBucket::DeleteHeapBlockList(this->mediumFinalizableData.bucketData[i].pendingSweepList, recycler);
#ifdef RECYCLER_WRITE_BARRIER
        MediumFinalizableWithBarrierHeapBucket::DeleteHeapBlockList(this->mediumFinalizableWithBarrierData.bucketData[i].pendingSweepList, recycler);
#endif

        MediumLeafHeapBucket::DeleteEmptyHeapBlockList(this->mediumLeafData.bucketData[i].pendingEmptyBlockList);
        MediumNormalHeapBucket::DeleteEmptyHeapBlockList(this->mediumNormalData.bucketData[i].pendingEmptyBlockList);
#ifdef RECYCLER_WRITE_BARRIER
        MediumNormalWithBarrierHeapBucket::DeleteEmptyHeapBlockList(this->mediumWithBarrierData.bucketData[i].pendingEmptyBlockList);
        Assert(this->mediumFinalizableWithBarrierData.bucketData[i].pendingEmptyBlockList == nullptr);
#endif
        Assert(this->mediumFinalizableData.bucketData[i].pendingEmptyBlockList == nullptr);
    }
#endif
}

void
RecyclerSweep::FlushPendingTransferDisposedObjects()
{
    if (this->heapInfo->hasPendingTransferDisposedObjects)
    {
        // If recycler->inResolveExternalWeakReferences is true, the recycler isn't really disposing anymore
        // so it's safe to call transferDisposedObjects
        Assert(!recycler->inDispose || recycler->inResolveExternalWeakReferences);
        Assert(!recycler->hasDisposableObject);
        heapInfo->TransferDisposedObjects();
    }
}

#if ENABLE_CONCURRENT_GC
template <typename TBlockType>
void
RecyclerSweep::MergePendingNewHeapBlockList()
{
    TBlockType *& blockList = this->GetData<TBlockType>().pendingMergeNewHeapBlockList;
    TBlockType * list = blockList;
    blockList = nullptr;
    HeapBlockList::ForEachEditing(list, [this](TBlockType * heapBlock)
    {
        auto& bucket = this->heapInfo->GetBucket<TBlockType::RequiredAttributes>(heapBlock->GetObjectSize());
        bucket.MergeNewHeapBlock(heapBlock);
    });
}
template void RecyclerSweep::MergePendingNewHeapBlockList<SmallLeafHeapBlock>();
template void RecyclerSweep::MergePendingNewHeapBlockList<SmallNormalHeapBlock>();
template void RecyclerSweep::MergePendingNewHeapBlockList<SmallFinalizableHeapBlock>();
#ifdef RECYCLER_VISITED_HOST
template void RecyclerSweep::MergePendingNewHeapBlockList<SmallRecyclerVisitedHostHeapBlock>();
#endif
#ifdef RECYCLER_WRITE_BARRIER
template void RecyclerSweep::MergePendingNewHeapBlockList<SmallNormalWithBarrierHeapBlock>();
template void RecyclerSweep::MergePendingNewHeapBlockList<SmallFinalizableWithBarrierHeapBlock>();
#endif

template <typename TBlockType>
void
RecyclerSweep::MergePendingNewMediumHeapBlockList()
{
    TBlockType *& blockList = this->GetData<TBlockType>().pendingMergeNewHeapBlockList;
    TBlockType * list = blockList;
    blockList = nullptr;
    HeapBlockList::ForEachEditing(list, [this](TBlockType * heapBlock)
    {
        auto& bucket = this->heapInfo->GetMediumBucket<TBlockType::RequiredAttributes>(heapBlock->GetObjectSize());
        bucket.MergeNewHeapBlock(heapBlock);
    });
}

template void RecyclerSweep::MergePendingNewMediumHeapBlockList<MediumLeafHeapBlock>();
template void RecyclerSweep::MergePendingNewMediumHeapBlockList<MediumNormalHeapBlock>();
template void RecyclerSweep::MergePendingNewMediumHeapBlockList<MediumFinalizableHeapBlock>();
#ifdef RECYCLER_VISITED_HOST
template void RecyclerSweep::MergePendingNewMediumHeapBlockList<MediumRecyclerVisitedHostHeapBlock>();
#endif
#ifdef RECYCLER_WRITE_BARRIER
template void RecyclerSweep::MergePendingNewMediumHeapBlockList<MediumNormalWithBarrierHeapBlock>();
template void RecyclerSweep::MergePendingNewMediumHeapBlockList<MediumFinalizableWithBarrierHeapBlock>();
#endif

bool
RecyclerSweep::HasPendingEmptyBlocks() const
{
    return this->hasPendingEmptyBlocks;
}

bool
RecyclerSweep::HasPendingSweepSmallHeapBlocks() const
{
    return this->hasPendingSweepSmallHeapBlocks;
}

void
RecyclerSweep::SetHasPendingSweepSmallHeapBlocks()
{
    this->hasPendingSweepSmallHeapBlocks = true;
}

#if DBG
bool
RecyclerSweep::HasPendingNewHeapBlocks() const
{
    return leafData.pendingMergeNewHeapBlockList != nullptr
        || normalData.pendingMergeNewHeapBlockList != nullptr
        || finalizableData.pendingMergeNewHeapBlockList != nullptr
#ifdef RECYCLER_WRITE_BARRIER
        || withBarrierData.pendingMergeNewHeapBlockList != nullptr
        || finalizableWithBarrierData.pendingMergeNewHeapBlockList != nullptr
#endif
        || mediumLeafData.pendingMergeNewHeapBlockList != nullptr
        || mediumNormalData.pendingMergeNewHeapBlockList != nullptr
        || mediumFinalizableData.pendingMergeNewHeapBlockList != nullptr
#ifdef RECYCLER_WRITE_BARRIER
        || mediumWithBarrierData.pendingMergeNewHeapBlockList != nullptr
        || mediumFinalizableWithBarrierData.pendingMergeNewHeapBlockList != nullptr
#endif

        ;
}
#endif

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
size_t
RecyclerSweep::GetPendingMergeNewHeapBlockCount(HeapInfo const * heapInfo)
{
    if (this->heapInfo != heapInfo)
    {
        return 0;
    }
    return HeapBlockList::Count(leafData.pendingMergeNewHeapBlockList)
        + HeapBlockList::Count(normalData.pendingMergeNewHeapBlockList)
        + HeapBlockList::Count(finalizableData.pendingMergeNewHeapBlockList)
#ifdef RECYCLER_VISITED_HOST
        + HeapBlockList::Count(recyclerVisitedHostData.pendingMergeNewHeapBlockList)
        + HeapBlockList::Count(mediumRecyclerVisitedHostData.pendingMergeNewHeapBlockList)
#endif
#ifdef RECYCLER_WRITE_BARRIER
        + HeapBlockList::Count(withBarrierData.pendingMergeNewHeapBlockList)
        + HeapBlockList::Count(finalizableWithBarrierData.pendingMergeNewHeapBlockList)
#endif
        + HeapBlockList::Count(mediumLeafData.pendingMergeNewHeapBlockList)
        + HeapBlockList::Count(mediumNormalData.pendingMergeNewHeapBlockList)
        + HeapBlockList::Count(mediumFinalizableData.pendingMergeNewHeapBlockList)
#ifdef RECYCLER_WRITE_BARRIER
        + HeapBlockList::Count(mediumWithBarrierData.pendingMergeNewHeapBlockList)
        + HeapBlockList::Count(mediumFinalizableWithBarrierData.pendingMergeNewHeapBlockList)
#endif
        ;
}
#endif
#endif

#if ENABLE_PARTIAL_GC
bool
RecyclerSweep::InPartialCollectMode() const
{
    return GetRecycler()->inPartialCollectMode;
}

bool
RecyclerSweep::InPartialCollect() const
{
    return GetManager()->InPartialCollect();
}
#endif
