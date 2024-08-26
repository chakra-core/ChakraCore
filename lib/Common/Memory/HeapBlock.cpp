//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"
#if defined(__clang__) && !defined(_MSC_VER)
#include <cxxabi.h>
#endif

//========================================================================================================
// HeapBlock
//========================================================================================================
template <typename TBlockAttributes>
SmallNormalHeapBlockT<TBlockAttributes> *
HeapBlock::AsNormalBlock()
{
    Assert(IsAnyNormalBlock());
    return static_cast<SmallNormalHeapBlockT<TBlockAttributes> *>(this);
}

template <typename TBlockAttributes>
SmallLeafHeapBlockT<TBlockAttributes> *
HeapBlock::AsLeafBlock()
{
    Assert(IsLeafBlock());
    return static_cast<SmallLeafHeapBlockT<TBlockAttributes> *>(this);
}

template <typename TBlockAttributes>
SmallFinalizableHeapBlockT<TBlockAttributes> *
HeapBlock::AsFinalizableBlock()
{
    Assert(IsAnyFinalizableBlock());
    return static_cast<SmallFinalizableHeapBlockT<TBlockAttributes> *>(this);
}

#ifdef RECYCLER_VISITED_HOST
template <typename TBlockAttributes>
SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes> *
HeapBlock::AsRecyclerVisitedHostBlock()
{
    Assert(IsRecyclerVisitedHostBlock());
    return static_cast<SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes> *>(this);
}
#endif

#ifdef RECYCLER_WRITE_BARRIER
template <typename TBlockAttributes>
SmallNormalWithBarrierHeapBlockT<TBlockAttributes> *
HeapBlock::AsNormalWriteBarrierBlock()
{
    Assert(IsNormalWriteBarrierBlock());
    return static_cast<SmallNormalWithBarrierHeapBlockT<TBlockAttributes> *>(this);
}

template <typename TBlockAttributes>
SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes> *
HeapBlock::AsFinalizableWriteBarrierBlock()
{
    Assert(IsFinalizableWriteBarrierBlock());
    return static_cast<SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes> *>(this);
}
#endif

void
HeapBlock::SetNeedOOMRescan(Recycler * recycler)
{
    Assert(!this->IsLeafBlock());
    this->needOOMRescan = true;
    recycler->SetNeedOOMRescan();
}

//========================================================================================================
// SmallHeapBlock
//========================================================================================================
template <class TBlockAttributes>
size_t
SmallHeapBlockT<TBlockAttributes>::GetAllocPlusSize(uint objectCount)
{
    // Small Heap Block Layout:
    //      TrackerData * [objectCount]  (Optional)
    //      ObjectInfo    [objectCount]  (In reverse index order)
    //      <Small*HeapBlock>

    size_t allocPlusSize = Math::Align<size_t>(sizeof(unsigned char) * objectCount, sizeof(size_t));
#ifdef PROFILE_RECYCLER_ALLOC
    if (Recycler::DoProfileAllocTracker())
    {
        allocPlusSize += objectCount * sizeof(void *);
    }
#endif
    return allocPlusSize;
}


template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::ConstructorCommon(HeapBucket * bucket, ushort objectSize, ushort objectCount, HeapBlockType heapBlockType)
{
    this->heapBucket = bucket;
    this->Init(objectSize, objectCount);
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    Assert(heapBlockType < HeapBlock::HeapBlockType::SmallAllocBlockTypeCount);
    Assert(objectCount == (this->GetPageCount() * AutoSystemInfo::PageSize) / objectSize);
#else
    Assert(heapBlockType < HeapBlock::HeapBlockType::SmallAllocBlockTypeCount + HeapBlock::HeapBlockType::MediumAllocBlockTypeCount);
    Assert(objectCount > 1 && objectCount == (this->GetPageCount() * AutoSystemInfo::PageSize) / objectSize);
#endif

#if defined(RECYCLER_SLOW_CHECK_ENABLED)
    heapBucket->heapInfo->heapBlockCount[heapBlockType]++;
#endif

    if (TBlockAttributes::IsSmallBlock)
    {
        Assert(heapBlockType < HeapBlockType::SmallAllocBlockTypeCount);
    }
    else
    {
        Assert(heapBlockType >= HeapBlockType::SmallAllocBlockTypeCount && heapBlockType < HeapBlockType::SmallBlockTypeCount);
    }

    DebugOnly(lastUncollectedAllocBytes = 0);
}

template <class TBlockAttributes>
SmallHeapBlockT<TBlockAttributes>::SmallHeapBlockT(HeapBucket * bucket, ushort objectSize, ushort objectCount, HeapBlockType heapBlockType)
    : HeapBlock(heapBlockType),
    bucketIndex(bucket->GetBucketIndex()),
    validPointers(HeapInfo::smallAllocValidPointersMap.GetValidPointersForIndex(bucket->GetBucketIndex())),
    objectSize(objectSize), objectCount(objectCount)
{
    ConstructorCommon(bucket, objectSize, objectCount, heapBlockType);
}

#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
template <>
SmallHeapBlockT<MediumAllocationBlockAttributes>::SmallHeapBlockT(HeapBucket * bucket, ushort objectSize, ushort objectCount, HeapBlockType heapBlockType)
    : HeapBlock((HeapBlockType)(heapBlockType)),
    bucketIndex(HeapInfo::GetMediumBucketIndex(objectSize)),
    validPointers(HeapInfo::mediumAllocValidPointersMap.GetValidPointersForIndex(HeapInfo::GetMediumBucketIndex(objectSize))),
    objectSize(objectSize), objectCount(objectCount)
{
    ConstructorCommon(bucket, objectSize, objectCount, heapBlockType);
}
#endif
template <class TBlockAttributes>
SmallHeapBlockT<TBlockAttributes>::~SmallHeapBlockT()
{
    Assert((this->segment == nullptr && this->address == nullptr) ||
        (this->IsLeafBlock()) ||
        this->GetPageAllocator()->IsClosed());

#if defined(RECYCLER_SLOW_CHECK_ENABLED)
    heapBucket->heapInfo->heapBlockCount[this->GetHeapBlockType()]--;
#endif

#if defined(RECYCLER_SLOW_CHECK_ENABLED) || ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    heapBucket->heapBlockCount--;
#endif
}

template <class TBlockAttributes>
uint
SmallHeapBlockT<TBlockAttributes>::GetObjectBitDeltaForBucketIndex(uint bucketIndex)
{
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    if (HeapInfo::heapBucketSizes[bucketIndex].objectGranularity > HeapConstants::ObjectGranularity)
    {
        return HeapInfo::heapBucketSizes[bucketIndex].bucketSizeCat / HeapConstants::ObjectGranularity;
    }
    else
#endif
    {
        return bucketIndex + 1;
    }
}

#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
template <>
uint
SmallHeapBlockT<MediumAllocationBlockAttributes>::GetObjectBitDeltaForBucketIndex(uint bucketIndex)
{
    return HeapInfo::GetObjectSizeForBucketIndex<MediumAllocationBlockAttributes>(bucketIndex) / HeapConstants::ObjectGranularity;
}
#endif

template <class TBlockAttributes>
uint
SmallHeapBlockT<TBlockAttributes>::GetPageCount() const
{
    return TBlockAttributes::PageCount;
}

#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
template <>
uint
SmallHeapBlockT<MediumAllocationBlockAttributes>::GetUnusablePageCount()
{
    return ((MediumAllocationBlockAttributes::PageCount * AutoSystemInfo::PageSize) % this->objectSize) / AutoSystemInfo::PageSize;
}
#endif

#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::ProtectUnusablePages()
{
    size_t count = this->GetUnusablePageCount();
    if (count > 0)
    {
        char* startPage = this->address + (TBlockAttributes::PageCount - count) * AutoSystemInfo::PageSize;
        DWORD oldProtect;
        BOOL ret = ::VirtualProtect(startPage, count * AutoSystemInfo::PageSize, PAGE_READONLY, &oldProtect);
        Assert(ret && oldProtect == PAGE_READWRITE);
#ifdef RECYCLER_WRITE_WATCH
        if (!CONFIG_FLAG(ForceSoftwareWriteBarrier))
        {
            ::ResetWriteWatch(startPage, count*AutoSystemInfo::PageSize);
        }
#endif
    }
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::RestoreUnusablePages()
{
    size_t count = this->GetUnusablePageCount();
    if (count > 0)
    {
        char* startPage = (char*)this->address + (TBlockAttributes::PageCount - count) * AutoSystemInfo::PageSize;
        DWORD oldProtect;
        BOOL ret = ::VirtualProtect(startPage, count * AutoSystemInfo::PageSize, PAGE_READWRITE, &oldProtect);

#if DBG
        HeapBlock* block = this->heapBucket->heapInfo->recycler->heapBlockMap.GetHeapBlock(this->address);
        // only need to do this after the unusable page is already successfully protected
        // currently we don't have a flag to save that, but it should not fail after it successfully added to blockmap (see SetPage() implementation)
        if (block)
        {
            Assert(block == this);
            Assert(ret && oldProtect == PAGE_READONLY);
        }
#endif
    }
}
#else
template <>
void
SmallHeapBlockT<MediumAllocationBlockAttributes>::ProtectUnusablePages()
{
    size_t count = this->GetUnusablePageCount();
    if (count > 0)
    {
        char* startPage = this->address + (MediumAllocationBlockAttributes::PageCount - count) * AutoSystemInfo::PageSize;
        DWORD oldProtect;
        BOOL ret = ::VirtualProtect(startPage, count * AutoSystemInfo::PageSize, PAGE_READONLY, &oldProtect);
        Assert(ret && oldProtect == PAGE_READWRITE);
#ifdef RECYCLER_WRITE_WATCH
        if (!CONFIG_FLAG(ForceSoftwareWriteBarrier))
        {
            ::ResetWriteWatch(startPage, count*AutoSystemInfo::PageSize);
        }
#endif
    }
}

template <>
void
SmallHeapBlockT<MediumAllocationBlockAttributes>::RestoreUnusablePages()
{
    size_t count = this->GetUnusablePageCount();
    if (count > 0)
    {
        char* startPage = (char*)this->address + (MediumAllocationBlockAttributes::PageCount - count) * AutoSystemInfo::PageSize;
        DWORD oldProtect;
        BOOL ret = ::VirtualProtect(startPage, count * AutoSystemInfo::PageSize, PAGE_READWRITE, &oldProtect);

#if DBG
        HeapBlock* block = this->heapBucket->heapInfo->recycler->heapBlockMap.GetHeapBlock(this->address);
        // only need to do this after the unusable page is already successfully protected
        // currently we don't have a flag to save that, but it should not fail after it successfully added to blockmap (see SetPage() implementation)
        if (block)
        {
            Assert(block == this);
            Assert(ret && oldProtect == PAGE_READONLY);
        }
#endif
    }
}
#endif

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::ClearObjectInfoList()
{
    ushort count = this->objectCount;
    // the object info list is prefix to the object
    memset(((byte *)this) - count, 0, count);
}

template <class TBlockAttributes>
byte&
SmallHeapBlockT<TBlockAttributes>::ObjectInfo(uint index)
{
    // See SmallHeapBlockT<TBlockAttributes>::GetAllocPlusSize for layout description
    // the object info list is prefix to the object and in reverse index order
    Assert(index < this->objectCount);
    return *(((byte *)this) - index - 1);
}

template <class TBlockAttributes>
ushort
SmallHeapBlockT<TBlockAttributes>::GetExpectedFreeObjectCount() const
{
    Assert(this->GetRecycler()->IsSweeping());

    return objectCount - markCount;
}

template <class TBlockAttributes>
uint
SmallHeapBlockT<TBlockAttributes>::GetExpectedFreeBytes() const
{
    return GetExpectedFreeObjectCount() * objectSize;
}

template <class TBlockAttributes>
ushort
SmallHeapBlockT<TBlockAttributes>::GetExpectedSweepObjectCount() const
{
    return GetExpectedFreeObjectCount() - freeCount;
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::Init(ushort objectSize, ushort objectCount)
{
    Assert(objectCount != 0);
    Assert(TBlockAttributes::IsAlignedObjectSize(objectSize));

    Assert(this->next == nullptr);

    Assert(this->freeObjectList == nullptr);

    Assert(this->freeCount == 0);
#if ENABLE_PARTIAL_GC
    this->oldFreeCount = this->lastFreeCount = this->objectCount;
#else
    this->lastFreeCount = this->objectCount;
#endif
#if ENABLE_CONCURRENT_GC
    this->isPendingConcurrentSweep = false;
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        // This flag is to identify whether this block was made available for allocations during the concurrent sweep and still needs to be swept.
        this->isPendingConcurrentSweepPrep = false;
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        this->objectsAllocatedDuringConcurrentSweepCount = 0;
        this->hasFinishedSweepObjects = false;
        this->wasAllocatedFromDuringSweep = false;
        this->lastObjectsAllocatedDuringConcurrentSweepCount = 0;
#endif
    }
#endif
#endif

    Assert(!this->isInAllocator);
    Assert(!this->isClearedFromAllocator);
    Assert(!this->isIntegratedBlock);
}

template <class TBlockAttributes>
BOOL
SmallHeapBlockT<TBlockAttributes>::ReassignPages(Recycler * recycler)
{
    Assert(this->address == nullptr);
    Assert(this->segment == nullptr);

    PageSegment * segment;

    auto pageAllocator = this->GetPageAllocator();
    uint pagecount = this->GetPageCount();
    char * address = pageAllocator->AllocPagesPageAligned(pagecount, &segment);

    if (address == NULL)
    {
        return FALSE;
    }

#if ENABLE_PARTIAL_GC
    recycler->autoHeap.uncollectedNewPageCount += this->GetPageCount();
#endif
#ifdef RECYCLER_ZERO_MEM_CHECK
    if (!this->IsLeafBlock()
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_THREAD_PAGE
        && !this->IsWithBarrier()
#endif
        )
    {
        recycler->VerifyZeroFill(address, AutoSystemInfo::PageSize * this->GetPageCount());
    }
#endif

    if (!this->SetPage(address, segment, recycler))
    {
        this->GetPageAllocator()->SuspendIdleDecommit();
        this->ReleasePages(recycler);
        this->GetPageAllocator()->ResumeIdleDecommit();
        return FALSE;
    }

    RECYCLER_PERF_COUNTER_ADD(FreeObjectSize, this->GetPageCount() * AutoSystemInfo::PageSize);
    RECYCLER_PERF_COUNTER_ADD(SmallHeapBlockFreeObjectSize, this->GetPageCount() * AutoSystemInfo::PageSize);
    return TRUE;
}

template <class TBlockAttributes>
BOOL
SmallHeapBlockT<TBlockAttributes>::SetPage(__in_ecount_pagesize char * baseAddress, PageSegment * pageSegment, Recycler * recycler)
{
    char* address = baseAddress;

    Assert(HeapBlockMap32::GetLevel2Id(address) + (TBlockAttributes::PageCount - 1) < 256);

    this->segment = pageSegment;
    this->address = address;

    // Set up the page to have nothing is free
    Assert(this->freeObjectList == nullptr);
    Assert(this->IsFreeBitsValid());
    Assert(this->freeCount == 0);
    Assert(this->freeCount == this->GetFreeBitVector()->Count());
    Assert(this->objectCount == this->lastFreeCount);

    Assert(this->explicitFreeBits.Count() == 0);

#if ENABLE_CONCURRENT_GC
    Assert(recycler->IsConcurrentMarkState() || !recycler->IsMarkState() || recycler->IsCollectionDisabled());
#else
    Assert(!recycler->IsMarkState() || recycler->IsCollectionDisabled());
#endif

    Assert(this->bucketIndex <= 0xFF);

    // We use the block type directly here, without the getter so that we can tell on the heap block map,
    // whether the block is a medium block or not
    if (!recycler->heapBlockMap.SetHeapBlock(this->address, this->GetPageCount() - this->GetUnusablePageCount(), this, this->heapBlockType, (byte)this->bucketIndex))
    {
        return FALSE;
    }

    // Retrieve pointer to mark bits for this block and store it locally.
    // Note, mark bits aren't guaranteed to exist until after we register with HBM.
    this->markBits = recycler->heapBlockMap.GetMarkBitVectorForPages<TBlockAttributes::BitVectorCount>(this->address);
    Assert(this->markBits);

#if defined(_M_ARM32_OR_ARM64)
    // We need to ensure that the above writes to the SmallHeapBlock are visible to the background GC thread.
    // In particular, see Threshold 331596 -- we were seeing an old value for SmallHeapBlockT<TBlockAttributes>::markBits in ResetMarks.
    // which caused the bit vector Copy operation there to AV.
    // See also SmallHeapBlockT<TBlockAttributes>::ResetMarks.
    MemoryBarrier();
#endif

    this->ProtectUnusablePages();

    return TRUE;
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::ReleasePages(Recycler * recycler)
{
    Assert(recycler->collectionState != CollectionStateMark);
    Assert(segment != nullptr);
    Assert(address != nullptr);

#if DBG
    if (this->IsLeafBlock())
    {
        RecyclerVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("Releasing leaf block pages at address 0x%p\n"), address);
    }
#endif

    char* address = this->address;

#ifdef RECYCLER_FREE_MEM_FILL
    memset(address, DbgMemFill, AutoSystemInfo::PageSize * (this->GetPageCount()-this->GetUnusablePageCount()));
#endif

    if (this->GetUnusablePageCount() > 0)
    {
        this->RestoreUnusablePages();
    }

    this->GetPageAllocator()->ReleasePages(address, this->GetPageCount(), this->GetPageSegment());

    this->segment = nullptr;
    this->address = nullptr;
}

#if ENABLE_BACKGROUND_PAGE_FREEING
template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::BackgroundReleasePagesSweep(Recycler* recycler)
{
    recycler->heapBlockMap.ClearHeapBlock(address, this->GetPageCount() - this->GetUnusablePageCount());
    char* address = this->address;

    if (this->GetUnusablePageCount() > 0)
    {
        this->RestoreUnusablePages();
    }
    this->GetPageAllocator()->BackgroundReleasePages(address, this->GetPageCount(), this->GetPageSegment());

    this->address = nullptr;
    this->segment = nullptr;
    this->Reset();
}
#endif

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::ReleasePagesShutdown(Recycler * recycler)
{
#if DBG
    if (this->IsLeafBlock())
    {
        RecyclerVerboseTrace(recycler->GetRecyclerFlagsTable(), _u("Releasing leaf block pages at address 0x%p\n"), address);
    }

    RemoveFromHeapBlockMap(recycler);

    // Don't release the page in shut down, the page allocator will release them faster
    // Leaf block's allocator need not be closed
    // For non-large normal heap blocks ReleasePagesShutdown could be called during shutdown cleanup when the block is still pending concurrent
    // sweep i.e. it resides in the pendingSweepList of the RecyclerSweep instance. In this case the page allocator may not have been closed yet.
    Assert(this->IsLeafBlock() || this->GetPageAllocator()->IsClosed() || this->isPendingConcurrentSweep);
#endif

}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::RemoveFromHeapBlockMap(Recycler* recycler)
{
    recycler->heapBlockMap.ClearHeapBlock(address, this->GetPageCount() - this->GetUnusablePageCount());
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::ReleasePagesSweep(Recycler * recycler)
{
    RemoveFromHeapBlockMap(recycler);
    ReleasePages(recycler);
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::Reset()
{
    this->GetFreeBitVector()->ClearAll();

    this->freeCount = 0;
    this->markCount = 0;
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        this->hasFinishedSweepObjects = false;
        this->wasAllocatedFromDuringSweep = false;
        this->objectsMarkedDuringSweep = 0;
        this->objectsAllocatedDuringConcurrentSweepCount = 0;
        this->lastObjectsAllocatedDuringConcurrentSweepCount = 0;
#endif
        this->isPendingConcurrentSweepPrep = false;
    }
#endif

#if ENABLE_PARTIAL_GC
    this->oldFreeCount = this->lastFreeCount = this->objectCount;
#else
    this->lastFreeCount = this->objectCount;
#endif
    this->freeObjectList = nullptr;
    this->lastFreeObjectHead = nullptr;
    this->ClearObjectInfoList();

    this->isInAllocator = false;

#if DBG || defined(RECYCLER_STATS)
    this->GetDebugFreeBitVector()->ClearAll();
#endif

#if DBG
    this->isClearedFromAllocator = false;
    this->isIntegratedBlock = false;
#endif

    // There is no page associated with this heap block,
    // and therefore we should have no mark bits either
    this->markBits = nullptr;

    Assert(this->explicitFreeBits.Count() == 0);
}

// Map any object address to it's object index within the heap block
template <class TBlockAttributes>
ushort
SmallHeapBlockT<TBlockAttributes>::GetAddressIndex(void * objectAddress)
{
    Assert(objectAddress >= address && objectAddress < this->GetEndAddress());
    Assert(HeapInfo::IsAlignedAddress(objectAddress));
    Assert(HeapInfo::IsAlignedAddress(address));

    unsigned int offset = (unsigned int)((char*)objectAddress - address);
    offset = offset >> HeapConstants::ObjectAllocationShift;

    ushort index = validPointers.GetAddressIndex(offset);
    Assert(index == SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit ||
        index <= TBlockAttributes::MaxAddressBit);
    return index;
}

template <class TBlockAttributes>
typename SmallHeapBlockT<TBlockAttributes>::SmallHeapBlockBitVector const*
SmallHeapBlockT<TBlockAttributes>::GetInvalidBitVector()
{
    return HeapInfo::GetInvalidBitVector<TBlockAttributes>(objectSize);
}

template <class TBlockAttributes>
typename SmallHeapBlockT<TBlockAttributes>::BlockInfo const*
SmallHeapBlockT<TBlockAttributes>::GetBlockInfo()
{
    return HeapInfo::GetBlockInfo<TBlockAttributes>(objectSize);
}

template <class TBlockAttributes>
ushort
SmallHeapBlockT<TBlockAttributes>::GetInteriorAddressIndex(void * interiorAddress)
{
    Assert(interiorAddress >= address && interiorAddress < this->GetEndAddress());
    Assert(HeapInfo::IsAlignedAddress(address));

    unsigned int offset = (unsigned int)((char*)interiorAddress - address);
    offset = offset >> HeapConstants::ObjectAllocationShift;

    ushort index = validPointers.GetInteriorAddressIndex(offset);
    Assert(index == SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit ||
        index <= TBlockAttributes::MaxAddressBit);
    return index;
}


template <class TBlockAttributes>
BOOL
SmallHeapBlockT<TBlockAttributes>::IsInFreeObjectList(void * objectAddress)
{
    FreeObject * freeObject = this->freeObjectList;
    while (freeObject != nullptr)
    {
        if (freeObject == objectAddress)
        {
            return true;
        }
        freeObject = freeObject->GetNext();
    }
    return false;
}

template <class TBlockAttributes>
template <typename TBlockType>
bool
SmallHeapBlockT<TBlockAttributes>::FindHeapObjectImpl(void* objectAddress, Recycler * recycler, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject)
{
    if (flags & FindHeapObjectFlags_AllowInterior)
    {
        objectAddress = (void*) this->GetRealAddressFromInterior(objectAddress);
        if (objectAddress == nullptr)
        {
            return false;
        }
    }

    ushort index = GetAddressIndex(objectAddress);
    Assert(index != SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit);

    if (index == SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit)
    {
        return false;
    }

    // If we have pending object, we still need to check the free bit if the caller requested the attribute to be correct
    bool const disableCheck = ((flags & FindHeapObjectFlags_NoFreeBitVerify) != 0) ||
        ((flags & FindHeapObjectFlags_VerifyFreeBitForAttribute) != 0 && !this->HasPendingDisposeObjects());
    if (!disableCheck)
    {
        // REVIEW: Checking if an object if free is strictly not necessary
        // In all case, we should have a valid object, For memory protect heap, this is just to make sure we don't
        // free pointers that are invalid.
#if ENABLE_CONCURRENT_GC
        if (recycler->IsConcurrentSweepExecutingState())
        {
            // TODO: unless we know the state of the heap block, we don't know.
            // skip the check for now.
        }
        else
#endif
        {
            if (flags & FindHeapObjectFlags_ClearedAllocators)
            {
                // Heap enum has some case where it allocates, so we can't assert
                Assert(((HeapBucketT<TBlockType> *)this->heapBucket)->AllocatorsAreEmpty() || recycler->isHeapEnumInProgress);
            }
            else if (this->IsInAllocator())
            {
                ((HeapBucketT<TBlockType> *)this->heapBucket)->UpdateAllocators();
            }

            // REVIEW allocation heuristics
            if (this->EnsureFreeBitVector()->Test(this->GetObjectBitDelta() * index))
            {
                return false;
            }
        }
    }

    byte& attributes = ObjectInfo(index);
    heapObject = RecyclerHeapObjectInfo(objectAddress, recycler, this, &attributes);
    return true;
}

template <class TBlockAttributes>
BOOL
SmallHeapBlockT<TBlockAttributes>::IsValidObject(void* objectAddress)
{
    if (objectAddress < this->GetAddress() || objectAddress >= this->GetEndAddress())
    {
        return false;
    }
    ushort index = GetAddressIndex(objectAddress);
    if (index == SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit)
    {
        return false;
    }
#if DBG
    return !this->GetDebugFreeBitVector()->Test(GetAddressBitIndex(objectAddress));
#else
    return true;
#endif
}

template <class TBlockAttributes>
bool
SmallHeapBlockT<TBlockAttributes>::IsInAllocator() const
{
    return isInAllocator;
}

template <class TBlockAttributes>
bool
SmallHeapBlockT<TBlockAttributes>::HasPendingDisposeObjects()
{
    return this->IsAnyFinalizableBlock() && this->AsFinalizableBlock<TBlockAttributes>()->HasPendingDisposeObjects();
}

template <class TBlockAttributes>
bool
SmallHeapBlockT<TBlockAttributes>::HasAnyDisposeObjects()
{
    return this->IsAnyFinalizableBlock() && this->AsFinalizableBlock<TBlockAttributes>()->HasAnyDisposeObjects();
}

template <class TBlockAttributes>
Recycler *
SmallHeapBlockT<TBlockAttributes>::GetRecycler() const
{
#if DBG
    return this->heapBucket->heapInfo->recycler;
#else
    return nullptr;
#endif
}

#if DBG
template <class TBlockAttributes>
HeapInfo *
SmallHeapBlockT<TBlockAttributes>::GetHeapInfo() const
{
    return this->heapBucket->heapInfo;
}

template <class TBlockAttributes>
BOOL
SmallHeapBlockT<TBlockAttributes>::IsFreeObject(void * objectAddress)
{
    if (objectAddress < this->GetAddress() || objectAddress >= this->GetEndAddress())
    {
        return false;
    }
    ushort index = GetAddressIndex(objectAddress);
    if (index == SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit)
    {
        return false;
    }

    return this->GetDebugFreeBitVector()->Test(GetAddressBitIndex(objectAddress));
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::VerifyMarkBitVector()
{
    this->GetRecycler()->heapBlockMap.template VerifyMarkCountForPages<TBlockAttributes::BitVectorCount>(this->address, TBlockAttributes::PageCount);
}

template <class TBlockAttributes>
bool
SmallHeapBlockT<TBlockAttributes>::IsClearedFromAllocator() const
{
    return isClearedFromAllocator;
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::SetIsClearedFromAllocator(bool value)
{
    isClearedFromAllocator = value;
}
#endif

template <class TBlockAttributes>
byte *
SmallHeapBlockT<TBlockAttributes>::GetRealAddressFromInterior(void * interiorAddress)
{
    Assert(interiorAddress >= this->address && interiorAddress < this->address + AutoSystemInfo::PageSize * this->GetPageCount());
    ushort index = GetInteriorAddressIndex(interiorAddress);
    if (index != SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit)
    {
        return (byte *)this->address + index * this->GetObjectSize();
    }
    return nullptr;
}

template <class TBlockAttributes>
bool
SmallHeapBlockT<TBlockAttributes>::TestObjectMarkedBit(void* objectAddress)
{
    Assert(this->address != nullptr);
    Assert(this->segment != nullptr);

    uint bitIndex = GetAddressBitIndex(objectAddress);
    Assert(IsValidBitIndex(bitIndex));

    return this->GetMarkedBitVector()->Test(bitIndex) != 0;
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::SetObjectMarkedBit(void* objectAddress)
{
    Assert(this->address != nullptr);
    Assert(this->segment != nullptr);

    uint bitIndex = GetAddressBitIndex(objectAddress);
    Assert(IsValidBitIndex(bitIndex));

    this->GetMarkedBitVector()->Set(bitIndex);
}

#ifdef RECYCLER_MEMORY_VERIFY
template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::SetExplicitFreeBitForObject(void* objectAddress)
{
    Assert(this->address != nullptr);
    Assert(this->segment != nullptr);

    uint bitIndex = GetAddressBitIndex(objectAddress);
    Assert(IsValidBitIndex(bitIndex));

    BOOLEAN wasSet = this->explicitFreeBits.TestAndSet(bitIndex);
    Assert(!wasSet);
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::ClearExplicitFreeBitForObject(void* objectAddress)
{
    Assert(this->address != nullptr);
    Assert(this->segment != nullptr);

    uint bitIndex = GetAddressBitIndex(objectAddress);
    Assert(IsValidBitIndex(bitIndex));
    BOOLEAN wasSet = this->explicitFreeBits.TestAndClear(bitIndex);
    Assert(wasSet);
}

#endif

#ifdef RECYCLER_VERIFY_MARK

#if DBG
void HeapBlock::PrintVerifyMarkFailure(Recycler* recycler, char* objectAddress, char* target)
{
    // Due to possible GC mark optimization, the pointers may point to object
    // internal and "unaligned". Align them then FindHeapBlock.
    HeapBlock* block = recycler->FindHeapBlock(HeapInfo::GetAlignedAddress(objectAddress));
    if (block == nullptr)
    {
        return;
    }
    HeapBlock* targetBlock = recycler->FindHeapBlock(HeapInfo::GetAlignedAddress(target));
    if (targetBlock == nullptr)
    {
        return;
    }

#ifdef TRACK_ALLOC
    Recycler::TrackerData* trackerData = nullptr;
    Recycler::TrackerData* targetTrackerData = nullptr;
    const char* typeName = nullptr;
    const char* targetTypeName = nullptr;
    uint offset = 0;
    uint targetOffset = 0;
    char* objectStartAddress = nullptr;
    char* targetStartAddress = nullptr;

    if (targetBlock->IsLargeHeapBlock())
    {
        targetOffset = (uint)(target - (char*)((LargeHeapBlock*)targetBlock)->GetRealAddressFromInterior(target));
    }
    else
    {
        targetOffset = (uint)(target - targetBlock->GetAddress()) % targetBlock->GetObjectSize(nullptr);
    }

    if (targetOffset != 0)
    {
        // "target" points to internal of an object. This is not a GC pointer.
        return;
    }

    if (Recycler::DoProfileAllocTracker())
    {
        // need CheckMemoryLeak or KeepRecyclerTrackData flag to have the tracker data and show following detailed info
#if  defined(__clang__) && !defined(_MSC_VER)
        auto getDemangledName = [](const type_info* typeinfo) ->const char*
        {
            int status;
            char buffer[1024];
            size_t buflen = 1024;
            char* name = abi::__cxa_demangle(typeinfo->name(), buffer, &buflen, &status);
            if (status != 0)
            {
                Output::Print(_u("Demangle failed: result=%d, buflen=%d\n"), status, buflen);
            }
            char* demangledName = (char*)malloc(buflen);
            memcpy(demangledName, name, buflen);
            return demangledName;
        };
#else
        auto getDemangledName = [](const type_info* typeinfo) ->const char*
        {
            return typeinfo->name();
        };
#endif

        if (block->IsLargeHeapBlock())
        {
            offset = (uint)(objectAddress - (char*)((LargeHeapBlock*)block)->GetRealAddressFromInterior(objectAddress));
        }
        else
        {
            offset = (uint)(objectAddress - block->address) % block->GetObjectSize(objectAddress);
        }
        objectStartAddress = objectAddress - offset;
        trackerData = (Recycler::TrackerData*)block->GetTrackerData(objectStartAddress);
        if (trackerData)
        {
            typeName = getDemangledName(trackerData->typeinfo);
            if (trackerData->isArray)
            {
                Output::Print(_u("Missing Barrier\nOn array of %S\n"), typeName);
#ifdef STACK_BACK_TRACE
                if (CONFIG_FLAG(KeepRecyclerTrackData))
                {
                    Output::Print(_u("Allocation stack:\n"));
                    ((StackBackTrace*)(trackerData + 1))->Print();
                }
#endif
            }
            else
            {
                auto dumpFalsePositive = [&]()
                {
                    if (CONFIG_FLAG(Verbose))
                    {
                        Output::Print(_u("False Positive: %S+0x%x => 0x%p -> 0x%p\n"), typeName, offset, objectAddress, target);
                    }
                };

                if (IsLikelyRuntimeFalseReference(objectStartAddress, offset, typeName))
                {
                    dumpFalsePositive();
                    return;
                }

                //TODO: (leish)(swb) analyze pdb to check if the field is a pointer field or not
                Output::Print(_u("Missing Barrier\nOn type %S+0x%x\n"), typeName, offset);
            }
        }


        targetStartAddress = target - targetOffset;
        targetTrackerData = (Recycler::TrackerData*)targetBlock->GetTrackerData(targetStartAddress);


        if (targetTrackerData)
        {
            targetTypeName = getDemangledName(targetTrackerData->typeinfo);
            if (targetTrackerData->isArray)
            {
                Output::Print(_u("Target type (missing barrier field type) is array item of %S\n"), targetTypeName);
#ifdef STACK_BACK_TRACE
                if (CONFIG_FLAG(KeepRecyclerTrackData))
                {
                    Output::Print(_u("Allocation stack:\n"));
                    ((StackBackTrace*)(targetTrackerData + 1))->Print();
                }
#endif
            }
            else if (targetOffset == 0)
            {
                Output::Print(_u("Target type (missing barrier field type) is %S\n"), targetTypeName);
            }
            else
            {
                Output::Print(_u("Target type (missing barrier field type) is pointing to %S+0x%x\n"), targetTypeName, targetOffset);
            }
        }

        Output::Print(_u("---------------------------------\n"));
    }
#endif

    Output::Print(_u("Missing barrier on 0x%p, target is 0x%p\n"), objectAddress, target);
    AssertMsg(false, "Missing barrier.");
}
#endif  // DBG

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::VerifyMark()
{
    Assert(!this->needOOMRescan);

    SmallHeapBlockBitVector * marked = this->GetMarkedBitVector();
    SmallHeapBlockBitVector tempFreeBits;
    this->BuildFreeBitVector(&tempFreeBits);
    SmallHeapBlockBitVector * free = &tempFreeBits;
    SmallHeapBlockBitVector const * invalid = this->GetInvalidBitVector();
    uint objectWordCount = this->GetObjectWordCount();
    Recycler * recycler = this->heapBucket->heapInfo->recycler;

    FOREACH_BITSET_IN_FIXEDBV(bitIndex, marked)
    {
        if (!free->Test(bitIndex) && !invalid->Test(bitIndex))
        {
            Assert(IsValidBitIndex(bitIndex));
            uint objectIndex = GetObjectIndexFromBitIndex((ushort)bitIndex);

            Assert((this->ObjectInfo(objectIndex) & NewTrackBit) == 0);

            // NOTE: We can't verify mark for software write barrier blocks, because they may have
            // non-pointer updates that don't trigger the write barrier, but still look like a false reference.
            // Thus, when we get here, we'll see a false reference that isn't marked.
            // Since this situation is hard to detect, just don't verify mark for write barrier blocks.
            // We could fix this if we had object layout info.

            if (!this->IsLeafBlock()
#ifdef RECYCLER_WRITE_BARRIER
                && (!this->IsWithBarrier() || CONFIG_FLAG(ForceSoftwareWriteBarrier))
#endif
                )
            {
                if ((ObjectInfo(objectIndex) & LeafBit) == 0)
                {
                    char * objectAddress = this->address + objectIndex * objectSize;
                    for (uint i = 0; i < objectWordCount; i++)
                    {
                        void* target = *(void**) objectAddress;
                        if (recycler->VerifyMark(objectAddress, target))
                        {
#if DBG && GLOBAL_ENABLE_WRITE_BARRIER
                            if (CONFIG_FLAG(ForceSoftwareWriteBarrier) && CONFIG_FLAG(VerifyBarrierBit))
                            {
                                this->WBVerifyBitIsSet(objectAddress);
                            }
#endif
                        }

                        objectAddress += sizeof(void *);
                    }
                }
            }
        }
    }
    NEXT_BITSET_IN_FIXEDBV;
}

template <class TBlockAttributes>
bool
SmallHeapBlockT<TBlockAttributes>::VerifyMark(void * objectAddress, void * target)
{
    // Because we mark through new object, we might have a false reference
    // somewhere that we have scanned before this new block is allocated
    // so the object will not be marked even though it looks like a reference
    // Can't verify when the block is new
    if (this->heapBucket->GetRecycler()->heapBlockMap.IsAddressInNewChunk(target))
    {
        return false;
    }

    ushort bitIndex = GetAddressBitIndex(target);
    bool isMarked = this->GetMarkedBitVector()->Test(bitIndex) == TRUE;
#if DBG
    if (!isMarked)
    {
        PrintVerifyMarkFailure(this->GetRecycler(), (char*)objectAddress, (char*)target);
    }
#else
    if (!isMarked)
    {
        DebugBreak();
    }
#endif
    return isMarked;
}

#endif

#ifdef RECYCLER_STRESS
template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::InduceFalsePositive(Recycler * recycler)
{
    // Induce a false positive mark by marking the first object on the free list, if any.
    // Note that if the block is in the allocator, freeObjectList is not up to date.
    // So we may be marking an already-allocated block, but that's okay --
    // we call TryMark so that normal processing (including tracked object processing, etc)
    // will occur just as if we had a false reference to this object previously.

    void * falsePositive = this->freeObjectList;
    if (falsePositive != nullptr)
    {
        recycler->TryMarkNonInterior(falsePositive, nullptr);
    }
}
#endif

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::ClearAllAllocBytes()
{
#if ENABLE_PARTIAL_GC
    this->oldFreeCount = this->lastFreeCount = this->freeCount;
#else
    this->lastFreeCount = this->freeCount;
#endif
}

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::ResetConcurrentSweepAllocationCounts()
{
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && this->objectsAllocatedDuringConcurrentSweepCount > 0)
    {
        // Reset the count of objects allocated during this concurrent sweep; so we will start afresh the next time around.
        Assert(this->objectsAllocatedDuringConcurrentSweepCount == this->objectsMarkedDuringSweep);
        this->lastObjectsAllocatedDuringConcurrentSweepCount = this->objectsAllocatedDuringConcurrentSweepCount;
        this->objectsAllocatedDuringConcurrentSweepCount = 0;
        this->objectsMarkedDuringSweep = 0;
    }
}
#endif
#endif

#if ENABLE_PARTIAL_GC
template <class TBlockAttributes>
bool
SmallHeapBlockT<TBlockAttributes>::DoPartialReusePage(RecyclerSweep const& recyclerSweep, uint& expectFreeByteCount)
{
    // Partial GC page reuse heuristic

    Assert(recyclerSweep.InPartialCollectMode());
    expectFreeByteCount = GetExpectedFreeBytes();
    // PartialCollectSmallHeapBlockReuseMinFreeBytes is calculated by dwPageSize* efficacy. If efficacy is
    // high (== 1), and dwPageSize % objectSize != 0, all the pages in the bucket will be partial, and that
    // could increase in thread sweep time.
    // OTOH, if the object size is really large, the calculation below will reduce the chance for a page to be
    // partial. we might need to watch out for that.
    return (expectFreeByteCount + objectSize >= recyclerSweep.GetManager()->GetPartialCollectSmallHeapBlockReuseMinFreeBytes());
}

#if DBG
// do debug assert for partial block that we are not going to sweep
template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::SweepVerifyPartialBlock(Recycler * recycler)
{
    Assert(!this->IsLeafBlock());
    // nothing in the partialHeapBlockList is sweepable
    Assert(GetExpectedSweepObjectCount() == 0);
}
#endif

template <class TBlockAttributes>
uint
SmallHeapBlockT<TBlockAttributes>::GetAndClearUnaccountedAllocBytes()
{
    Assert(this->lastFreeCount >= this->freeCount);
    const ushort currentFreeCount = this->freeCount;

    uint unaccountedAllocBytes = (this->lastFreeCount - currentFreeCount) * this->objectSize;
    this->lastFreeCount = currentFreeCount;
    return unaccountedAllocBytes;
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::AdjustPartialUncollectedAllocBytes(RecyclerSweep& recyclerSweep, uint const expectSweepCount)
{
    const uint allObjectCount = this->objectCount;
    const ushort currentFreeCount = this->freeCount;
    Assert(this->lastFreeCount == currentFreeCount);

    uint newAllocatedCount = this->oldFreeCount - currentFreeCount;
    this->oldFreeCount = currentFreeCount;

    uint newObjectExpectSweepCount = expectSweepCount;
#if ENABLE_CONCURRENT_GC
    if (expectSweepCount != 0 && !recyclerSweep.InPartialCollect())
    {
        // We don't know which objects that we are going sweep are old and which object are new
        // So just assume one way or the other by the amount of old vs. new object in the block
        const uint allocatedObjectCount = allObjectCount - currentFreeCount;
        Assert(allocatedObjectCount >= newAllocatedCount);
        const uint oldObjectCount = allocatedObjectCount - newAllocatedCount;
        if (oldObjectCount < newAllocatedCount)
        {
            // count all of the swept object as new, but don't exceed the amount we allocated
            if (newObjectExpectSweepCount > newAllocatedCount)
            {
                newObjectExpectSweepCount = newAllocatedCount;
            }
        }
        else
        {
            // count all of the swept object as old
            newObjectExpectSweepCount = 0;
        }
    }
#endif

    // The page can be old, or it is full (where we set lastFreeCount to 0)
    // Otherwise, the newly allocated count must be bigger then the expect sweep count
    Assert(newAllocatedCount >= newObjectExpectSweepCount);
    Assert(this->lastUncollectedAllocBytes >= newObjectExpectSweepCount * this->objectSize);

    recyclerSweep.GetManager()->SubtractSweepNewObjectAllocBytes(newObjectExpectSweepCount * this->objectSize);
}
#endif  // RECYCLER_VERIFY_MARK

template <class TBlockAttributes>
uint
SmallHeapBlockT<TBlockAttributes>::GetMarkCountForSweep()
{
    Assert(IsFreeBitsValid());

    // Make a local copy of mark bits, so we don't modify the actual mark bits.
    SmallHeapBlockBitVector temp;
    temp.Copy(this->GetMarkedBitVector());

    // Remove any invalid bits that may have been set
    temp.Minus(this->GetInvalidBitVector());

    // Remove the mark bit for things that are still free
    if (this->freeCount != 0)
    {
        temp.Minus(this->GetFreeBitVector());
    }

    return temp.Count();
}

template <class TBlockAttributes>
SweepState
SmallHeapBlockT<TBlockAttributes>::Sweep(RecyclerSweep& recyclerSweep, bool queuePendingSweep, bool allocable, ushort finalizeCount, bool hasPendingDispose)
{
    Assert(this->address != nullptr);
    Assert(this->segment != nullptr);
#if ENABLE_CONCURRENT_GC
    Assert(!this->isPendingConcurrentSweep);
#endif

#if DBG && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    // In concurrent sweep pass1, we mark the object directly in the mark bit vector for objects allocated during the sweep to prevent them from getting swept during the ongoing sweep itself.
    // This will make the mark bit vector on the HeapBlockMap out-of-date w.r.t. these newly allocated objects.
    if (!this->wasAllocatedFromDuringSweep)
#endif
    {
        DebugOnly(VerifyMarkBitVector());
    }

    if (allocable)
    {
        // This block has been allocated from since the last GC.
        // We need to update its free bit vector so we can use it below.
        DebugOnly(ushort currentFreeCount = (ushort)this->GetFreeBitVector()->Count());
        Assert(freeCount == currentFreeCount);
#if ENABLE_PARTIAL_GC
        Assert(this->lastFreeCount == 0 || this->oldFreeCount == this->lastFreeCount);
#endif

        this->EnsureFreeBitVector();

        Assert(this->lastFreeCount >= this->freeCount);
#if ENABLE_PARTIAL_GC
        Assert(this->oldFreeCount >= this->freeCount);
#endif

#if ENABLE_PARTIAL_GC
        // Accounting for partial heuristics
        recyclerSweep.GetManager()->AddUnaccountedNewObjectAllocBytes(this);
#endif
    }

    Assert(this->freeCount == this->GetFreeBitVector()->Count());
    RECYCLER_SLOW_CHECK(CheckFreeBitVector(true));

    const uint localMarkCount = this->GetMarkCountForSweep();
    this->markCount = (ushort)localMarkCount;
    Assert(markCount <= objectCount - this->freeCount);

    const uint expectFreeCount = objectCount - localMarkCount;
    Assert(expectFreeCount >= this->freeCount);

    uint expectSweepCount = expectFreeCount - this->freeCount;

    Assert(!this->IsLeafBlock() || finalizeCount == 0);

    Recycler * recycler = recyclerSweep.GetRecycler();
    RECYCLER_STATS_INC(recycler, heapBlockCount[this->GetHeapBlockType()]);

#if ENABLE_PARTIAL_GC
    if (recyclerSweep.GetManager()->DoAdjustPartialHeuristics() && allocable)
    {
        this->AdjustPartialUncollectedAllocBytes(recyclerSweep, expectSweepCount);
    }
#endif
    DebugOnly(this->lastUncollectedAllocBytes = 0);

    bool noRealObjectsMarked = (localMarkCount == 0);

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        Assert(!this->IsAnyFinalizableBlock() || !this->isPendingConcurrentSweepPrep);
        // This heap block is ready to be swept concurrently.
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        this->hasFinishedSweepObjects = false;
#endif
        this->isPendingConcurrentSweepPrep = false;
    }
#endif

    const bool isAllFreed = (finalizeCount == 0 && noRealObjectsMarked && !hasPendingDispose);
    if (isAllFreed)
    {
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
        if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
        {
            AssertMsg(this->objectsAllocatedDuringConcurrentSweepCount == 0, "This block shouldn't be considered EMPTY if we allocated from it during concurrent sweep.");
        }
#endif
            recycler->NotifyFree(this);

            Assert(!this->HasPendingDisposeObjects());

#ifdef RECYCLER_TRACE
            recycler->PrintBlockStatus(this->heapBucket, this, _u("[**26**] ending sweep Pass1, state returned SweepStateEmpty."));
#endif
            return SweepStateEmpty;
    }

    RECYCLER_STATS_ADD(recycler, heapBlockFreeByteCount[this->GetHeapBlockType()], expectFreeCount * this->objectSize);

    Assert(!hasPendingDispose || (this->freeCount != 0));
    SweepState state = SweepStateSwept;

    if (hasPendingDispose)
    {
        state = SweepStatePendingDispose;
    }

    if (expectSweepCount == 0)
    {
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
        if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
        {
            this->ResetConcurrentSweepAllocationCounts();
        }
#endif
#endif

        // nothing has been freed
#ifdef RECYCLER_TRACE
        if (recycler->GetRecyclerFlagsTable().Trace.IsEnabled(Js::ConcurrentSweepPhase) && CONFIG_FLAG_RELEASE(Verbose))
        {
            SweepState stateReturned = (this->freeCount == 0) ? SweepStateFull : state;
            CollectionState collectionState = recycler->collectionState;
            Output::Print(_u("[GC #%d] [HeapBucket 0x%p] HeapBlock 0x%p %s %d [CollectionState: %d] \n"), recycler->collectionCount, this->heapBucket, this, _u("[**37**] heapBlock swept. State returned:"), stateReturned, collectionState);
        }
#endif
        return (this->freeCount == 0) ? SweepStateFull : state;
    }

    RECYCLER_STATS_INC(recycler, heapBlockSweptCount[this->GetHeapBlockType()]);

    // We need to sweep in thread if there are any finalizable object.
    // So that the PrepareFinalize() can be called before concurrent sweep
    // and other finalizer.  This gives the object an opportunity before any
    // other script can be ran to clean up their references/states that are not
    // valid since we determine the object is not live any more.
    //
    // An example is the ITrackable's tracking alias.  The reference to the alias
    // object needs to be clear so that the reference will not be given out again
    // in other script during concurrent sweep or finalizer called before.

#if ENABLE_CONCURRENT_GC
    if (queuePendingSweep)
    {
        Assert(finalizeCount == 0);
        Assert(!this->HasPendingDisposeObjects());

        recyclerSweep.SetHasPendingSweepSmallHeapBlocks();
        RECYCLER_STATS_INC(recycler, heapBlockConcurrentSweptCount[this->GetHeapBlockType()]);
        // This heap block has objects that need to be swept concurrently.
        this->isPendingConcurrentSweep = true;
#ifdef RECYCLER_TRACE
        if (recycler->GetRecyclerFlagsTable().Trace.IsEnabled(Js::ConcurrentSweepPhase))
        {
            recycler->PrintBlockStatus(this->heapBucket, this, _u("[**29**] heapBlock swept. State returned: SweepStatePendingSweep"));
    }
#endif
        return SweepStatePendingSweep;
    }
#else
    Assert(!recyclerSweep.IsBackground());
#endif

#ifdef RECYCLER_TRACE
    recycler->PrintBlockStatus(this->heapBucket, this, _u("[**16**] calling SweepObjects."));
#endif
    SweepObjects<SweepMode_InThread>(recycler);
    if (HasPendingDisposeObjects())
    {
        Assert(finalizeCount != 0);
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
        if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
        {
            AssertMsg(this->objectsAllocatedDuringConcurrentSweepCount == 0, "Allocations during concurrent sweep not supported for finalizable blocks.");
        }
#endif

        return SweepStatePendingDispose;
    }

    // Already swept, no more work to be done. Put it back to the queue.
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && !this->IsAnyFinalizableBlock())
    {
#ifdef RECYCLER_TRACE
        if (recycler->GetRecyclerFlagsTable().Trace.IsEnabled(Js::ConcurrentSweepPhase) && CONFIG_FLAG_RELEASE(Verbose))
        {
            SweepState stateReturned = (this->freeCount == 0) ? SweepStateFull : state;
            CollectionState collectionState = recycler->collectionState;
            Output::Print(_u("[GC #%d] [HeapBucket 0x%p] HeapBlock 0x%p %s %d [CollectionState: %d] \n"), recycler->collectionCount, this->heapBucket, this, _u("[**38**] heapBlock swept. State returned:"), stateReturned, collectionState);
        }
#endif
        // We always need to check the free count as we may have allocated from this block during concurrent sweep.
        return (this->freeCount == 0) ? SweepStateFull : state;
    }
    else
#endif
    {
        return state;
    }
}

#if DBG
template <class TBlockAttributes>
uint
SmallHeapBlockT<TBlockAttributes>::GetMarkCountOnHeapBlockMap() const
{
    uint heapBlockMapMarkCount = 0;
    char* startPage = this->GetAddress();
    char* endPage = this->GetEndAddress();
    const HeapBlockMap& blockMap = this->GetRecycler()->heapBlockMap;
    for (char* page = startPage; page < endPage; page += AutoSystemInfo::PageSize)
    {
        heapBlockMapMarkCount += blockMap.GetPageMarkCount(page);
    }
    return heapBlockMapMarkCount;
}
#endif

template <class TBlockAttributes>
template <SweepMode mode>
void
SmallHeapBlockT<TBlockAttributes>::SweepObjects(Recycler * recycler)
{
#if ENABLE_CONCURRENT_GC
    Assert(mode == SweepMode_InThread || this->isPendingConcurrentSweep);
    Assert(mode == SweepMode_InThread || !this->IsAnyFinalizableBlock());
#else
    Assert(mode == SweepMode_InThread);
#endif
    Assert(this->IsFreeBitsValid());
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    AssertMsg(!hasFinishedSweepObjects, "Block in SweepObjects more than once during the ongoing sweep.");
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        Assert(this->markCount != 0 || this->objectsAllocatedDuringConcurrentSweepCount > 0 || this->isForceSweeping || this->IsAnyFinalizableBlock());
    }
    else
#endif
    {
        Assert(this->markCount != 0 || this->isForceSweeping || this->IsAnyFinalizableBlock());
    }

    Assert(this->markCount == this->GetMarkCountForSweep());

#if DBG && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    // In concurrent sweep pass1, we mark the object directly in the mark bit vector for objects allocated during the sweep to prevent them from getting swept during the ongoing sweep itself.
    // This will make the mark bit vector on the HeapBlockMap out-of-date w.r.t. these newly allocated objects.
    if (!this->wasAllocatedFromDuringSweep)
#endif
    {
        DebugOnly(VerifyMarkBitVector());
    }

    SmallHeapBlockBitVector * marked = this->GetMarkedBitVector();

    DebugOnly(uint expectedSweepCount = objectCount - freeCount - markCount);
#if DBG && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        Assert(expectedSweepCount != 0 || this->isForceSweeping || this->objectsAllocatedDuringConcurrentSweepCount != 0);
    }
    else
#endif
    {
        Assert(expectedSweepCount != 0 || this->isForceSweeping);
    }

    DebugOnly(uint sweepCount = 0);

    const uint localSize = objectSize;
    const uint localObjectCount = objectCount;
    const char* objectAddress = address;
    uint objectBitDelta = this->GetObjectBitDelta();

    for (uint objectIndex = 0, bitIndex = 0; objectIndex < localObjectCount; objectIndex++, bitIndex += objectBitDelta)
    {
        Assert(IsValidBitIndex(bitIndex));

        RECYCLER_STATS_ADD(recycler, objectSweepScanCount, !isForceSweeping);

        if (!marked->Test(bitIndex))
        {
            if (!this->GetFreeBitVector()->Test(bitIndex))
            {
                Assert((this->ObjectInfo(objectIndex) & ImplicitRootBit) == 0);
                FreeObject* addr = (FreeObject*)objectAddress;

#if ENABLE_PARTIAL_GC && ENABLE_CONCURRENT_GC
                if (mode != SweepMode_ConcurrentPartial)
#endif
                {
                    // Don't call NotifyFree if we are doing a partial sweep.
                    // Since we are not actually collecting the object, we will do the NotifyFree later
                    // when the object is actually collected in a future Sweep.
                    recycler->NotifyFree((char *)addr, this->objectSize);
                }
#if DBG
                sweepCount++;
#endif
                SweepObject<mode>(recycler, objectIndex, addr);
            }
        }

#if DBG
        if (marked->Test(bitIndex))
        {
            Assert((ObjectInfo(objectIndex) & NewTrackBit) == 0);
        }
#endif

        objectAddress += localSize;
    }

    Assert(sweepCount == expectedSweepCount);

#if ENABLE_CONCURRENT_GC
    this->isPendingConcurrentSweep = false;
#endif

#if ENABLE_PARTIAL_GC && ENABLE_CONCURRENT_GC
    if (mode == SweepMode_ConcurrentPartial)
    {
        Assert(recycler->inPartialCollectMode);

        // We didn't actually collect anything, so the free bit vector should still be valid.
        Assert(IsFreeBitsValid());
    }
    else
#endif
    {
        // Update the free bit vector
        // Need to update even if there are not swept object because finalizable object are
        // consider freed but not on the free list.
        ushort currentFreeCount = GetExpectedFreeObjectCount();

        this->GetFreeBitVector()->OrComplimented(marked);
        this->GetFreeBitVector()->Minus(this->GetInvalidBitVector());
#if ENABLE_PARTIAL_GC
        this->oldFreeCount = this->lastFreeCount = this->freeCount = currentFreeCount;
#else
        this->lastFreeCount = this->freeCount = currentFreeCount;
#endif

        this->lastFreeObjectHead = this->freeObjectList;
    }

    // While allocations are allowed during concurrent sweep into still unswept blocks the
    // free bit vectors are not valid yet.
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && defined(RECYCLER_SLOW_CHECK_ENABLED)
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc) && this->objectsAllocatedDuringConcurrentSweepCount == 0)
#endif
    {
        RECYCLER_SLOW_CHECK(CheckFreeBitVector(true));
    }

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        this->ResetConcurrentSweepAllocationCounts();
    }
#endif
#endif

    // The count of marked, non-free objects should still be the same
    Assert(this->markCount == this->GetMarkCountForSweep());

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    DebugOnly(this->hasFinishedSweepObjects = true);
#endif

#ifdef RECYCLER_TRACE
    recycler->PrintBlockStatus(this->heapBucket, this, _u("[**30**] finished SweepObjects, heapblock SWEPT."));
#endif
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::EnqueueProcessedObject(FreeObject ** list, void* objectAddress, uint index)
{
    Assert(GetAddressIndex(objectAddress) == index);

    Assert(index != SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit);
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    Assert(this->objectCount != 1);
#endif

#if DBG || defined(RECYCLER_STATS)
    if (list == &this->freeObjectList)
    {
        BOOL isSet = this->GetDebugFreeBitVector()->TestAndSet(GetAddressBitIndex(objectAddress));
        Assert(!isSet);
    }
#endif
    FillFreeMemory(objectAddress, objectSize);

    FreeObject * freeObject = (FreeObject *)objectAddress;
    freeObject->SetNext(*list);
    *list = freeObject;

#if DBG && GLOBAL_ENABLE_WRITE_BARRIER
    if (CONFIG_FLAG(ForceSoftwareWriteBarrier) && CONFIG_FLAG(RecyclerVerifyMark))
    {
        this->WBClearObject((char*)objectAddress);
    }
#endif

    // clear the attributes so that when we are allocating a leaf, we don't have to set the attribute
    this->ObjectInfo(index) = 0;
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::EnqueueProcessedObject(FreeObject ** list, FreeObject ** tail, void* objectAddress, uint index)
{
    if (*tail == nullptr)
    {
        Assert(*list == nullptr);
        *tail = (FreeObject *)objectAddress;
    }
    EnqueueProcessedObject(list, objectAddress, index);
}

//
// This method transfers the list of objects starting at list and ending
// at tail to the free list.
// In debug mode, it also makes sure that none of the objects that are
// being prepended to the free list are already free
//
template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::TransferProcessedObjects(FreeObject * list, FreeObject * tail)
{
    Assert(tail != nullptr);
    Assert(list);
#if DBG || defined(RECYCLER_STATS)
    // make sure that object we are transferred to the free list are not freed yet
    tail->SetNext(nullptr);
    FreeObject * freeObject = list;
    while (freeObject != nullptr)
    {
        Assert(!this->IsInFreeObjectList(freeObject));
        BOOL isSet = this->GetDebugFreeBitVector()->TestAndSet(GetAddressBitIndex(freeObject));
        Assert(!isSet);
        freeObject = freeObject->GetNext();
    }
#endif
    tail->SetNext(this->freeObjectList);
    this->freeObjectList = list;

    RECYCLER_SLOW_CHECK(this->CheckDebugFreeBitVector(true));
}

template <class TBlockAttributes>
uint
SmallHeapBlockT<TBlockAttributes>::GetAndClearLastFreeCount()
{
    uint lastFreeCount = this->lastFreeCount;
    this->lastFreeCount = 0;
    return lastFreeCount;
}

#ifdef RECYCLER_SLOW_CHECK_ENABLED

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::Check(bool expectFull, bool expectPending)
{
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    // If we allocated from this block during the concurrent sweep the free bit vectors would be invalid.
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    if (!this->wasAllocatedFromDuringSweep)
#endif
#endif
    {
        if (this->IsFreeBitsValid())
        {
            CheckFreeBitVector(false);
        }
        else
        {
            CheckDebugFreeBitVector(false);
        }
    }

    Assert(expectPending == HasAnyDisposeObjects());

    // As the blocks are added to the SLIST and used from there during concurrent sweep, the expectFull assertion doesn't hold anymore.
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    if (!CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
#endif
    {
        if (this->isInAllocator || this->isClearedFromAllocator)
        {
            Assert(expectFull && !expectPending);
        }
        else
        {
            Assert(expectFull == (!this->HasFreeObject() && !HasAnyDisposeObjects()));
        }
    }
}


template <class TBlockAttributes>
template <typename TBlockType>
bool
SmallHeapBlockT<TBlockAttributes>::GetFreeObjectListOnAllocatorImpl(FreeObject ** freeObjectList)
{
    // not during collection, the allocator has the current info
    SmallHeapBlockAllocator<TBlockType> * head =
        &((HeapBucketT<TBlockType> *)this->heapBucket)->allocatorHead;
    SmallHeapBlockAllocator<TBlockType> * current = head;
    do
    {
        if (current->GetHeapBlock() == this)
        {
            if (current->IsFreeListAllocMode())
            {
                *freeObjectList = current->freeObjectList;
                return true;
            }
            return false;
        }
        current = current->GetNext();
    }
    while (current != head);
    return false;
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::CheckDebugFreeBitVector(bool isCollecting)
{
    FreeObject * freeObject = this->freeObjectList;

    if (!isCollecting)
    {
        this->GetFreeObjectListOnAllocator(&freeObject);
    }

    uint verifyFreeCount = 0;
    while (freeObject != nullptr)
    {
        uint index = this->GetAddressIndex(freeObject);
        Assert(index != SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit);
        Assert(this->GetDebugFreeBitVector()->Test(GetAddressBitIndex(freeObject)));
        verifyFreeCount++;
        freeObject = freeObject->GetNext();
    }
    Assert(this->GetDebugFreeBitVector()->Count() == verifyFreeCount);
    Assert(verifyFreeCount <= this->lastFreeCount);
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::CheckFreeBitVector(bool isCollecting)
{
    // during collection, the heap block has the current info when we are verifying
    if (!isCollecting)
    {
        FreeObject * freeObjectList;
        this->GetFreeObjectListOnAllocator(&freeObjectList);
        if (freeObjectList != this->freeObjectList)
        {
            // allocator has the current info and if we have already allocated some memory,
            // the free bit vector isn't really correct, so we can't verify it.
            // Just verify the debug free bit vector
            this->CheckDebugFreeBitVector(false);
            return;
        }
    }

    SmallHeapBlockBitVector * free = this->GetFreeBitVector();

    // Shouldn't be any invalid bits set in the free bit vector
    SmallHeapBlockBitVector temp;
    temp.Copy(free);
    temp.And(this->GetInvalidBitVector());
    Assert(temp.IsAllClear());

    uint verifyFreeCount = 0;
    FreeObject * freeObject = this->freeObjectList;
    while (freeObject != nullptr)
    {
        uint bitIndex = GetAddressBitIndex(freeObject);
        Assert(IsValidBitIndex(bitIndex));
        Assert(this->GetDebugFreeBitVector()->Test(bitIndex));
        Assert(free->Test(bitIndex));
        verifyFreeCount++;

        freeObject = freeObject->GetNext();
    }
    Assert(this->GetDebugFreeBitVector()->Count() == verifyFreeCount);
    Assert(this->freeCount == this->GetFreeBitVector()->Count());

    if (this->IsAnyFinalizableBlock())
    {
        auto finalizableBlock = this->AsFinalizableBlock<TBlockAttributes>();

        // Include pending dispose objects
        finalizableBlock->ForEachPendingDisposeObject([&] (uint index) {
            uint bitIndex = ((uint)index) * this->GetObjectBitDelta();
            Assert(IsValidBitIndex(bitIndex));
            Assert(!this->GetDebugFreeBitVector()->Test(bitIndex));
            Assert(free->Test(bitIndex));
            verifyFreeCount++;
        });

        // Include disposed objects
        verifyFreeCount += finalizableBlock->CheckDisposedObjectFreeBitVector();
    }

    Assert(verifyFreeCount == this->freeCount);
    Assert(verifyFreeCount <= this->lastFreeCount);
    Assert(this->IsFreeBitsValid());
}
#endif

template <class TBlockAttributes>
typename SmallHeapBlockT<TBlockAttributes>::SmallHeapBlockBitVector *
SmallHeapBlockT<TBlockAttributes>::EnsureFreeBitVector(bool isCollecting)
{
    if (this->IsFreeBitsValid())
    {
        // the free object list hasn't change, so the free vector should be valid
        RECYCLER_SLOW_CHECK(CheckFreeBitVector(isCollecting));
        return this->GetFreeBitVector();
    }
    return BuildFreeBitVector();
}

template <class TBlockAttributes>
typename SmallHeapBlockT<TBlockAttributes>::SmallHeapBlockBitVector *
SmallHeapBlockT<TBlockAttributes>::BuildFreeBitVector()
{
    SmallHeapBlockBitVector * free = this->GetFreeBitVector();
    this->freeCount = this->BuildFreeBitVector(free);
    this->lastFreeObjectHead = this->freeObjectList;
    return free;
}

template <class TBlockAttributes>
ushort
SmallHeapBlockT<TBlockAttributes>::BuildFreeBitVector(SmallHeapBlockBitVector * free)
{
    free->ClearAll();
    ushort freeCount = 0;
    FreeObject * freeObject = this->freeObjectList;
    while (freeObject != nullptr)
    {
        uint bitIndex = GetAddressBitIndex(freeObject);
        Assert(IsValidBitIndex(bitIndex));
        Assert(this->GetDebugFreeBitVector()->Test(bitIndex));
        free->Set(bitIndex);
        freeCount++;
        freeObject = freeObject->GetNext();
    }
    Assert(this->GetDebugFreeBitVector()->Count() == freeCount);

    if (this->IsAnyFinalizableBlock())
    {
        auto finalizableBlock = this->AsFinalizableBlock<TBlockAttributes>();

        // Include pending dispose objects
        finalizableBlock->ForEachPendingDisposeObject([&] (uint index) {
            uint bitIndex = ((uint)index) * this->GetObjectBitDelta();
            Assert(IsValidBitIndex(bitIndex));
            Assert(!this->GetDebugFreeBitVector()->Test(bitIndex));
            free->Set(bitIndex);
            freeCount++;
        });

        // Include disposed objects
        freeCount += finalizableBlock->AddDisposedObjectFreeBitVector(free);
    }

    Assert(freeCount <= this->lastFreeCount);
    return freeCount;
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::MarkImplicitRoots()
{
    uint localObjectCount = this->GetObjectCount();
    uint localObjectBitDelta = this->GetObjectBitDelta();
    uint localMarkCount = 0;
    SmallHeapBlockBitVector * mark = this->GetMarkedBitVector();

#if DBG
    uint localObjectSize = this->GetObjectSize();
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
    Assert(localObjectSize <= HeapConstants::MaxSmallObjectSize);
#else
    Assert(localObjectSize <= HeapConstants::MaxMediumObjectSize);
#endif
    ushort markCountPerPage[TBlockAttributes::PageCount];
    for (uint i = 0; i < TBlockAttributes::PageCount; i++)
    {
        markCountPerPage[i] = 0;
    }
#endif

    for (uint i = 0; i < localObjectCount; i++)
    {
        // REVIEW: This may include free object.  It is okay to mark them and scan them
        // But kind inefficient.
        if (this->ObjectInfo(i) & ImplicitRootBit)
        {
#if DBG
            {
                int index = (i * localObjectSize) / AutoSystemInfo::PageSize;
                Assert(index < TBlockAttributes::PageCount);
                markCountPerPage[index]++;
            }
#endif

            mark->Set(localObjectBitDelta * i);
            localMarkCount++;
        }
    }
    Assert(mark->Count() == localMarkCount);
    this->markCount = (ushort)localMarkCount;
#if DBG
    HeapBlockMap& map = this->GetRecycler()->heapBlockMap;

    for (uint i = 0; i < TBlockAttributes::PageCount; i++)
    {
        map.SetPageMarkCount(this->address + (i * AutoSystemInfo::PageSize), markCountPerPage[i]);
    }
#endif
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size))
{
    ForEachAllocatedObject(infoBits, [=](uint index, void * objectAddress)
    {
        CallBackFunction(objectAddress, this->objectSize);
    });
}

template <class TBlockAttributes>
inline
void SmallHeapBlockT<TBlockAttributes>::FillFreeMemory(__in_bcount(size) void * address, size_t size)
{
#ifdef RECYCLER_MEMORY_VERIFY
    if (this->heapBucket->heapInfo->recycler->VerifyEnabled())
    {
        memset(address, Recycler::VerifyMemFill, size);
        return;
    }
#endif

    if (this->IsLeafBlock()
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_THREAD_PAGE
        || this->IsWithBarrier()
#endif
        )
    {
        return;
    }

    // REVIEW: Do DbgMemFill on debug build?
#if defined(_M_IX86)
    uint qwordCount = size / sizeof(uint64) ;
    switch (qwordCount)
    {
    case 2:
        ((uint64*)address)[0] = 0;
        ((uint64*)address)[1] = 0;
        break;
    case 4:
        ((uint64*)address)[0] = 0;
        ((uint64*)address)[1] = 0;
        ((uint64*)address)[2] = 0;
        ((uint64*)address)[3] = 0;
        break;
    case 6:
        ((uint64*)address)[0] = 0;
        ((uint64*)address)[1] = 0;
        ((uint64*)address)[2] = 0;
        ((uint64*)address)[3] = 0;
        ((uint64*)address)[4] = 0;
        ((uint64*)address)[5] = 0;
        break;
    default:
        memset(address, 0, size);
    }
#else
    memset(address, 0, size);
#endif
}


#ifdef RECYCLER_MEMORY_VERIFY
template <class TBlockAttributes>
void SmallHeapBlockT<TBlockAttributes>::VerifyBumpAllocated(_In_ char * bumpAllocateAddress)
{
    ushort verifyFinalizeCount = 0;
    Recycler * recycler = this->heapBucket->heapInfo->recycler;
    char * memBlock = this->GetAddress();
    for (uint i = 0; i < objectCount; i++)
    {
        if (memBlock >= bumpAllocateAddress)
        {
            Recycler::VerifyCheckFill(memBlock + sizeof(FreeObject), this->GetObjectSize() - sizeof(FreeObject));
        }
        else
        {
            recycler->VerifyCheckPad(memBlock, this->GetObjectSize());
            if ((this->ObjectInfo(i) & FinalizeBit) != 0)
            {
                if (this->IsAnyFinalizableBlock())
                {
                    verifyFinalizeCount++;
                }
                else
                {
                    Recycler::VerifyCheck(false, _u("Non-Finalizable block should not have finalizable objects"),
                        this->GetAddress(), &this->ObjectInfo(i));
                }
            }
        }
        memBlock += this->GetObjectSize();
    }
}
template <class TBlockAttributes>
void SmallHeapBlockT<TBlockAttributes>::Verify(bool pendingDispose)
{
    ushort verifyFinalizeCount = 0;
    SmallHeapBlockBitVector tempFree;
    SmallHeapBlockBitVector *free = &tempFree;
    SmallHeapBlockBitVector tempPending;
    this->BuildFreeBitVector(free);
    Recycler * recycler = this->heapBucket->heapInfo->recycler;
    char * memBlock = this->GetAddress();
    uint objectBitDelta = this->GetObjectBitDelta();
    Recycler::VerifyCheck(!pendingDispose || this->IsAnyFinalizableBlock(),
        _u("Non-finalizable block shouldn't be disposing. May have corrupted block type."),
        this->GetAddress(), (void *)&this->heapBlockType);

    if (HasPendingDisposeObjects())
    {
        Assert(pendingDispose);

        // Pending object are not free yet, they don't have memory cleared.
        this->AsFinalizableBlock<TBlockAttributes>()->ForEachPendingDisposeObject([&](uint index) {

            uint bitIndex = ((uint)index) * this->GetObjectBitDelta();
            Assert(IsValidBitIndex(bitIndex));
            Assert(!this->GetDebugFreeBitVector()->Test(bitIndex));
            Assert(free->Test(bitIndex));

            tempPending.Set(bitIndex);

            // We are a pending dispose block, so the finalize count hasn't been update yet.
            // Including the pending objects in the finalize count
            verifyFinalizeCount++;
        });
    }

    for (uint i = 0; i < objectCount; i++)
    {
        if (free->Test(i * objectBitDelta))
        {
            if (!tempPending.Test(i * objectBitDelta))
            {
                char * nextFree = (char *)((FreeObject *)memBlock)->GetNext();
                Recycler::VerifyCheck(nextFree == nullptr
                    || (nextFree >= address && nextFree < this->GetEndAddress()
                    && free->Test(GetAddressBitIndex(nextFree))),
                    _u("SmallHeapBlock memory written to after freed"), memBlock, memBlock);
                Recycler::VerifyCheckFill(memBlock + sizeof(FreeObject), this->GetObjectSize() - sizeof(FreeObject));
            }
        }
        else
        {
            if (explicitFreeBits.Test(i * objectBitDelta))
            {
                char * nextFree = (char *)((FreeObject *)memBlock)->GetNext();

                HeapBlock* nextFreeHeapBlock = this;

                if (nextFree != nullptr)
                {
                    nextFreeHeapBlock = recycler->FindHeapBlock(nextFree);
                }

                Recycler::VerifyCheck(nextFree == nullptr
                    || (nextFree >= address && nextFree < this->GetEndAddress()
                    && explicitFreeBits.Test(GetAddressBitIndex(nextFree)))
                    || nextFreeHeapBlock->GetObjectSize(nextFree) == this->objectSize,
                    _u("SmallHeapBlock memory written to after freed"), memBlock, memBlock);
                recycler->VerifyCheckPadExplicitFreeList(memBlock, this->GetObjectSize());
            }
            else
            {
                recycler->VerifyCheckPad(memBlock, this->GetObjectSize());
            }

            if ((this->ObjectInfo(i) & FinalizeBit) != 0)
            {
                if (this->IsAnyFinalizableBlock())
                {
                    verifyFinalizeCount++;
                }
                else
                {
                    Recycler::VerifyCheck(false, _u("Non-Finalizable block should not have finalizable objects"),
                        this->GetAddress(), &this->ObjectInfo(i));
                }
            }

        }
        memBlock += this->GetObjectSize();
    }

    if (this->IsAnyFinalizableBlock())
    {
        Recycler::VerifyCheck(this->AsFinalizableBlock<TBlockAttributes>()->finalizeCount == verifyFinalizeCount,
            _u("SmallHeapBlock finalize count mismatch"), this->GetAddress(), &this->AsFinalizableBlock<TBlockAttributes>()->finalizeCount);
    }
    else
    {
        Assert(verifyFinalizeCount == 0);
    }
}
#endif

#if ENABLE_MEM_STATS
template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::AggregateBlockStats(HeapBucketStats& stats, bool isAllocatorBlock, FreeObject* freeObjectList, bool isBumpAllocated)
{
    if (this->segment == nullptr || this->IsInAllocator() != isAllocatorBlock)
    {
        return;  // skip empty blocks, or blocks mismatching isInAllocator to avoid double count
    }

    DUMP_FRAGMENTATION_STATS_ONLY(stats.totalBlockCount++);

    ushort blockObjectCount = this->objectCount;
    BVIndex blockFreeCount = this->GetFreeBitVector()->Count();
    ushort blockObjectSize = this->objectSize;

    uint objectCount = 0;
    if (isBumpAllocated)
    {
        objectCount = static_cast<uint>(((char*) freeObjectList - this->address) / blockObjectSize);
    }
    else
    {
        objectCount = blockObjectCount;

        // If this is an allocator block, remove the free objects on the allocator
        // from this count. Otherwise, remove the free objects found in the free bit vector
        if (freeObjectList)
        {
            Assert(isAllocatorBlock);
            FreeObject* next = freeObjectList->GetNext();
            while (next != nullptr && next != freeObjectList)
            {
                objectCount--;
                next = next->GetNext();
            }
        }
        else
        {
            objectCount -= blockFreeCount;
        }
    }

    DUMP_FRAGMENTATION_STATS_ONLY(stats.objectCount += objectCount);
    stats.objectByteCount += (objectCount * blockObjectSize);
    stats.totalByteCount += this->GetPageCount() * AutoSystemInfo::PageSize;

#ifdef DUMP_FRAGMENTATION_STATS
    if (!isAllocatorBlock)
    {
        if (this->IsAnyFinalizableBlock())
        {
            auto finalizableBlock = this->AsFinalizableBlock<TBlockAttributes>();
            stats.finalizeCount += (finalizableBlock->GetFinalizeCount());
        }
    }
#endif
}
#endif

#ifdef RECYCLER_PERF_COUNTERS
template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::UpdatePerfCountersOnFree()
{
    Assert(markCount == 0);
    Assert(this->IsFreeBitsValid());

    size_t usedCount = (objectCount - freeCount);
    size_t usedBytes = usedCount * objectSize;

    RECYCLER_PERF_COUNTER_SUB(SmallHeapBlockLiveObject, usedCount);
    RECYCLER_PERF_COUNTER_SUB(SmallHeapBlockLiveObjectSize, usedBytes);
    RECYCLER_PERF_COUNTER_SUB(SmallHeapBlockFreeObjectSize, this->GetPageCount() * AutoSystemInfo::PageSize - usedBytes);

    RECYCLER_PERF_COUNTER_SUB(LiveObject, usedCount);
    RECYCLER_PERF_COUNTER_SUB(LiveObjectSize, usedBytes);
    RECYCLER_PERF_COUNTER_SUB(FreeObjectSize, this->GetPageCount() * AutoSystemInfo::PageSize - usedBytes);
}
#endif
#ifdef PROFILE_RECYCLER_ALLOC
template <class TBlockAttributes>
void *
SmallHeapBlockT<TBlockAttributes>::GetTrackerData(void * address)
{
    Assert(Recycler::DoProfileAllocTracker());
    ushort index = this->GetAddressIndex(address);
    Assert(index != SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit);
    return this->GetTrackerDataArray()[index];
}

template <class TBlockAttributes>
void
SmallHeapBlockT<TBlockAttributes>::SetTrackerData(void * address, void * data)
{
    Assert(Recycler::DoProfileAllocTracker());
    ushort index = this->GetAddressIndex(address);
    Assert(index != SmallHeapBlockT<TBlockAttributes>::InvalidAddressBit);

    void* existingTrackerData = this->GetTrackerDataArray()[index];
    Assert((existingTrackerData == nullptr || data == nullptr) ||
        (existingTrackerData == &Recycler::TrackerData::ExplicitFreeListObjectData || data == &Recycler::TrackerData::ExplicitFreeListObjectData));
    this->GetTrackerDataArray()[index] = data;
}

template <class TBlockAttributes>
void **
SmallHeapBlockT<TBlockAttributes>::GetTrackerDataArray()
{
    // See SmallHeapBlockT<TBlockAttributes>::GetAllocPlusSize for layout description
    return (void **)((char *)this - SmallHeapBlockT<TBlockAttributes>::GetAllocPlusSize(this->objectCount));
}
#endif

#ifdef RECYCLER_WRITE_BARRIER
template <class TBlockAttributes>
bool
SmallHeapBlockT<TBlockAttributes>::IsWithBarrier() const
{
    return IsNormalWriteBarrierBlock() || IsFinalizableWriteBarrierBlock();
}
#endif

namespace Memory
{
// Instantiate the template
template class SmallHeapBlockT<SmallAllocationBlockAttributes>;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
template class SmallHeapBlockT<MediumAllocationBlockAttributes>;
#endif
};

#define TBlockTypeAttributes SmallAllocationBlockAttributes
#include "SmallBlockDeclarations.inl"
#undef TBlockTypeAttributes

#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
#define TBlockTypeAttributes MediumAllocationBlockAttributes
#include "SmallBlockDeclarations.inl"
#undef TBlockTypeAttributes
#endif
