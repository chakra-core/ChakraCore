//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
namespace Memory
{
template <typename TBlockType>
class SmallHeapBlockAllocator
{
public:
    typedef TBlockType BlockType;

    SmallHeapBlockAllocator();
    void Initialize();

    template <ObjectInfoBits attributes>
    inline char * InlinedAlloc(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat);

    // Pass through template parameter to InlinedAllocImpl
    template <bool canFaultInject>
    inline char * SlowAlloc(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, ObjectInfoBits attributes);

    // There are paths where we simply can't OOM here, so we shouldn't fault inject as it creates a bit of a mess
    template <bool canFaultInject>
    inline char* InlinedAllocImpl(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, ObjectInfoBits attributes);

    TBlockType * GetHeapBlock() const { return heapBlock; }
    SmallHeapBlockAllocator * GetNext() const { return next; }

    void Set(TBlockType * heapBlock);
    void SetNew(TBlockType * heapBlock);
    void Clear();
    void UpdateHeapBlock();
    void SetExplicitFreeList(FreeObject* list);

    static uint32 GetEndAddressOffset() { return offsetof(SmallHeapBlockAllocator, endAddress); }
    char *GetEndAddress() { return endAddress; }
    static uint32 GetFreeObjectListOffset() { return offsetof(SmallHeapBlockAllocator, freeObjectList); }
    FreeObject *GetFreeObjectList() { return freeObjectList; }
    void SetFreeObjectList(FreeObject *freeObject) { freeObjectList = freeObject; }

#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY) || defined(MEMSPECT_TRACKING) || defined(ETW_MEMORY_TRACKING)
    void SetTrackNativeAllocatedObjectCallBack(void (*pfnCallBack)(Recycler *, void *, size_t))
    {
        pfnTrackNativeAllocatedObjectCallBack = pfnCallBack;
    }
#endif
#if DBG
    FreeObject * GetExplicitFreeList() const
    {
        Assert(IsExplicitFreeObjectListAllocMode());
        return this->freeObjectList;
    }
#endif

    bool IsBumpAllocMode() const
    {
        return endAddress != nullptr;
    }
    bool IsExplicitFreeObjectListAllocMode() const
    {
        return this->heapBlock == nullptr;
    }
    bool IsFreeListAllocMode() const
    {
        return !IsBumpAllocMode() && !IsExplicitFreeObjectListAllocMode();
    }
private:
    static bool NeedSetAttributes(ObjectInfoBits attributes)
    {
        return attributes != LeafBit && (attributes & InternalObjectInfoBitMask) != 0;
    }

    char * endAddress;
    FreeObject * freeObjectList;
    TBlockType * heapBlock;

    SmallHeapBlockAllocator * prev;
    SmallHeapBlockAllocator * next;

    friend class HeapBucketT<BlockType>;
#ifdef RECYCLER_SLOW_CHECK_ENABLED
    template <class TBlockAttributes>
    friend class SmallHeapBlockT;
#endif
#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY)
    HeapBucket * bucket;
#endif

#ifdef RECYCLER_TRACK_NATIVE_ALLOCATED_OBJECTS
    char * lastNonNativeBumpAllocatedBlock;
    void TrackNativeAllocatedObjects();
#endif
#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY) || defined(MEMSPECT_TRACKING) || defined(ETW_MEMORY_TRACKING)
    void (*pfnTrackNativeAllocatedObjectCallBack)(Recycler * recycler, void *, size_t sizeCat);
#endif
};

template <typename TBlockType>
template <bool canFaultInject>
inline char*
SmallHeapBlockAllocator<TBlockType>::InlinedAllocImpl(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, ObjectInfoBits attributes)
{
    Assert((attributes & InternalObjectInfoBitMask) == attributes);
#ifdef RECYCLER_WRITE_BARRIER
    Assert(!CONFIG_FLAG(ForceSoftwareWriteBarrier) || (attributes & WithBarrierBit) || (attributes & LeafBit));
#endif

    AUTO_NO_EXCEPTION_REGION;
    if (canFaultInject)
    {
        FAULTINJECT_MEMORY_NOTHROW(_u("InlinedAllocImpl"), sizeCat);
    }

    char * memBlock = (char *)freeObjectList;
    char * nextCurrentAddress = memBlock + sizeCat;
    char * endAddress = this->endAddress;

    if (nextCurrentAddress <= endAddress)
    {
        // Bump Allocation
        Assert(this->IsBumpAllocMode());
#ifdef RECYCLER_TRACK_NATIVE_ALLOCATED_OBJECTS
        TrackNativeAllocatedObjects();
        lastNonNativeBumpAllocatedBlock = memBlock;
#endif
        freeObjectList = (FreeObject *)nextCurrentAddress;

        if (NeedSetAttributes(attributes))
        {
            heapBlock->SetAttributes(memBlock, (attributes & StoredObjectInfoBitMask));
        }

        return memBlock;
    }

    if (memBlock != nullptr && endAddress == nullptr)
    {
        // Free list allocation
        Assert(!this->IsBumpAllocMode());
        if (NeedSetAttributes(attributes))
        {
            TBlockType * allocationHeapBlock = this->heapBlock;
            if (allocationHeapBlock == nullptr)
            {
                Assert(this->IsExplicitFreeObjectListAllocMode());
                allocationHeapBlock = (TBlockType *)recycler->FindHeapBlock(memBlock);
                Assert(allocationHeapBlock != nullptr);
                Assert(!allocationHeapBlock->IsLargeHeapBlock());
            }
            allocationHeapBlock->SetAttributes(memBlock, (attributes & StoredObjectInfoBitMask));
        }
        freeObjectList = ((FreeObject *)memBlock)->GetNext();

#ifdef RECYCLER_MEMORY_VERIFY
        ((FreeObject *)memBlock)->DebugFillNext();

        if (this->IsExplicitFreeObjectListAllocMode())
        {
            HeapBlock* heapBlock = recycler->FindHeapBlock(memBlock);
            Assert(heapBlock != nullptr);
            Assert(!heapBlock->IsLargeHeapBlock());
            TBlockType* smallBlock = (TBlockType*)heapBlock;
            smallBlock->ClearExplicitFreeBitForObject(memBlock);
        }
#endif

#if DBG || defined(RECYCLER_STATS)
        if (!IsExplicitFreeObjectListAllocMode())
        {
            BOOL isSet = heapBlock->GetDebugFreeBitVector()->TestAndClear(heapBlock->GetAddressBitIndex(memBlock));
            Assert(isSet);
        }
#endif
        return memBlock;
    }

    return nullptr;
}


template <typename TBlockType>
template <ObjectInfoBits attributes>
inline char *
SmallHeapBlockAllocator<TBlockType>::InlinedAlloc(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat)
{
    return InlinedAllocImpl<true /* allow fault injection */>(recycler, sizeCat, attributes);
}

template <typename TBlockType>
template <bool canFaultInject>
inline
char *
SmallHeapBlockAllocator<TBlockType>::SlowAlloc(Recycler * recycler, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, ObjectInfoBits attributes)
{
    Assert((attributes & InternalObjectInfoBitMask) == attributes);

    return InlinedAllocImpl<canFaultInject>(recycler, sizeCat, attributes);
}
}
