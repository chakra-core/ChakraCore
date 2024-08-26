//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::SetAttributes(void * address, unsigned char attributes)
{
    Assert(this->address != nullptr);
    Assert(this->segment != nullptr);
    ushort index = GetAddressIndex(address);
    Assert(this->ObjectInfo(index) == 0);
    Assert(index != SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit);
    ObjectInfo(index) = attributes;
}

inline
IdleDecommitPageAllocator*
HeapBlock::GetPageAllocator(HeapInfo * heapInfo)
{
    switch (this->GetHeapBlockType())
    {
    case SmallLeafBlockType:
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    case MediumLeafBlockType:
#endif
        return heapInfo->GetRecyclerLeafPageAllocator();
    case LargeBlockType:
        return heapInfo->GetRecyclerLargeBlockPageAllocator();
#ifdef RECYCLER_WRITE_BARRIER
    case SmallNormalBlockWithBarrierType:
    case SmallFinalizableBlockWithBarrierType:
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    case MediumNormalBlockWithBarrierType:
    case MediumFinalizableBlockWithBarrierType:
#endif
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_THREAD_PAGE
        return heapInfo->GetRecyclerLeafPageAllocator();
#elif defined(RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE)
        return heapInfo->GetRecyclerWithBarrierPageAllocator();
#endif
#endif

    default:
        return heapInfo->GetRecyclerPageAllocator();
    };
}

template <class TBlockAttributes>
IdleDecommitPageAllocator*
SmallHeapBlockT<TBlockAttributes>::GetPageAllocator()
{
    return __super::GetPageAllocator(this->heapBucket->heapInfo);
}

template <class TBlockAttributes>
template <class Fn>
void SmallHeapBlockT<TBlockAttributes>::ForEachAllocatedObject(Fn fn)
{
    uint const objectBitDelta = this->GetObjectBitDelta();
    SmallHeapBlockBitVector * free = this->EnsureFreeBitVector();
    char * address = this->GetAddress();
    uint objectSize = this->GetObjectSize();
    for (uint i = 0; i < objectCount; i++)
    {
        if (!free->Test(i * objectBitDelta))
        {
            fn(i, address + i * objectSize);
        }
    }
}

template <class TBlockAttributes>
template <typename Fn>
void SmallHeapBlockT<TBlockAttributes>::ForEachAllocatedObject(ObjectInfoBits attributes, Fn fn)
{
    ForEachAllocatedObject([=](uint index, void * objectAddress)
    {
        if ((ObjectInfo(index) & attributes) != 0)
        {
            fn(index, objectAddress);
        }
    });
};

template <class TBlockAttributes>
template <typename Fn>
void SmallHeapBlockT<TBlockAttributes>::ScanNewImplicitRootsBase(Fn fn)
{
    uint const localObjectCount = this->objectCount;

    // NOTE: we no longer track the mark count as we mark.  So this value
    // is basically the mark count we set during the initial implicit root scan
    // plus any subsequent new implicit root scan.
    uint localMarkCount = this->markCount;
    if (localMarkCount == localObjectCount)
    {
        // The block is full when we first do the initial implicit root scan
        // So there can't be any new implicit roots
        return;
    }

#if DBG
    HeapBlockMap& map = this->GetRecycler()->heapBlockMap;
    ushort newlyMarkedCountForPage[TBlockAttributes::PageCount];
    for (uint i = 0; i < TBlockAttributes::PageCount; i++)
    {
        newlyMarkedCountForPage[i] = 0;
    }
#endif

    uint const localObjectBitDelta = this->GetObjectBitDelta();
    uint const localObjectSize = this->GetObjectSize();
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    Assert(localObjectSize <= HeapConstants::MaxSmallObjectSize);
#else
    Assert(localObjectSize <= HeapConstants::MaxMediumObjectSize);
#endif
    SmallHeapBlockBitVector * mark = this->GetMarkedBitVector();
    char * address = this->GetAddress();

    for (uint i = 0; i < localObjectCount; i++)
    {
        if ((this->ObjectInfo(i) & ImplicitRootBit) != 0
            && !mark->TestAndSet(i * localObjectBitDelta))
        {
            uint objectOffset = i * localObjectSize;
            localMarkCount++;
#if DBG
            uint pageNumber = objectOffset / AutoSystemInfo::PageSize;
            Assert(pageNumber < TBlockAttributes::PageCount);
            newlyMarkedCountForPage[pageNumber]++;
#endif

            fn(address + objectOffset, localObjectSize);
        }
    }
    Assert(localMarkCount <= USHRT_MAX);
#if DBG
    // Add newly marked count
    for (uint i = 0; i < TBlockAttributes::PageCount; i++)
    {
        char* pageAddress = address + (AutoSystemInfo::PageSize * i);
        ushort oldPageMarkCount = map.GetPageMarkCount(pageAddress);
        map.SetPageMarkCount(pageAddress, oldPageMarkCount + newlyMarkedCountForPage[i]);
    }

#endif
    this->markCount = (ushort)localMarkCount;
}

template <class TBlockAttributes>
bool
SmallHeapBlockT<TBlockAttributes>::FindImplicitRootObject(void* candidate, Recycler* recycler, RecyclerHeapObjectInfo& heapObject)
{
    ushort index = GetAddressIndex(candidate);

    if (index == InvalidAddressBit)
    {
        return false;
    }

    byte& attributes = ObjectInfo(index);
    heapObject = RecyclerHeapObjectInfo(candidate, recycler, this, &attributes);
    return true;
}

template <bool doSpecialMark, typename Fn>
bool
HeapBlock::UpdateAttributesOfMarkedObjects(MarkContext * markContext, void * objectAddress, size_t objectSize, unsigned char attributes, Fn fn)
{
#ifdef RECYCLER_VISITED_HOST
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    Assert(GetHeapBlockType() != HeapBlock::HeapBlockType::SmallRecyclerVisitedHostBlockType && GetHeapBlockType() != HeapBlock::HeapBlockType::LargeBlockType);
#else
    Assert(GetHeapBlockType() != HeapBlock::HeapBlockType::SmallRecyclerVisitedHostBlockType && GetHeapBlockType() != HeapBlock::HeapBlockType::MediumRecyclerVisitedHostBlockType && GetHeapBlockType() != HeapBlock::HeapBlockType::LargeBlockType);
#endif
#endif

    bool noOOMDuringMark = true;

    if (attributes & TrackBit)
    {
        FinalizableObject * trackedObject = (FinalizableObject *)objectAddress;

#if ENABLE_PARTIAL_GC
        if (!markContext->GetRecycler()->inPartialCollectMode)
#endif
        {
#if ENABLE_CONCURRENT_GC
            if (markContext->GetRecycler()->DoQueueTrackedObject())
            {
                if (!markContext->AddTrackedObject(trackedObject))
                {
                    noOOMDuringMark = false;
                }
            }
            else
#endif
            {
                // Process the tracked object right now
                markContext->MarkTrackedObject(trackedObject);
            }
        }

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

    // only need to scan non-leaf objects
    if ((attributes & LeafBit) == 0)
    {
        if (!markContext->AddMarkedObject(objectAddress, objectSize))
        {
            noOOMDuringMark = false;
        }
    }

    // Special mark-time behavior for finalizable objects on certain GC's
    if (doSpecialMark)
    {
        if (attributes & FinalizeBit)
        {
            FinalizableObject * trackedObject = (FinalizableObject *)objectAddress;
            trackedObject->OnMark();
        }
    }        

#ifdef RECYCLER_STATS
    RECYCLER_STATS_INTERLOCKED_INC(markContext->GetRecycler(), markData.markCount);
    RECYCLER_STATS_INTERLOCKED_ADD(markContext->GetRecycler(), markData.markBytes, objectSize);

    // Don't count track or finalize it if we still have to process it in thread because of OOM
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
