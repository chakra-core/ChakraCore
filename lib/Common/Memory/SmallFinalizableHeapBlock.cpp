//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

#ifdef RECYCLER_WRITE_BARRIER
template <class TBlockAttributes>
SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes>*
SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes>::New(HeapBucketT<SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes>> * bucket)
{
    CompileAssert(TBlockAttributes::MaxObjectSize <= USHRT_MAX);
    Assert(bucket->sizeCat <= TBlockAttributes::MaxObjectSize);
    Assert((TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / bucket->sizeCat <= USHRT_MAX);

    ushort objectSize = (ushort)bucket->sizeCat;
    ushort objectCount = (ushort)(TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / objectSize;
    return NoMemProtectHeapNewNoThrowPlusPrefixZ(Base::GetAllocPlusSize(objectCount), SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes>, bucket, objectSize, objectCount);
}

template <class TBlockAttributes>
void
SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes>::Delete(SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes>* heapBlock)
{
    Assert(heapBlock->IsAnyFinalizableBlock());
    Assert(heapBlock->IsWithBarrier());

    NoMemProtectHeapDeletePlusPrefix(Base::GetAllocPlusSize(heapBlock->objectCount), heapBlock);
}
#endif

#ifdef RECYCLER_VISITED_HOST
template <class TBlockAttributes>
SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>*
SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>::New(HeapBucketT<SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>> * bucket)
{
    CompileAssert(TBlockAttributes::MaxObjectSize <= USHRT_MAX);
    Assert(bucket->sizeCat <= TBlockAttributes::MaxObjectSize);
    Assert((TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / bucket->sizeCat <= USHRT_MAX);

    ushort objectSize = (ushort)bucket->sizeCat;
    ushort objectCount = (ushort)(TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / objectSize;
    return NoMemProtectHeapNewNoThrowPlusPrefixZ(Base::GetAllocPlusSize(objectCount), SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>, bucket, objectSize, objectCount);
}

template <class TBlockAttributes>
void
SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>::Delete(SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>* heapBlock)
{
    Assert(heapBlock->IsRecyclerVisitedHostBlock());

    NoMemProtectHeapDeletePlusPrefix(Base::GetAllocPlusSize(heapBlock->objectCount), heapBlock);
}
#endif

template <class TBlockAttributes>
SmallFinalizableHeapBlockT<TBlockAttributes> *
SmallFinalizableHeapBlockT<TBlockAttributes>::New(HeapBucketT<SmallFinalizableHeapBlockT<TBlockAttributes>> * bucket)
{
    CompileAssert(TBlockAttributes::MaxObjectSize <= USHRT_MAX);
    Assert(bucket->sizeCat <= TBlockAttributes::MaxObjectSize);
    Assert((TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / bucket->sizeCat <= USHRT_MAX);

    ushort objectSize = (ushort)bucket->sizeCat;
    ushort objectCount = (ushort)(TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / objectSize;
    return NoMemProtectHeapNewNoThrowPlusPrefixZ(Base::GetAllocPlusSize(objectCount), SmallFinalizableHeapBlockT<TBlockAttributes>, bucket, objectSize, objectCount);
}

template <class TBlockAttributes>
void
SmallFinalizableHeapBlockT<TBlockAttributes>::Delete(SmallFinalizableHeapBlockT<TBlockAttributes> * heapBlock)
{
    Assert(heapBlock->IsFinalizableBlock());
    NoMemProtectHeapDeletePlusPrefix(Base::GetAllocPlusSize(heapBlock->objectCount), heapBlock);
}

template <>
SmallFinalizableHeapBlockT<SmallAllocationBlockAttributes>::SmallFinalizableHeapBlockT(HeapBucketT<SmallFinalizableHeapBlockT<SmallAllocationBlockAttributes>> * bucket, ushort objectSize, ushort objectCount)
    : Base(bucket, objectSize, objectCount, HeapBlock::SmallFinalizableBlockType)
{
    // We used AllocZ
    Assert(this->finalizeCount == 0);
    Assert(this->pendingDisposeCount == 0);
    Assert(this->disposedObjectList == nullptr);
    Assert(this->disposedObjectListTail == nullptr);
    Assert(!this->isPendingDispose);
}

template <>
SmallFinalizableHeapBlockT<MediumAllocationBlockAttributes>::SmallFinalizableHeapBlockT(HeapBucketT<SmallFinalizableHeapBlockT<MediumAllocationBlockAttributes>> * bucket, ushort objectSize, ushort objectCount)
    : Base(bucket, objectSize, objectCount, MediumFinalizableBlockType)
{
    // We used AllocZ
    Assert(this->finalizeCount == 0);
    Assert(this->pendingDisposeCount == 0);
    Assert(this->disposedObjectList == nullptr);
    Assert(this->disposedObjectListTail == nullptr);
    Assert(!this->isPendingDispose);
}

#ifdef RECYCLER_VISITED_HOST
template <class TBlockAttributes>
SmallFinalizableHeapBlockT<TBlockAttributes>::SmallFinalizableHeapBlockT(HeapBucketT<SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>> * bucket, ushort objectSize, ushort objectCount, HeapBlockType blockType)
    : SmallNormalHeapBlockT<TBlockAttributes>(bucket, objectSize, objectCount, blockType)
{
    // We used AllocZ
    Assert(this->finalizeCount == 0);
    Assert(this->pendingDisposeCount == 0);
    Assert(this->disposedObjectList == nullptr);
    Assert(this->disposedObjectListTail == nullptr);
    Assert(!this->isPendingDispose);
}
#endif

#ifdef RECYCLER_WRITE_BARRIER
template <class TBlockAttributes>
SmallFinalizableHeapBlockT<TBlockAttributes>::SmallFinalizableHeapBlockT(HeapBucketT<SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes>> * bucket, ushort objectSize, ushort objectCount, HeapBlockType blockType)
    : SmallNormalHeapBlockT<TBlockAttributes>(bucket, objectSize, objectCount, blockType)
{
    // We used AllocZ
    Assert(this->finalizeCount == 0);
    Assert(this->pendingDisposeCount == 0);
    Assert(this->disposedObjectList == nullptr);
    Assert(this->disposedObjectListTail == nullptr);
    Assert(!this->isPendingDispose);
}
#endif

template <class TBlockAttributes>
void
SmallFinalizableHeapBlockT<TBlockAttributes>::SetAttributes(void * address, unsigned char attributes)
{
    Assert((attributes & FinalizeBit) != 0);
    __super::SetAttributes(address, attributes);
    finalizeCount++;

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        AssertMsg(!this->isPendingConcurrentSweepPrep, "Finalizable blocks don't support allocations during concurrent sweep.");
    }
#endif

#ifdef RECYCLER_FINALIZE_CHECK
    HeapInfo * heapInfo = this->heapBucket->heapInfo;
    heapInfo->liveFinalizableObjectCount++;
    heapInfo->newFinalizableObjectCount++;
#endif
}

#ifdef RECYCLER_VISITED_HOST
template <class TBlockAttributes>
void
SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>::SetAttributes(void * address, unsigned char attributes)
{
    // Don't call __super, since that has behavior we don't want (it asserts that FinalizeBit is set
    // but recycler visited block allows traced only objects; it also unconditionally bumps the heap info
    // live/new finalizable object counts which will become unbalance if FinalizeBit is not set).
    // We do want the grandparent class behavior though, which actually sets the ObjectInfo bits.
    SmallNormalHeapBlockT<TBlockAttributes>::SetAttributes(address, attributes);

    if (attributes & FinalizeBit)
    {
        this->finalizeCount++;
#ifdef RECYCLER_FINALIZE_CHECK
        HeapInfo * heapInfo = this->heapBucket->heapInfo;
        heapInfo->liveFinalizableObjectCount++;
        heapInfo->newFinalizableObjectCount++;
#endif
    }
}

template <class TBlockAttributes>
template <bool doSpecialMark>
_NOINLINE
void SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>::ProcessMarkedObject(void* objectAddress, MarkContext * markContext)
{
    unsigned char * attributes = nullptr;
    if (!this->TryGetAddressOfAttributes(objectAddress, &attributes))
    {
        return;
    }

    if (!this->template UpdateAttributesOfMarkedObjects<doSpecialMark>(markContext, objectAddress, this->objectSize, *attributes,
        [&](unsigned char _attributes) { *attributes = _attributes; }))
    {
        // Couldn't mark children- bail out and come back later
        this->SetNeedOOMRescan(markContext->GetRecycler());
    }
}

template <class TBlockAttributes>
template <bool doSpecialMark, typename Fn>
bool SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>::UpdateAttributesOfMarkedObjects(MarkContext * markContext, void * objectAddress, size_t objectSize, unsigned char attributes, Fn fn)
{
    bool noOOMDuringMark = true;

    if (attributes & TrackBit)
    {
        Assert((attributes & LeafBit) == 0);
        IRecyclerVisitedObject* recyclerVisited = static_cast<IRecyclerVisitedObject*>(objectAddress);
        noOOMDuringMark = markContext->AddPreciselyTracedObject(recyclerVisited);

        if (noOOMDuringMark)
        {
            // Object has been successfully processed, so clear NewTrackBit
            attributes &= ~NewTrackBit;
        }
        else
        {
            // Set the NewTrackBit, so that the main thread will redo tracking
            attributes |= NewTrackBit;
            noOOMDuringMark = false;
        }
        fn(attributes);
    }

#ifdef RECYCLER_STATS
    RECYCLER_STATS_INTERLOCKED_INC(markContext->GetRecycler(), markData.markCount);
    RECYCLER_STATS_INTERLOCKED_ADD(markContext->GetRecycler(), markData.markBytes, objectSize);

    // Count track or finalize if we don't have to process it in thread because of OOM.
    if ((attributes & (TrackBit | NewTrackBit)) != (TrackBit | NewTrackBit))
    {
        // Only count those we have queued, so we don't double count
        if (attributes & TrackBit)
        {
            RECYCLER_STATS_INTERLOCKED_INC(markContext->GetRecycler(), trackCount);
        }
        if (attributes & FinalizeBit)
        {
            // we counted the finalizable object here,
            // turn off the new bit so we don't count it again
            // on Rescan
            attributes &= ~NewFinalizeBit;
            fn(attributes);
            RECYCLER_STATS_INTERLOCKED_INC(markContext->GetRecycler(), finalizeCount);
        }
    }
#endif

    return noOOMDuringMark;
}

// static
template <class TBlockAttributes>
bool
SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes>::RescanObject(SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes> * block, __in_ecount(localObjectSize) char * objectAddress, uint localObjectSize,
    uint objectIndex, Recycler * recycler)
{
    unsigned char const attributes = block->ObjectInfo(objectIndex);

    if ((attributes & TrackBit) != 0)
    {
        Assert((attributes & LeafBit) == 0);
        Assert(block->GetAddressIndex(objectAddress) != SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit);

        if (!recycler->AddPreciselyTracedMark(reinterpret_cast<IRecyclerVisitedObject*>(objectAddress)))
        {
            // Failed to add to the mark stack due to OOM.
            return false;
        }

        // We have processed this object as tracked, we can clear the NewTrackBit
        block->ObjectInfo(objectIndex) &= ~NewTrackBit;
        RECYCLER_STATS_INC(recycler, trackCount);

        RECYCLER_STATS_INC(recycler, markData.rescanObjectCount);
        RECYCLER_STATS_ADD(recycler, markData.rescanObjectByteCount, localObjectSize);
    }

#ifdef RECYCLER_STATS
    if (attributes & FinalizeBit)
    {
        // Concurrent thread mark the object before the attribute is set and missed the finalize count
        // For finalized object, we will always write a dummy vtable before returning to the call,
        // so the page will always need to be rescanned, and we can count those here.

        // NewFinalizeBit is cleared if the background thread has already counted the object.
        // So if it is still set here, we need to count it

        RECYCLER_STATS_INC_IF(attributes & NewFinalizeBit, recycler, finalizeCount);
        block->ObjectInfo(objectIndex) &= ~NewFinalizeBit;
    }
#endif

    return true;
}
#endif

template <class TBlockAttributes>
bool
SmallFinalizableHeapBlockT<TBlockAttributes>::TryGetAttributes(void* objectAddress, unsigned char * pAttr)
{
    unsigned char * attributes = nullptr;
    if (this->TryGetAddressOfAttributes(objectAddress, &attributes))
    {
        *pAttr = *attributes;
        return true;
    }
    return false;
}

template <class TBlockAttributes>
bool
SmallFinalizableHeapBlockT<TBlockAttributes>::TryGetAddressOfAttributes(void* objectAddress, unsigned char ** ppAttrs)
{
    ushort objectIndex = this->GetAddressIndex(objectAddress);

    if (objectIndex == SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit)
    {
        // Not a valid offset within the block.  No further processing necessary.
        return false;
    }

    *ppAttrs = &this->ObjectInfo(objectIndex);
    return true;
}

template <class TBlockAttributes>
template <bool doSpecialMark>
_NOINLINE
void
SmallFinalizableHeapBlockT<TBlockAttributes>::ProcessMarkedObject(void* objectAddress, MarkContext * markContext)
{
    unsigned char * attributes = nullptr;
    if (!this->TryGetAddressOfAttributes(objectAddress, &attributes))
    {
        return;
    }

    if (!this->template UpdateAttributesOfMarkedObjects<doSpecialMark>(markContext, objectAddress, this->objectSize, *attributes,
        [&](unsigned char _attributes) { *attributes = _attributes; }))
    {
        // Couldn't mark children- bail out and come back later
        this->SetNeedOOMRescan(markContext->GetRecycler());
    }
}

#if ENABLE_PARTIAL_GC || ENABLE_CONCURRENT_GC
// static
template <class TBlockAttributes>
bool
SmallFinalizableHeapBlockT<TBlockAttributes>::CanRescanFullBlock()
{
    // Finalizable block need to rescan object one at a time.
    return false;
}

// static
template <class TBlockAttributes>
bool
SmallFinalizableHeapBlockT<TBlockAttributes>::RescanObject(SmallFinalizableHeapBlockT<TBlockAttributes> * block, __in_ecount(localObjectSize) char * objectAddress, uint localObjectSize,
    uint objectIndex, Recycler * recycler)
{
    unsigned char const attributes = block->ObjectInfo(objectIndex);
    Assert(block->IsAnyFinalizableBlock());

    if ((attributes & LeafBit) == 0)
    {
        Assert(block->GetAddressIndex(objectAddress) != SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit);

        if (!recycler->AddMark(objectAddress, localObjectSize))
        {
            // Failed to add to the mark stack due to OOM.
            return false;
        }

        RECYCLER_STATS_INC(recycler, markData.rescanObjectCount);
        RECYCLER_STATS_ADD(recycler, markData.rescanObjectByteCount, localObjectSize);
    }

    // Since we mark through unallocated objects, we might have marked an object before it
    // is allocated as a tracked object. The object will not be queue up in the
    // tracked object list, and NewTrackBit will still be on. Queue it up now.

    // NewTrackBit will also be on for tracked object that we weren't able to queue
    // because of OOM.  In those case, the page is forced to be rescan, and we will
    // try to process those again here.
    if ((attributes & (TrackBit | NewTrackBit)) == (TrackBit | NewTrackBit))
    {
        if (!block->RescanTrackedObject((FinalizableObject*) objectAddress, objectIndex, recycler))
        {
            // Failed to add to the mark stack due to OOM.
            return false;
        }
    }
#ifdef RECYCLER_STATS
    else if (attributes & FinalizeBit)
    {
        // Concurrent thread mark the object before the attribute is set and missed the finalize count
        // For finalized object, we will always write a dummy vtable before returning to the call,
        // so the page will always need to be rescanned, and we can count those here.

        // NewFinalizeBit is cleared if the background thread has already counted the object.
        // So if it is still set here, we need to count it

        RECYCLER_STATS_INC_IF(attributes & NewFinalizeBit, recycler, finalizeCount);
        block->ObjectInfo(objectIndex) &= ~NewFinalizeBit;
    }
#endif

    return true;
}

template <class TBlockAttributes>
bool
SmallFinalizableHeapBlockT<TBlockAttributes>::RescanTrackedObject(FinalizableObject * object, uint objectIndex, Recycler * recycler)
{
    RecyclerVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("Marking 0x%08x during rescan\n"), object);
#if ENABLE_CONCURRENT_GC
#if ENABLE_PARTIAL_GC
    if (recycler->inPartialCollectMode)
    {
        Assert(!recycler->DoQueueTrackedObject());
    }
    else
#endif
    {
        Assert(recycler->DoQueueTrackedObject());

        if (!recycler->QueueTrackedObject(object))
        {
            // Failed to add to track stack due to OOM.
            return false;
        }
    }

    RECYCLER_STATS_INC(recycler, trackCount);
    RECYCLER_STATS_INC_IF(this->ObjectInfo(objectIndex) & FinalizeBit, recycler, finalizeCount);

    // We have processed this object as tracked, we can clear the NewTrackBit
    this->ObjectInfo(objectIndex) &= ~NewTrackBit;

    return true;
#else
    // REVIEW: Is this correct? Or should we remove the track bit always?
    return false;
#endif
}
#endif

template <class TBlockAttributes>
SweepState
SmallFinalizableHeapBlockT<TBlockAttributes>::Sweep(RecyclerSweep& recyclerSweep, bool queuePendingSweep, bool allocable)
{
    Assert(!recyclerSweep.IsBackground());
    Assert(!queuePendingSweep);

    // If there are finalizable objects in this heap block, they need to be swept
    // in-thread and not in the concurrent thread, so don't queue pending sweep

    return SmallNormalHeapBlockT<TBlockAttributes>::Sweep(recyclerSweep, false, allocable, this->finalizeCount, HasAnyDisposeObjects());
}

template <class TBlockAttributes>
void
SmallFinalizableHeapBlockT<TBlockAttributes>::DisposeObjects()
{
    Assert(this->isPendingDispose);
    Assert(HasAnyDisposeObjects());

    // PARTIALGC-CONSIDER: page with finalizable/disposable object will always be modified
    // because calling dispose probably will modify object itself, and it may call other
    // script that might touch the page as well.  We can't distinguish between these two kind
    // of write to the page.
    //
    // Possible mitigation include:
    //      - allocating finalizable/disposable object on separate pages
    //      - some of the object only need finalize, but not dispose.  mark them separately
    //
    // For now, we always touch the page by zeroing out disposed object which should be moved as well.

    ForEachPendingDisposeObject([&] (uint index) {
        void * objectAddress = this->address + (this->objectSize * index);

        // Dispose the object.
        // Note that Dispose can cause reentrancy, which can cause allocation, which can cause collection.
        // The object we're disposing is still considered PendingDispose until the Dispose call completes.
        // So in case we call CheckFreeBitVector or similar, we should still see correct state re this object.

        ((FinalizableObject *)objectAddress)->Dispose(false);

        Assert(finalizeCount != 0);
        finalizeCount--;
        Assert(pendingDisposeCount != 0);
        pendingDisposeCount--;

        // Properly enqueue the processed object
        // This will also clear the ObjectInfo bits so it's not marked as PendingDispose anymore
        this->EnqueueProcessedObject(&disposedObjectList, &disposedObjectListTail, objectAddress, index);

        RECYCLER_STATS_INC(this->heapBucket->heapInfo->recycler, finalizeSweepCount);
#ifdef RECYCLER_FINALIZE_CHECK
        this->heapBucket->heapInfo->liveFinalizableObjectCount--;
        this->heapBucket->heapInfo->pendingDisposableObjectCount--;
#endif
    });

    // Dispose could have re-entered and caused new pending dispose objects on this block.
    // If so, recycler->hasDisposableObject will have been set again, and we will do another
    // round of Dispose to actually dispose these objects.
    Assert(this->pendingDisposeCount == 0 || this->heapBucket->heapInfo->recycler->hasDisposableObject);
}

template <class TBlockAttributes>
void
SmallFinalizableHeapBlockT<TBlockAttributes>::TransferDisposedObjects()
{
    // CONCURRENT-TODO: we don't allocate on pending disposed blocks during concurrent sweep or disable dispose
    // So the free bit vector must be valid
    Assert(this->IsFreeBitsValid());
    Assert(this->isPendingDispose);
    Assert(this->pendingDisposeCount == 0);

    DebugOnly(this->isPendingDispose = false);

    this->TransferProcessedObjects(this->disposedObjectList, this->disposedObjectListTail);
    this->disposedObjectList = nullptr;
    this->disposedObjectListTail = nullptr;

    // We already updated the bit vector on TransferSweptObjects
    // So just update the free object head.
    this->lastFreeObjectHead = this->freeObjectList;

    RECYCLER_SLOW_CHECK(this->CheckFreeBitVector(true));
}

template <class TBlockAttributes>
ushort
SmallFinalizableHeapBlockT<TBlockAttributes>::AddDisposedObjectFreeBitVector(SmallHeapBlockBitVector * free)
{
    // all the finalized object are considered freed, but not allocable yet
    ushort freeCount = 0;
    FreeObject * freeObject = this->disposedObjectList;

    if (freeObject != nullptr)
    {
        while (true)
        {
            uint bitIndex = this->GetAddressBitIndex(freeObject);
            Assert(this->IsValidBitIndex(bitIndex));

            // not allocable yet
            Assert(!this->GetDebugFreeBitVector()->Test(bitIndex));

            // but in the free list to mark can skip scanning the object
            free->Set(bitIndex);
            freeCount++;

            if (freeObject == this->disposedObjectListTail)
            {
                break;
            }
            freeObject = freeObject->GetNext();
        }
    }
    return freeCount;
}

template <class TBlockAttributes>
void
SmallFinalizableHeapBlockT<TBlockAttributes>::FinalizeAllObjects()
{
    if (this->finalizeCount != 0)
    {
        DebugOnly(uint processedCount = 0);
        this->ForEachAllocatedObject(FinalizeBit, [&](uint index, void * objectAddress)
        {
            FinalizableObject * finalizableObject = ((FinalizableObject *)objectAddress);

            finalizableObject->Finalize(true);
            finalizableObject->Dispose(true);
#ifdef RECYCLER_FINALIZE_CHECK
            this->heapBucket->heapInfo->liveFinalizableObjectCount --;
#endif
            DebugOnly(processedCount++);
        });

        this->ForEachPendingDisposeObject([&] (uint index) {
            void * objectAddress = this->address + (this->objectSize * index);
            ((FinalizableObject *)objectAddress)->Dispose(true);
#ifdef RECYCLER_FINALIZE_CHECK
            this->heapBucket->heapInfo->liveFinalizableObjectCount--;
            this->heapBucket->heapInfo->pendingDisposableObjectCount--;
#endif
            DebugOnly(processedCount++);
        });

        Assert(this->finalizeCount == processedCount);
    }
}

#if DBG
template <class TBlockAttributes>
void
SmallFinalizableHeapBlockT<TBlockAttributes>::Init(ushort objectSize, ushort objectCount)
{
    Assert(this->disposedObjectList == nullptr);
    Assert(this->disposedObjectListTail == nullptr);
    Assert(this->finalizeCount == 0);
    Assert(this->pendingDisposeCount == 0);
    __super::Init(objectSize, objectCount);
}

#if ENABLE_PARTIAL_GC
template <class TBlockAttributes>
void
SmallFinalizableHeapBlockT<TBlockAttributes>::FinishPartialCollect()
{
    Assert(this->disposedObjectList == nullptr);
    Assert(this->disposedObjectListTail == nullptr);
    __super::FinishPartialCollect();
}
#endif
#endif

#ifdef RECYCLER_SLOW_CHECK_ENABLED
template <class TBlockAttributes>
uint
SmallFinalizableHeapBlockT<TBlockAttributes>::CheckDisposedObjectFreeBitVector()
{
    uint verifyFreeCount = 0;
    // all the finalized object are considered freed, but not allocable yet
    FreeObject *freeObject = this->disposedObjectList;
    if (freeObject != nullptr)
    {
        SmallHeapBlockBitVector * free = this->GetFreeBitVector();
        while (true)
        {
            uint bitIndex = this->GetAddressBitIndex(freeObject);
            Assert(this->IsValidBitIndex(bitIndex));
            Assert(!this->GetDebugFreeBitVector()->Test(bitIndex));
            Assert(free->Test(bitIndex));
            verifyFreeCount++;

            if (freeObject == this->disposedObjectListTail)
            {
                break;
            }
            freeObject = freeObject->GetNext();
        }
    }
    return verifyFreeCount;
}

template <class TBlockAttributes>
bool
SmallFinalizableHeapBlockT<TBlockAttributes>::GetFreeObjectListOnAllocator(FreeObject ** freeObjectList)
{
    return this->template GetFreeObjectListOnAllocatorImpl<SmallFinalizableHeapBlockT<TBlockAttributes>>(freeObjectList);
}

#endif

namespace Memory
{
    template class SmallFinalizableHeapBlockT<SmallAllocationBlockAttributes>;
    template void SmallFinalizableHeapBlockT<SmallAllocationBlockAttributes>::ProcessMarkedObject<true>(void* objectAddress, MarkContext * markContext);
    template void SmallFinalizableHeapBlockT<SmallAllocationBlockAttributes>::ProcessMarkedObject<false>(void* objectAddress, MarkContext * markContext);
    template class SmallFinalizableHeapBlockT<MediumAllocationBlockAttributes>;
    template void SmallFinalizableHeapBlockT<MediumAllocationBlockAttributes>::ProcessMarkedObject<true>(void* objectAddress, MarkContext * markContext);;
    template void SmallFinalizableHeapBlockT<MediumAllocationBlockAttributes>::ProcessMarkedObject<false>(void* objectAddress, MarkContext * markContext);;
#ifdef RECYCLER_VISITED_HOST
    template class SmallRecyclerVisitedHostHeapBlockT<SmallAllocationBlockAttributes>;
    template void SmallRecyclerVisitedHostHeapBlockT<SmallAllocationBlockAttributes>::ProcessMarkedObject<true>(void* objectAddress, MarkContext * markContext);
    template void SmallRecyclerVisitedHostHeapBlockT<SmallAllocationBlockAttributes>::ProcessMarkedObject<false>(void* objectAddress, MarkContext * markContext);
    template class SmallRecyclerVisitedHostHeapBlockT<MediumAllocationBlockAttributes>;
    template void SmallRecyclerVisitedHostHeapBlockT<MediumAllocationBlockAttributes>::ProcessMarkedObject<true>(void* objectAddress, MarkContext * markContext);;
    template void SmallRecyclerVisitedHostHeapBlockT<MediumAllocationBlockAttributes>::ProcessMarkedObject<false>(void* objectAddress, MarkContext * markContext);;
#endif

#ifdef RECYCLER_WRITE_BARRIER
    template class SmallFinalizableWithBarrierHeapBlockT<SmallAllocationBlockAttributes>;
    template class SmallFinalizableWithBarrierHeapBlockT<MediumAllocationBlockAttributes>;
#endif
}
