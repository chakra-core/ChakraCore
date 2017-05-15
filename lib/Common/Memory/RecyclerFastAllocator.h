//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once


namespace Memory
{
template <typename T, ObjectInfoBits attributes>
class RecyclerFastAllocator
{
    typedef typename SmallHeapBlockType<(ObjectInfoBits)(attributes & GetBlockTypeBitMask), SmallAllocationBlockAttributes>::BlockType BlockType;
public:
#ifdef TRACK_ALLOC
    RecyclerFastAllocator * TrackAllocInfo(TrackAllocData const& data)
    {
#ifdef PROFILE_RECYCLER_ALLOC
        recycler->TrackAllocInfo(data);
#endif
        return this;
    }
#endif

    void Initialize(Recycler * recycler)
    {
        this->recycler = recycler;

        size_t sizeCat = GetAlignedAllocSize();
        recycler->AddSmallAllocator(&allocator, sizeCat);
    }
    void Uninitialize()
    {
        size_t sizeCat = GetAlignedAllocSize();
        this->recycler->RemoveSmallAllocator(&allocator, sizeCat);
        this->recycler = nullptr;
    }

    Recycler * GetRecycler() { return recycler; }
    char * Alloc(DECLSPEC_GUARD_OVERFLOW size_t size)
    {
        Assert(recycler != nullptr);
        Assert(!recycler->IsHeapEnumInProgress() || recycler->AllowAllocationDuringHeapEnum());
        Assert(size == sizeof(T));

#ifdef PROFILE_RECYCLER_ALLOC
        TrackAllocData trackAllocData;
        recycler->ClearTrackAllocInfo(&trackAllocData);
#endif
        size_t sizeCat = GetAlignedAllocSize();
        Assert(HeapInfo::IsSmallObject(sizeCat));

        // TODO: SWB, currently RecyclerFastAllocator only used for number allocating, which is Leaf
        // need to add WithBarrierBit if we have other usage with NonLeaf
        char * memBlock = allocator.template InlinedAlloc<(ObjectInfoBits)(attributes & InternalObjectInfoBitMask)>(recycler, sizeCat);

        if (memBlock == nullptr)
        {
            memBlock = recycler->SmallAllocatorAlloc<attributes>(&allocator, sizeCat, size);
            Assert(memBlock != nullptr);
        }

#ifdef PROFILE_RECYCLER_ALLOC
        recycler->TrackAlloc(memBlock, sizeof(T), trackAllocData);
#endif
        RecyclerMemoryTracking::ReportAllocation(this->recycler, memBlock, sizeof(T));
        RECYCLER_PERF_COUNTER_INC(LiveObject);
        RECYCLER_PERF_COUNTER_ADD(LiveObjectSize, sizeCat);
        RECYCLER_PERF_COUNTER_SUB(FreeObjectSize, sizeCat);

        RECYCLER_PERF_COUNTER_INC(SmallHeapBlockLiveObject);
        RECYCLER_PERF_COUNTER_ADD(SmallHeapBlockLiveObjectSize, sizeCat);
        RECYCLER_PERF_COUNTER_SUB(SmallHeapBlockFreeObjectSize, sizeCat);

#ifdef RECYCLER_MEMORY_VERIFY
        recycler->FillCheckPad(memBlock, sizeof(T), sizeCat);
#endif
#ifdef RECYCLER_PAGE_HEAP
        recycler->VerifyPageHeapFillAfterAlloc(memBlock, size, attributes);
#endif
        return memBlock;
    };
    static uint32 GetEndAddressOffset()
    {
        return offsetof(RecyclerFastAllocator, allocator) + SmallHeapBlockAllocator<BlockType>::GetEndAddressOffset();
    }

    bool AllowNativeCodeBumpAllocation()
    {
        return recycler->AllowNativeCodeBumpAllocation();
    }

    char *GetEndAddress()
    {
        return allocator.GetEndAddress();
    }
    static uint32 GetFreeObjectListOffset()
    {
        return offsetof(RecyclerFastAllocator, allocator) + SmallHeapBlockAllocator<BlockType>::GetFreeObjectListOffset();
    }
    FreeObject *GetFreeObjectList()
    {
        return allocator.GetFreeObjectList();
    }
    void SetFreeObjectList(FreeObject *freeObject)
    {
        allocator.SetFreeObjectList(freeObject);
    }

#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY) || defined(MEMSPECT_TRACKING) || defined(ETW_MEMORY_TRACKING)
    RecyclerFastAllocator()
    {
        allocator.SetTrackNativeAllocatedObjectCallBack(&TrackNativeAllocatedObject);
    }

    static void TrackNativeAllocatedObject(Recycler * recycler, void * memBlock, size_t sizeCat)
    {
#ifdef PROFILE_RECYCLER_ALLOC
        TrackAllocData trackAllocData = { &typeid(T), 0, (size_t)-1, NULL, 0 };
        recycler->TrackAlloc(memBlock, sizeof(T), trackAllocData);
#endif
        RecyclerMemoryTracking::ReportAllocation(recycler, memBlock, sizeof(T));
        RECYCLER_PERF_COUNTER_INC(LiveObject);
        RECYCLER_PERF_COUNTER_ADD(LiveObjectSize, sizeCat);
        RECYCLER_PERF_COUNTER_SUB(FreeObjectSize, sizeCat);

        RECYCLER_PERF_COUNTER_INC(SmallHeapBlockLiveObject);
        RECYCLER_PERF_COUNTER_ADD(SmallHeapBlockLiveObjectSize, sizeCat);
        RECYCLER_PERF_COUNTER_SUB(SmallHeapBlockFreeObjectSize, sizeCat);

#ifdef RECYCLER_MEMORY_VERIFY
        recycler->FillCheckPad(memBlock, sizeof(T), sizeCat, true);
#endif
    }
#endif

    size_t GetAlignedAllocSize() const
    {
#ifdef RECYCLER_MEMORY_VERIFY
        return GetAlignedAllocSize(recycler->VerifyEnabled(), recycler->verifyPad);
#else
        return GetAlignedAllocSize(FALSE, 0);
#endif
    }

    static size_t GetAlignedAllocSize(BOOL verifyEnabled, uint verifyPad)
    {
#ifdef RECYCLER_MEMORY_VERIFY
        if (verifyEnabled)
        {
            CompileAssert(sizeof(T) <= (size_t)-1 - sizeof(size_t));
            return HeapInfo::GetAlignedSize(AllocSizeMath::Add(sizeof(T) + sizeof(size_t), verifyPad));
        }
#endif
        // We should have structures large enough that would cause this to overflow
        CompileAssert(((sizeof(T) + (HeapConstants::ObjectGranularity - 1)) & ~(HeapConstants::ObjectGranularity - 1)) != 0);
        return HeapInfo::GetAlignedSizeNoCheck(sizeof(T));
    }

private:
    SmallHeapBlockAllocator<BlockType> allocator;
    Recycler * recycler;

    CompileAssert(sizeof(T) <= HeapConstants::MaxSmallObjectSize);
};
}

