//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "HeapBucketStats.h"

class ScriptMemoryDumper;

#include "HeapBucketStats.h"

namespace Memory
{
#ifdef RECYCLER_PAGE_HEAP
enum class PageHeapBlockTypeFilter;

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
#define PageHeapVerboseTrace(flags, ...) \
if (flags.Verbose && flags.Trace.IsEnabled(Js::PageHeapPhase)) \
    { \
        Output::Print(__VA_ARGS__); \
    }
#define PageHeapTrace(flags, ...) \
if (flags.Trace.IsEnabled(Js::PageHeapPhase)) \
    { \
        Output::Print(__VA_ARGS__); \
    }
#else
#define PageHeapVerboseTrace(...)
#define PageHeapTrace(...)
#endif

#else
#define PageHeapVerboseTrace(...)
#define PageHeapTrace(...)
#endif


class Recycler;
class HeapBucket;
class HeapInfo;
template <typename TBlockType> class HeapBucketT;
class  RecyclerSweep;
class MarkContext;

#if ENABLE_MEM_STATS

#ifdef DUMP_FRAGMENTATION_STATS
#define DUMP_FRAGMENTATION_STATS_ONLY(x) x
#define DUMP_FRAGMENTATION_STATS_IS(x) x
#else
#define DUMP_FRAGMENTATION_STATS_ONLY(x)
#define DUMP_FRAGMENTATION_STATS_IS(x) false
#endif
#endif  // ENABLE_MEM_STATS

#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY) || defined(MEMSPECT_TRACKING) || defined(RECYCLER_PERF_COUNTERS) || defined(ETW_MEMORY_TRACKING)
#define RECYCLER_TRACK_NATIVE_ALLOCATED_OBJECTS
#endif

#ifdef RECYCLER_SLOW_CHECK_ENABLED
#define RECYCLER_SLOW_CHECK(x) x
#define RECYCLER_SLOW_CHECK_IF(cond, x) if (cond) { x; }
#else
#define RECYCLER_SLOW_CHECK(x)
#define RECYCLER_SLOW_CHECK_IF(cond, x)
#endif

// ObjectInfoBits is unsigned short, but only the lower byte is stored as the object attribute
// The upper bits are used to pass other information about allocation (e.g. NoDisposeBit)
//
enum ObjectInfoBits : unsigned short
{
    // Bits that are actually stored in ObjectInfo

    NoBit                       = 0x00,    // assume an allocation is not leaf unless LeafBit is specified.
    FinalizeBit                 = 0x80,    // Indicates that the object has a finalizer
    PendingDisposeBit           = 0x40,    // Indicates that the object is pending dispose
    LeafBit                     = 0x20,    // Indicates that the object is a leaf-object (objects without this bit need to be scanned)
    TrackBit                    = 0x10,    // Indicates that the object is a TrackableObject, but has also been overloaded to mean traced for RecyclerVisitedHostHeap objects
    ImplicitRootBit             = 0x08,
    NewTrackBit                 = 0x04,    // Tracked object is newly allocated and hasn't been process by concurrent GC
    MemoryProfilerOldObjectBit  = 0x02,
    EnumClass_1_Bit             = 0x01,    // This can be extended to add more enumerable classes (if we still have bits left)

    // Mask for above bits
    StoredObjectInfoBitMask     = 0xFF,

    // Bits that implied by the block type, and thus don't need to be stored (for small blocks)
    // Note, LeafBit is used in finalizable blocks, thus is not always implied by the block type
    // GC-TODO: FinalizeBit doesn't need to be stored since we have separate bucket for them.
    // We can move it the upper byte.

#ifdef RECYCLER_WRITE_BARRIER
    WithBarrierBit              = 0x0100,
#endif

#ifdef RECYCLER_VISITED_HOST
    RecyclerVisitedHostBit      = 0x0200,
#endif

    // Mask for above bits
    InternalObjectInfoBitMask   = 0x03FF,

    // Bits that only affect allocation behavior, not mark/sweep/etc

    ClientTrackedBit            = 0x0400,       // This allocation is client tracked
    TraceBit                    = 0x0800,

    // Additional definitions based on above

#ifdef RECYCLER_STATS
    NewFinalizeBit              = NewTrackBit,  // Use to detect if the background thread has counted the finalizable object in stats
#else
    NewFinalizeBit              = 0x00,
#endif

#ifdef RECYCLER_WRITE_BARRIER
    FinalizableWithBarrierBit   = WithBarrierBit | FinalizeBit,
#endif

    // Allocation bits
    FinalizableLeafBits         = NewFinalizeBit | FinalizeBit | LeafBit,
    FinalizableObjectBits       = NewFinalizeBit | FinalizeBit,
#ifdef RECYCLER_WRITE_BARRIER
    FinalizableWithBarrierObjectBits = NewFinalizeBit | FinalizableWithBarrierBit,
#endif
    ClientFinalizableObjectBits = NewFinalizeBit | ClientTrackedBit | FinalizeBit,

    ClientTrackableLeafBits     = NewTrackBit | ClientTrackedBit | TrackBit | FinalizeBit | LeafBit,
    ClientTrackableObjectBits   = NewTrackBit | ClientTrackedBit | TrackBit | FinalizeBit,

#ifdef RECYCLER_WRITE_BARRIER
    ClientTrackableObjectWithBarrierBits = ClientTrackableObjectBits | WithBarrierBit,
    ClientFinalizableObjectWithBarrierBits = ClientFinalizableObjectBits | WithBarrierBit,
#endif

    WeakReferenceEntryBits      = LeafBit,

    ImplicitRootLeafBits        = LeafBit | ImplicitRootBit,

    // Pending dispose objects should have LeafBit set and no others
    PendingDisposeObjectBits    = PendingDisposeBit | LeafBit,

#ifdef RECYCLER_VISITED_HOST
    // Bits for use with recycler visited host heap block.
    // Recycler visited host heap block will both mark and finalize based on IRecyclerVisitedHost v-table, as specified
    // by TrackBit and FinalizeBit. These objects are expected to be allocated by chakra, but implemented by
    // the host, including construction of the IRecyclerVisitedHost v-table.
    //
    // RecyclerVisitedHostBit is implicit in the heap block type and thus isn't part of the StoredObjectInfoBitMask.
    // LeafBit is also set for any object that is not precisely traced.
    RecyclerVisitedHostTracedBits = RecyclerVisitedHostBit | TrackBit | NewTrackBit,
    RecyclerVisitedHostFinalizableBits = RecyclerVisitedHostBit | LeafBit | FinalizeBit | NewFinalizeBit,
    RecyclerVisitedHostTracedFinalizableBits = RecyclerVisitedHostTracedBits | FinalizeBit,

    // These set of bits describe the four possible types of blocktype bits for recycler visited host heap blocks.
    // These are the four combinations of the above bits, AND'd with GetBlockTypeBitMask.
    // In the end, these are treated the same in terms of which heap block/bucket type they end up using and
    // but are defined here for ease of use.
    RecyclerVisitedHostFinalizableBlockTypeBits = RecyclerVisitedHostBit | LeafBit | FinalizeBit,
    RecyclerVisitedHostTracedFinalizableBlockTypeBits = RecyclerVisitedHostBit | FinalizeBit,
#endif

    GetBlockTypeBitMask = FinalizeBit | LeafBit 
#ifdef RECYCLER_WRITE_BARRIER
    | WithBarrierBit
#endif
#ifdef RECYCLER_VISITED_HOST
    | RecyclerVisitedHostBit
#endif
    ,

    CollectionBitMask           = LeafBit | FinalizeBit | TrackBit | NewTrackBit,  // Bits relevant to collection

    EnumClassMask               = EnumClass_1_Bit,
};


enum ResetMarkFlags
{
    ResetMarkFlags_None = 0x0,
    ResetMarkFlags_Background = 0x1,
    ResetMarkFlags_ScanImplicitRoot = 0x2,


    // For in thread GC
    ResetMarkFlags_InThread = ResetMarkFlags_None,
    ResetMarkFlags_InThreadImplicitRoots = ResetMarkFlags_None | ResetMarkFlags_ScanImplicitRoot,

    // For background GC
    ResetMarkFlags_InBackgroundThread = ResetMarkFlags_Background,
    ResetMarkFlags_InBackgroundThreadImplicitRoots = ResetMarkFlags_Background | ResetMarkFlags_ScanImplicitRoot,

    // For blocking synchronized GC
    ResetMarkFlags_Synchronized = ResetMarkFlags_None,
    ResetMarkFlags_SynchronizedImplicitRoots = ResetMarkFlags_None | ResetMarkFlags_ScanImplicitRoot,

    // For heap enumeration
    ResetMarkFlags_HeapEnumeration = ResetMarkFlags_None,
};

enum RescanFlags
{
    RescanFlags_None             = 0x0,
    RescanFlags_ResetWriteWatch  = 0x1
};

enum FindHeapObjectFlags
{
    FindHeapObjectFlags_NoFlags                     = 0x0,
    FindHeapObjectFlags_ClearedAllocators           = 0x1,   // Assumes that the allocator is already cleared
    FindHeapObjectFlags_VerifyFreeBitForAttribute   = 0x2,   // Don't recompute the free bit vector if there is no pending objects, the attributes will always be correct
    FindHeapObjectFlags_NoFreeBitVerify             = 0x4,   // No checking whether the address is free or not.
    FindHeapObjectFlags_AllowInterior               = 0x8,   // Allow finding heap objects for interior pointers.
};

template <class TBlockAttributes> class SmallNormalHeapBlockT;
template <class TBlockAttributes> class SmallLeafHeapBlockT;
template <class TBlockAttributes> class SmallFinalizableHeapBlockT;

#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
#define INSTANTIATE_MED_BLOCKTYPES(TemplateType)
#define INSTANTIATE_SWB_MED_BLOCKTYPES(TemplateType)
#define INSTANTIATE_RECYCLER_VISITED_MED_BLOCKTYPES(TemplateType)
#else
#define INSTANTIATE_MED_BLOCKTYPES(TemplateType) \
    template class TemplateType<Memory::MediumNormalHeapBlock>; \
    template class TemplateType<Memory::MediumLeafHeapBlock>; \
    template class TemplateType<Memory::MediumFinalizableHeapBlock>;
#define INSTANTIATE_MED_SWB_BLOCKTYPES(TemplateType) \
    template class TemplateType<Memory::MediumNormalWithBarrierHeapBlock>; \
    template class TemplateType<Memory::MediumFinalizableWithBarrierHeapBlock>;
#define INSTANTIATE_RECYCLER_VISITED_MED_BLOCKTYPES(TemplateType) \
    template class TemplateType<Memory::MediumRecyclerVisitedHostHeapBlock>;
#endif

#ifdef RECYCLER_WRITE_BARRIER
template <class TBlockAttributes> class SmallNormalWithBarrierHeapBlockT;
template <class TBlockAttributes> class SmallFinalizableWithBarrierHeapBlockT;

#define INSTANTIATE_SWB_BLOCKTYPES(TemplateType) \
    template class TemplateType<Memory::SmallNormalWithBarrierHeapBlock>; \
    template class TemplateType<Memory::SmallFinalizableWithBarrierHeapBlock>; \
    INSTANTIATE_SWB_MED_BLOCKTYPES(TemplateType)
#else
#define INSTANTIATE_SWB_BLOCKTYPES(TemplateType)
#endif

#ifdef RECYCLER_VISITED_HOST
template <class TBlockAttributes> class SmallRecyclerVisitedHostHeapBlockT;
#define INSTANTIATE_RECYCLER_VISITED_BLOCKTYPES(TemplateType) \
    template class TemplateType<Memory::SmallRecyclerVisitedHostHeapBlock>; \
    INSTANTIATE_RECYCLER_VISITED_MED_BLOCKTYPES(TemplateType) \

#else
#define INSTANTIATE_RECYCLER_VISITED_BLOCKTYPES(TemplateType)
#endif

#define EXPLICIT_INSTANTIATE_WITH_SMALL_HEAP_BLOCK_TYPE(TemplateType) \
    template class TemplateType<Memory::SmallNormalHeapBlock>;        \
    template class TemplateType<Memory::SmallLeafHeapBlock>; \
    template class TemplateType<Memory::SmallFinalizableHeapBlock>; \
    INSTANTIATE_MED_BLOCKTYPES(TemplateType) \
    INSTANTIATE_SWB_BLOCKTYPES(TemplateType) \
    INSTANTIATE_RECYCLER_VISITED_BLOCKTYPES(TemplateType) \

class RecyclerHeapObjectInfo;
class HeapBlock
{
public:
    enum HeapBlockType : byte
    {
        FreeBlockType = 0,                  // Only used in HeapBlockMap.  Actual HeapBlock structures should never have this.
        SmallNormalBlockType,
        SmallLeafBlockType,
        SmallFinalizableBlockType,
#ifdef RECYCLER_WRITE_BARRIER
        SmallNormalBlockWithBarrierType,
        SmallFinalizableBlockWithBarrierType,
#endif
#ifdef RECYCLER_VISITED_HOST
        SmallRecyclerVisitedHostBlockType,
#endif
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
        MediumNormalBlockType,
        MediumLeafBlockType,
        MediumFinalizableBlockType,
#ifdef RECYCLER_WRITE_BARRIER
        MediumNormalBlockWithBarrierType,
        MediumFinalizableBlockWithBarrierType,
#endif
#ifdef RECYCLER_VISITED_HOST
        MediumRecyclerVisitedHostBlockType,
#endif
#endif
        LargeBlockType,

#ifdef RECYCLER_VISITED_HOST
        SmallAllocBlockTypeCount = 7, // Actual number of types for blocks containing small allocations
#else
        SmallAllocBlockTypeCount = 6,
#endif

#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
#ifdef RECYCLER_VISITED_HOST
        MediumAllocBlockTypeCount = 6, // Actual number of types for blocks containing medium allocations
#else
        MediumAllocBlockTypeCount = 5,
#endif
#endif

        SmallBlockTypeCount = SmallAllocBlockTypeCount
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
        + MediumAllocBlockTypeCount
#endif
        ,      // Distinct block types independent of allocation size using SmallHeapBlockT
        LargeBlockTypeCount = 1, // There is only one LargeBlockType

        BlockTypeCount = SmallBlockTypeCount + LargeBlockTypeCount,
    };
    bool IsNormalBlock() const {
        return this->GetHeapBlockType() == SmallNormalBlockType
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
            || this->GetHeapBlockType() == MediumNormalBlockType
#endif
            ;
    }
    bool IsLeafBlock() const {
        return this->GetHeapBlockType() == SmallLeafBlockType
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
            || this->GetHeapBlockType() == MediumLeafBlockType
#endif
            ;
    }
    bool IsFinalizableBlock() const 
    {
        return this->GetHeapBlockType() == SmallFinalizableBlockType 
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
            || this->GetHeapBlockType() == MediumFinalizableBlockType
#endif
#ifdef RECYCLER_VISITED_HOST
            || IsRecyclerVisitedHostBlock()
#endif
            ;
    }
#ifdef RECYCLER_VISITED_HOST
    bool IsRecyclerVisitedHostBlock() const {
        return this->GetHeapBlockType() == SmallRecyclerVisitedHostBlockType
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
            || this->GetHeapBlockType() == MediumRecyclerVisitedHostBlockType
#endif
            ;
    }
#endif

#ifdef RECYCLER_WRITE_BARRIER
    bool IsAnyNormalBlock() const { return IsNormalBlock() || IsNormalWriteBarrierBlock(); }
    bool IsAnyFinalizableBlock() const { return IsFinalizableBlock() || IsFinalizableWriteBarrierBlock(); }
    bool IsNormalWriteBarrierBlock() const {
        return this->GetHeapBlockType() == SmallNormalBlockWithBarrierType
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
            || this->GetHeapBlockType() == MediumNormalBlockWithBarrierType
#endif
            ;
    }
    bool IsFinalizableWriteBarrierBlock() const { return this->GetHeapBlockType() == SmallFinalizableBlockWithBarrierType
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
        || this->GetHeapBlockType() == MediumFinalizableBlockWithBarrierType
#endif
    ;
    }
#else
    bool IsAnyFinalizableBlock() const { return IsFinalizableBlock(); }
    bool IsAnyNormalBlock() const { return IsNormalBlock(); }
#endif

    bool IsLargeHeapBlock() const { return this->GetHeapBlockType() == LargeBlockType; }
    char * GetAddress() const { return address; }
    Segment * GetSegment() const { return segment; }

    template <typename TBlockAttributes>
    SmallNormalHeapBlockT<TBlockAttributes> * AsNormalBlock();

    template <typename TBlockAttributes>
    SmallLeafHeapBlockT<TBlockAttributes> * AsLeafBlock();

    template <typename TBlockAttributes>
    SmallFinalizableHeapBlockT<TBlockAttributes> * AsFinalizableBlock();

#ifdef RECYCLER_VISITED_HOST
    template <typename TBlockAttributes>
    SmallRecyclerVisitedHostHeapBlockT<TBlockAttributes> * AsRecyclerVisitedHostBlock();
#endif

#ifdef RECYCLER_WRITE_BARRIER
    template <typename TBlockAttributes>
    SmallNormalWithBarrierHeapBlockT<TBlockAttributes> * AsNormalWriteBarrierBlock();

    template <typename TBlockAttributes>
    SmallFinalizableWithBarrierHeapBlockT<TBlockAttributes> * AsFinalizableWriteBarrierBlock();
#endif

protected:
    char * address;
    Segment * segment;
    HeapBlockType const heapBlockType;
    bool needOOMRescan;                             // Set if we OOMed while marking a particular object
#if ENABLE_CONCURRENT_GC
    bool isPendingConcurrentSweep;
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    // This flag is to identify whether this block was made available for allocations during the concurrent sweep and 
    // still needs to be swept.
    bool isPendingConcurrentSweepPrep;
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    // This flag ensures a block doesn't get swept more than once during a given sweep.
    bool hasFinishedSweepObjects;

    // When allocate from a block during concurrent sweep some checks need to be delayed until
    // the free and mark bits are rebuilt. This flag helps skip those validations until then.
    bool wasAllocatedFromDuringSweep;
#endif
#endif
#endif

public:
    template <bool doSpecialMark, typename Fn>
    bool UpdateAttributesOfMarkedObjects(MarkContext * markContext, void * objectAddress, size_t objectSize, unsigned char attributes, Fn fn);
    void SetNeedOOMRescan(Recycler * recycler);
public:
    HeapBlock(HeapBlockType heapBlockType) :
        heapBlockType(heapBlockType),
        needOOMRescan(false)
    {
        static_assert(HeapBlockType::LargeBlockType == HeapBlockType::SmallBlockTypeCount, "LargeBlockType must come right after small+medium alloc block types");
        Assert(GetHeapBlockType() <= HeapBlock::HeapBlockType::BlockTypeCount);
    }

    HeapBlockType GetHeapBlockType() const
    {
        return (heapBlockType);
    }

#if (DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)) && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    bool WasAllocatedFromDuringSweep()
    {
        return this->wasAllocatedFromDuringSweep;
    }
#endif

    IdleDecommitPageAllocator* GetPageAllocator(HeapInfo * heapInfo);

    bool GetAndClearNeedOOMRescan()
    {
        if (this->needOOMRescan)
        {
            this->needOOMRescan = false;
            return true;
        }

        return false;
    }
#if DBG
#if GLOBAL_ENABLE_WRITE_BARRIER
    virtual void WBSetBit(char* addr) = 0;
    virtual void WBSetBitRange(char* addr, uint count) = 0;
    virtual void WBClearBit(char* addr) = 0;
    virtual void WBVerifyBitIsSet(char* addr) = 0;
    virtual void WBClearObject(char* addr) = 0;
#endif
    static void PrintVerifyMarkFailure(Recycler* recycler, char* objectAddress, char* target);
#endif


#if DBG
    virtual HeapInfo * GetHeapInfo() const = 0;
    virtual BOOL IsFreeObject(void* objectAddress) = 0;
#endif
    virtual BOOL IsValidObject(void* objectAddress) = 0;

    virtual byte* GetRealAddressFromInterior(void* interiorAddress) = 0;
    virtual size_t GetObjectSize(void* object) const = 0;
    virtual bool FindHeapObject(void* objectAddress, Recycler * recycler, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject) = 0;
    virtual bool TestObjectMarkedBit(void* objectAddress) = 0;
    virtual void SetObjectMarkedBit(void* objectAddress) = 0;

#ifdef RECYCLER_VERIFY_MARK
    virtual bool VerifyMark(void * objectAddress, void * target) = 0;
#endif
#ifdef PROFILE_RECYCLER_ALLOC
    virtual void * GetTrackerData(void * address) = 0;
    virtual void SetTrackerData(void * address, void * data) = 0;
#endif
#if DBG || defined(RECYCLER_STATS) || defined(RECYCLER_PAGE_HEAP)
    bool isForceSweeping;
#endif
#ifdef RECYCLER_PERF_COUNTERS
    virtual void UpdatePerfCountersOnFree() = 0;
#endif
};

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST
template <typename TBlockType>
struct HeapBlockSListItem {
    // SLIST_ENTRY needs to be the first element in the structure to avoid calculating offset with the SList API calls.
    SLIST_ENTRY itemEntry;
    TBlockType * itemHeapBlock;
};
#endif

enum SweepMode
{
    SweepMode_InThread,
#if ENABLE_CONCURRENT_GC
    SweepMode_Concurrent,
#if ENABLE_PARTIAL_GC
    SweepMode_ConcurrentPartial
#endif
#endif
};

// enum indicating the result of a sweep
enum SweepState
{
    SweepStateEmpty,                // the block is completely empty and can be released
    SweepStateSwept,                // the block is partially allocated, no object needs to be swept or finalized
    SweepStateFull,                 // the block is full, no object needs to be swept or finalized
    SweepStatePendingDispose,       // the block has object that needs to be finalized
#if ENABLE_CONCURRENT_GC
    SweepStatePendingSweep,         // the block has object that needs to be swept
#endif
};

template <class TBlockAttributes>
class ValidPointers
{
public:
    ValidPointers(ushort const * validPointers, uint bucketIndex);
    ushort GetInteriorAddressIndex(uint index) const;
    ushort GetAddressIndex(uint index) const;
private:
#if USE_VPM_TABLE
    ushort const * validPointers;
#endif

#if !USE_VPM_TABLE || DBG
    uint indexPerObject;
    uint maxObjectIndex;

    static uint CalculateBucketInfo(uint bucketIndex, uint * stride);
    static ushort CalculateAddressIndex(uint index, uint indexPerObject, uint maxObjectIndex);
    static ushort CalculateInteriorAddressIndex(uint index, uint indexPerObject, uint maxObjectIndex);
#endif
};

template <class TBlockAttributes>
class SmallHeapBlockT : public HeapBlock
{
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    friend class ::ScriptMemoryDumper;
#endif

    template <typename TBlockType>
    friend class SmallHeapBlockAllocator;
    friend class HeapInfo;
    friend class RecyclerSweep;

    template <typename TBlockType>
    friend class SmallNormalHeapBucketBase;

public:
    static const ushort InvalidAddressBit = 0xFFFF;

    typedef BVStatic<TBlockAttributes::BitVectorCount> SmallHeapBlockBitVector;
    struct  BlockInfo
    {
        ushort lastObjectIndexOnPage;
        ushort pageObjectCount;
    };

    bool FindImplicitRootObject(void* candidate, Recycler* recycler, RecyclerHeapObjectInfo& heapObject);

    SmallHeapBlockT* next;

    FreeObject* freeObjectList;
    FreeObject* lastFreeObjectHead;

    ValidPointers<TBlockAttributes> validPointers;
    HeapBucket * heapBucket;

    // Review: Should GetBucketIndex return a short instead of an int?
    const ushort bucketIndex;
    const ushort objectSize; // size in bytes
    const ushort objectCount;

    ushort freeCount;
    ushort lastFreeCount;
    ushort markCount;
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    // We need to keep track of the number of objects allocated during concurrent sweep, to be
    // able to make the correct determination about whether a block is EMPTY or FULL when the actual
    // sweep of this block happens.
    ushort objectsAllocatedDuringConcurrentSweepCount;
    ushort objectsMarkedDuringSweep;
    ushort lastObjectsAllocatedDuringConcurrentSweepCount;
    bool blockNotReusedInPartialHeapBlockList;
    bool blockNotReusedInPendingList;
#endif
#endif

#if ENABLE_PARTIAL_GC
    ushort oldFreeCount;
#endif
    bool   isInAllocator;
#if DBG
    bool   isClearedFromAllocator;

    bool   isIntegratedBlock;

    uint   lastUncollectedAllocBytes;
#endif
    SmallHeapBlockBitVector* markBits;
    SmallHeapBlockBitVector  freeBits;
#if DBG
    // TODO: (leish)(swb) move this to the block header if memory pressure on chk build is a problem
    // this causes 1/64 more memory usage on x64 or 1/32 more on x86
    BVStatic<TBlockAttributes::PageCount * AutoSystemInfo::PageSize / sizeof(void*)> wbVerifyBits;
#endif

#if DBG || defined(RECYCLER_STATS)
    SmallHeapBlockBitVector debugFreeBits;
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    SmallHeapBlockBitVector explicitFreeBits;
#endif
    bool IsFreeBitsValid() const
    {
        return this->freeObjectList == this->lastFreeObjectHead;
    }

    PageSegment * GetPageSegment() const { return (PageSegment *)GetSegment(); }
public:
    ~SmallHeapBlockT();

    void ProtectUnusablePages();
    void RestoreUnusablePages();

#if DBG && GLOBAL_ENABLE_WRITE_BARRIER
    virtual void WBVerifyBitIsSet(char* addr) override
    {
        uint index = (uint)(addr - this->address) / sizeof(void*);
        if (!wbVerifyBits.Test(index)) // TODO: (leish)(swb) need interlocked? seems not
        {
            PrintVerifyMarkFailure(this->GetRecycler(), addr, *(char**)addr);
        }
    }
    virtual void WBSetBit(char* addr) override
    {
        uint index = (uint)(addr - this->address) / sizeof(void*);
        wbVerifyBits.TestAndSetInterlocked(index);
    }
    virtual void WBSetBitRange(char* addr, uint count) override
    {
        uint index = (uint)(addr - this->address) / sizeof(void*);
        for (uint i = 0; i < count; i++)
        {
            wbVerifyBits.TestAndSetInterlocked(index + i);
        }
    }
    virtual void WBClearBit(char* addr) override
    {
        uint index = (uint)(addr - this->address) / sizeof(void*);
        wbVerifyBits.TestAndClearInterlocked(index);
    }
    virtual void WBClearObject(char* addr) override
    {
        Assert((uint)(addr - this->address) % this->objectSize == 0);
        uint index = (uint)(addr - this->address) / sizeof(void*);
        uint count = (uint)(this->objectSize / sizeof(void*));
        for (uint i = 0; i < count; i++)
        {
            wbVerifyBits.TestAndClearInterlocked(index + i);
        }
    }
#endif

    uint GetUnusablePageCount()
    {
#if USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
        // Note: This will always return 0 with the PageCount 1 for small blocks; but is needed if we tweak the page count.
        if (this->GetPageCount() > 1 && this->objectSize > HeapConstants::MaxObjectSizeWithMinGranularity)
        {
            return ((TBlockAttributes::PageCount * AutoSystemInfo::PageSize) % this->objectSize) / AutoSystemInfo::PageSize;
        }
        else
#endif
        {
            return 0;
        }
    }

#ifdef RECYCLER_WRITE_BARRIER
    bool IsWithBarrier() const;
#endif
    void RemoveFromHeapBlockMap(Recycler* recycler);
    char* GetAddress() const { return address; }
    char * GetEndAddress() const { return address + (this->GetPageCount() * AutoSystemInfo::PageSize);  }
    uint GetObjectWordCount() const { return this->objectSize / sizeof(void *); }
    uint GetPageCount() const;

    bool HasFreeObject() const
    {
        return freeObjectList != nullptr;
    }

    bool IsInAllocator() const;

    bool HasPendingDisposeObjects();
    bool HasAnyDisposeObjects();

#if DBG
    void VerifyMarkBitVector();
    bool IsClearedFromAllocator() const;
    void SetIsClearedFromAllocator(bool value);
    void SetIsIntegratedBlock() { this->isIntegratedBlock = true; }
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    void SetExplicitFreeBitForObject(void* object);
    void ClearExplicitFreeBitForObject(void* object);
#endif
#ifdef RECYCLER_STRESS
    void InduceFalsePositive(Recycler * recycler);
#endif

#if ENABLE_MEM_STATS
    void AggregateBlockStats(HeapBucketStats& stats, bool isAllocatorBlock = false, FreeObject* freeObjectList = nullptr, bool isBumpAllocated = false);
#endif

    /*
    * Quick description of the bit vectors
    *
    * The free bit vector is created by EnsureFreeBitVector. It's created by walking through the free list
    * for the heap block and setting the corresponding bit indices in the bit vector.
    *
    * The mark bit vector is more complicated. In most cases, it represents the objects that are alive (marked)
    * + the objects the are free (i.e in the free list). This is so that when we sweep, we don't bother sweeping over objects
    * that are already in the free list, we sweep over objects that were allocated and no longer alive since the last GC.
    * However, during rescan, the mark bit vector represents the objects that are actually alive. We set the marked bit
    * vector to this state before calling RescanObjects, so that we scan through only the objects that are actually alive.
    * This means that we don't rescan newly allocated objects during rescan, because rescan doesn't change add new mark bits.
    * Instead, these objects are marked after rescan during in-thread mark if they're actually alive.
    */
    SmallHeapBlockBitVector * GetMarkedBitVector() { return markBits; }
    SmallHeapBlockBitVector * GetFreeBitVector() { return &freeBits; }

    SmallHeapBlockBitVector const * GetInvalidBitVector();
    BlockInfo const * GetBlockInfo();

    ushort GetObjectBitDelta();

    static uint GetObjectBitDeltaForBucketIndex(uint bucketIndex);

    static char* GetBlockStartAddress(char* address)
    {
        uintptr_t mask = ~((TBlockAttributes::PageCount * AutoSystemInfo::PageSize) - 1);
        return (char*)((uintptr_t)address & mask);
    }

    bool IsValidBitIndex(uint bitIndex)
    {
        Assert(bitIndex < TBlockAttributes::BitVectorCount);
        return bitIndex % GetObjectBitDelta() == 0;
    }

    void MarkImplicitRoots();

    void SetNextBlock(SmallHeapBlockT * next) { this->next=next; }
    SmallHeapBlockT * GetNextBlock() const { return next; }

    uint GetObjectSize() const { return objectSize; }
    uint GetObjectCount() const { return objectCount; }
    uint GetMarkedCount() const { return markCount; }

    // Valid during sweep time
    ushort GetExpectedFreeObjectCount() const;
    uint GetExpectedFreeBytes() const;
    ushort GetExpectedSweepObjectCount() const;

#if DBG || defined(RECYCLER_STATS)
    SmallHeapBlockBitVector * GetDebugFreeBitVector() { return &debugFreeBits; }
#endif
#if DBG
    virtual HeapInfo * GetHeapInfo() const override;
    virtual BOOL IsFreeObject(void* objectAddress) override;
#endif
    virtual BOOL IsValidObject(void* objectAddress) override;
    byte* GetRealAddressFromInterior(void* interiorAddress) override sealed;
    bool TestObjectMarkedBit(void* objectAddress) override sealed;
    void SetObjectMarkedBit(void* objectAddress) override;
    virtual size_t GetObjectSize(void* object) const override { return objectSize; }

    uint GetMarkCountForSweep();
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    void ResetConcurrentSweepAllocationCounts();
#endif
#endif
    SweepState Sweep(RecyclerSweep& recyclerSweep, bool queuePendingSweep, bool allocable, ushort finalizeCount = 0, bool hasPendingDispose = false);
    template <SweepMode mode>
    void SweepObjects(Recycler * recycler);

    uint GetAndClearLastFreeCount();
    void ClearAllAllocBytes();      // Reset all unaccounted alloc bytes and the new alloc count
#if ENABLE_PARTIAL_GC
    uint GetAndClearUnaccountedAllocBytes();
    void AdjustPartialUncollectedAllocBytes(RecyclerSweep& recyclerSweep, uint const expectSweepCount);
    bool DoPartialReusePage(RecyclerSweep const& recyclerSweep, uint& expectFreeByteCount);
#if DBG || defined(RECYCLER_STATS)
    void SweepVerifyPartialBlock(Recycler * recycler);
#endif
#endif
    void TransferProcessedObjects(FreeObject * list, FreeObject * tail);
    BOOL ReassignPages(Recycler * recycler);
    BOOL SetPage(__in_ecount_pagesize char * baseAddress, PageSegment * pageSegment, Recycler * recycler);

    void ReleasePages(Recycler * recycler);
    void ReleasePagesSweep(Recycler * recycler);
    void ReleasePagesShutdown(Recycler * recycler);

#if ENABLE_BACKGROUND_PAGE_FREEING
    void BackgroundReleasePagesSweep(Recycler* recycler);
#endif

    void Reset();

    void EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size));

    bool IsImplicitRoot(uint objectIndex)
    {
        return (this->ObjectInfo(objectIndex) & ImplicitRootBit) != 0;
    }

#ifdef RECYCLER_SLOW_CHECK_ENABLED
    void Check(bool expectFull, bool expectPending);
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    void Verify(bool pendingDispose = false);
    void VerifyBumpAllocated(_In_ char * bumpAllocatedAddres);
#endif
#ifdef RECYCLER_VERIFY_MARK
    void VerifyMark();
    virtual bool VerifyMark(void * objectAddress, void * target) override;
#endif
#ifdef RECYCLER_PERF_COUNTERS
    virtual void UpdatePerfCountersOnFree() override sealed;
#endif
#ifdef PROFILE_RECYCLER_ALLOC
    virtual void * GetTrackerData(void * address) override;
    virtual void SetTrackerData(void * address, void * data) override;
#endif

    ushort GetAddressBitIndex(void * objectAddress);
    static ushort GetAddressBitIndex(void * objectAddress, uint bucketIndex);
    static void * GetRealAddressFromInterior(void * objectAddress, uint objectSize, byte bucketIndex);

protected:
    static size_t GetAllocPlusSize(uint objectCount);
    inline void SetAttributes(void * address, unsigned char attributes);
    ushort GetAddressIndex(void * objectAddress);

    SmallHeapBlockT(HeapBucket * bucket, ushort objectSize, ushort objectCount, HeapBlockType heapBlockType);

    ushort GetInteriorAddressIndex(void * interiorAddress);
    ushort GetObjectIndexFromBitIndex(ushort bitIndex);

    template <SweepMode mode>
    void SweepObject(Recycler * recycler, uint index, void * addr);


    void EnqueueProcessedObject(FreeObject **list, void * objectAddress, uint index);
    void EnqueueProcessedObject(FreeObject **list, FreeObject ** tail, void * objectAddress, uint index);

#ifdef RECYCLER_SLOW_CHECK_ENABLED
    template <typename TBlockType>
    bool GetFreeObjectListOnAllocatorImpl(FreeObject ** freeObjectList);
    virtual bool GetFreeObjectListOnAllocator(FreeObject ** freeObjectList) = 0;
    void CheckDebugFreeBitVector(bool isCollecting);
    void CheckFreeBitVector(bool isCollecting);
#endif

    SmallHeapBlockBitVector * EnsureFreeBitVector(bool isCollecting = true);
    SmallHeapBlockBitVector * BuildFreeBitVector();
    ushort BuildFreeBitVector(SmallHeapBlockBitVector * bv);

    BOOL IsInFreeObjectList(void * objectAddress);

    void ClearObjectInfoList();
    byte& ObjectInfo(uint index);

    inline void FillFreeMemory(__in_bcount(size) void * address, size_t size);

    template <typename TBlockType>
    bool FindHeapObjectImpl(void* objectAddress, Recycler * recycler, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject);
protected:
    IdleDecommitPageAllocator * GetPageAllocator();

    void Init(ushort objectSize, ushort objectCount);
    void ConstructorCommon(HeapBucket * bucket, ushort objectSize, ushort objectCount, HeapBlockType heapBlockType);

    template <typename Fn>
    void ForEachAllocatedObject(Fn fn);

    template <typename Fn>
    void ForEachAllocatedObject(ObjectInfoBits attributes, Fn fn);

    template <typename Fn>
    void ScanNewImplicitRootsBase(Fn fn);

    // This is public for code readability but this
    // returns a value only on debug builds. On retail builds
    // this returns null
    Recycler * GetRecycler() const;

#if DBG
    uint GetMarkCountOnHeapBlockMap() const;
#endif

private:
#ifdef PROFILE_RECYCLER_ALLOC
    void ** GetTrackerDataArray();
#endif
};

#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
// Forward declare specializations
template<>
SmallHeapBlockT<MediumAllocationBlockAttributes>::SmallHeapBlockT(HeapBucket * bucket, ushort objectSize, ushort objectCount, HeapBlockType heapBlockType);

template <>
uint
SmallHeapBlockT<MediumAllocationBlockAttributes>::GetObjectBitDeltaForBucketIndex(uint bucketIndex);

template <>
uint
SmallHeapBlockT<MediumAllocationBlockAttributes>::GetUnusablePageCount();

template <>
void
SmallHeapBlockT<MediumAllocationBlockAttributes>::ProtectUnusablePages();

template <>
void
SmallHeapBlockT<MediumAllocationBlockAttributes>::RestoreUnusablePages();
#endif

// Declare the class templates
typedef SmallHeapBlockT<SmallAllocationBlockAttributes>  SmallHeapBlock;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
typedef SmallHeapBlockT<MediumAllocationBlockAttributes> MediumHeapBlock;
#endif

extern template class SmallHeapBlockT<SmallAllocationBlockAttributes>;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
extern template class SmallHeapBlockT<MediumAllocationBlockAttributes>;
#endif

extern template class ValidPointers<SmallAllocationBlockAttributes>;
#if !USE_STAGGERED_OBJECT_ALIGNMENT_BUCKETS
extern template class ValidPointers<MediumAllocationBlockAttributes>;
#endif

class HeapBlockList
{
public:
    template <typename TBlockType, typename Fn>
    static void ForEach(TBlockType * list, Fn fn)
    {
        ForEach<TBlockType, Fn>(list, nullptr, fn);
    }

    template <typename TBlockType, typename Fn>
    static void ForEach(TBlockType * list, TBlockType * tail, Fn fn)
    {
        TBlockType * heapBlock = list;
        while (heapBlock != tail)
        {
            fn(heapBlock);
            heapBlock = heapBlock->GetNextBlock();
        }
    }

    template <typename TBlockType, typename Fn>
    static void ForEachEditing(TBlockType * list, Fn fn)
    {
        ForEachEditing<TBlockType, Fn>(list, nullptr, fn);
    };

    template <typename TBlockType, typename Fn>
    static void ForEachEditing(TBlockType * list, TBlockType * tail,  Fn fn)
    {
        TBlockType * heapBlock = list;
        while (heapBlock != tail)
        {
            TBlockType * nextBlock = heapBlock->GetNextBlock();
            fn(heapBlock);
            heapBlock = nextBlock;
        }
    };

    template <typename TBlockType>
    static size_t Count(TBlockType * list)
    {
        size_t currentHeapBlockCount = 0;
        HeapBlockList::ForEach(list, [&currentHeapBlockCount](TBlockType * heapBlock)
        {
            currentHeapBlockCount++;
        });
        return currentHeapBlockCount;
    };

    template <typename TBlockType>
    static TBlockType * Tail(TBlockType * list)
    {
        TBlockType * tail = nullptr;
        HeapBlockList::ForEach(list, [&tail](TBlockType * heapBlock)
        {
            tail = heapBlock;
        });
        return tail;
    }

#if DBG
    template <typename TBlockType>
    static bool Contains(TBlockType * block, TBlockType * list, TBlockType * tail = nullptr)
    {
        TBlockType * heapBlock = list;
        while (heapBlock != tail)
        {
            if (heapBlock == block)
            {
                return true;
            }
            heapBlock = heapBlock->GetNextBlock();
        }
        return false;
    }
#endif
};
}

