//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if defined(_M_X64_OR_ARM64)
#define UINT_PAD_64BIT(x) uint x
#else
#define UINT_PAD_64BIT(x)
#endif

namespace Memory
{
struct LargeObjectHeader
{
    uint objectIndex;
    UINT_PAD_64BIT(unused1);
    size_t objectSize;

private:
    LargeObjectHeader * next;
    // xxxxxxxxyyyyyyyy, where xxxxxxxx = attributes field, yyyyyyyy = checksum field
#if LARGEHEAPBLOCK_ENCODING
    ushort attributesAndChecksum;
#else
    unsigned char attributes;
    unsigned char unused2;
#endif

public:
    bool markOnOOMRescan : 1;
#ifdef RECYCLER_WRITE_BARRIER
    bool hasWriteBarrier : 1;
#endif
#ifdef RECYCLER_PAGE_HEAP
    bool isObjectPageLocked : 1;
#endif
#if DBG
    bool isExplicitFreed : 1;
#endif
    UINT_PAD_64BIT(unused4);

    void *GetAddress();
#ifdef LARGEHEAPBLOCK_ENCODING
    unsigned char CalculateCheckSum(LargeObjectHeader* next, unsigned char attributes);
    LargeObjectHeader* EncodeNext(uint cookie, LargeObjectHeader* next);
    ushort EncodeAttributesAndChecksum(uint cookie, ushort attributesWithChecksum);
    LargeObjectHeader* DecodeNext(uint cookie, LargeObjectHeader* next);
    ushort DecodeAttributesAndChecksum(uint cookie);
#else
    unsigned char * GetAttributesPtr();
#endif
    void SetNext(uint cookie, LargeObjectHeader* next);
    LargeObjectHeader * GetNext(uint cookie);
    void SetAttributes(uint cookie, unsigned char attributes);
    unsigned char GetAttributes(uint cookie);
};
#if defined(_M_X64_OR_ARM64)
static_assert(sizeof(LargeObjectHeader) == 0x20, "Incorrect LargeObjectHeader size");
#else
static_assert(sizeof(LargeObjectHeader) == 0x10, "Incorrect LargeObjectHeader size");
#endif
class LargeHeapBlock;
class LargeHeapBucket;

struct LargeHeapBlockFreeListEntry
{
    uint headerIndex;
    size_t objectSize;
    LargeHeapBlock* heapBlock;
    LargeHeapBlockFreeListEntry* next;
};

struct LargeHeapBlockFreeList
{
public:
    LargeHeapBlockFreeList(LargeHeapBlock* heapBlock):
        previous(nullptr),
        next(nullptr),
        entries(nullptr),
        heapBlock(heapBlock)
    {
    }

    LargeHeapBlockFreeList* previous;
    LargeHeapBlockFreeList* next;
    LargeHeapBlockFreeListEntry* entries;
    LargeHeapBlock* heapBlock;
};

class HeapInfo;

#ifdef RECYCLER_PAGE_HEAP
struct PageHeapData
{
    ~PageHeapData();
    bool isLockedWithPageHeap;
    bool isGuardPageDecommited;
    PageHeapMode pageHeapMode;

    uint actualPageCount;
    ushort paddingBytes;
    ushort unusedBytes;
    char* guardPageAddress;
    char* objectAddress;
    char* objectEndAddr;
    char* objectPageAddr;    
    const char* lastMarkedBy;
#ifdef STACK_BACK_TRACE
    StackBackTrace* pageHeapAllocStack;
    StackBackTrace* pageHeapFreeStack;
    const static StackBackTrace* s_StackTraceAllocFailed;
#endif
};
#endif

// CONSIDER: Templatizing this so that we don't have free list support if we don't need it
class LargeHeapBlock sealed : public HeapBlock
{
    friend class HeapInfo;
public:
    Recycler * GetRecycler() const;

#if DBG
    virtual BOOL IsFreeObject(void* objectAddress) override;
#endif
    virtual BOOL IsValidObject(void* objectAddress) override;

    template <bool doSpecialMark>
    void Mark(void* objectAddress, MarkContext * markContext);
    virtual byte* GetRealAddressFromInterior(void* interiorAddress) override;
    bool TestObjectMarkedBit(void* objectAddress) override;
    void SetObjectMarkedBit(void* objectAddress) override;
    bool FindHeapObject(void* objectAddress, Recycler * recycler, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject) override;
    virtual size_t GetObjectSize(void* object) const override;
    bool FindImplicitRootObject(void* objectAddress, Recycler * recycler, RecyclerHeapObjectInfo& heapObject);

    size_t GetPageCount() const { return pageCount; }
    LargeHeapBlock * GetNextBlock() { return next; }
    void SetNextBlock(LargeHeapBlock * next) { this->next = next; }
    size_t GetFreeSize() const { return addressEnd - allocAddressEnd; }
    static LargeHeapBlock * New(__in char * address, DECLSPEC_GUARD_OVERFLOW size_t pageCount, Segment * segment, DECLSPEC_GUARD_OVERFLOW uint objectCount, LargeHeapBucket* bucket);
    static void Delete(LargeHeapBlock * heapBlock);
    bool IsInPendingDisposeList() { return isInPendingDisposeList; }
    void SetIsInPendingDisposeList(bool isInPendingDisposeList) { this->isInPendingDisposeList = isInPendingDisposeList; }

#if DBG
    void SetHasDisposeBeenCalled(bool hasDisposeBeenCalled) { this->hasDisposeBeenCalled = hasDisposeBeenCalled; }
#endif

    LargeHeapBlockFreeList* GetFreeList() { return &this->freeList; }

    ~LargeHeapBlock();

    size_t Rescan(Recycler* recycler, bool isPartialSwept, RescanFlags flags);
#if ENABLE_PARTIAL_GC && ENABLE_CONCURRENT_GC
    void PartialTransferSweptObjects();
    void FinishPartialCollect(Recycler * recycler);
#endif
    void ReleasePages(Recycler * recycler);
    void ReleasePagesSweep(Recycler * recycler);
    void ReleasePagesShutdown(Recycler * recycler);
    void ResetMarks(ResetMarkFlags flags, Recycler* recycler);
    void ScanInitialImplicitRoots(Recycler * recycler);
    void ScanNewImplicitRoots(Recycler * recycler);
    SweepState Sweep(RecyclerSweep& recyclerSweep, bool queuePendingSweep);
    template <SweepMode mode>
    void SweepObjects(Recycler * recycler);
    bool TransferSweptObjects();
    void DisposeObjects(Recycler * recycler);
    void FinalizeObjects(Recycler* recycler);
    void FinalizeAllObjects();

    char* GetBeginAddress() const { return address; }
    char* GetEndAddress() const { return addressEnd; }

    bool TryGetAttributes(void* objectAddress, unsigned char * pAttr);
    bool TryGetAttributes(LargeObjectHeader *objectHeader, unsigned char * pAttr);

    char * Alloc(DECLSPEC_GUARD_OVERFLOW size_t size, ObjectInfoBits attributes);
    char * TryAllocFromFreeList(DECLSPEC_GUARD_OVERFLOW size_t size, ObjectInfoBits attributes);

    static size_t GetPagesNeeded(DECLSPEC_GUARD_OVERFLOW size_t size, bool multiplyRequest);
    static uint GetMaxLargeObjectCount(size_t pageCount, size_t firstAllocationSize);

    void EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size));

#ifdef RECYCLER_SLOW_CHECK_ENABLED
    void Check(bool expectFull, bool expectPending);
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    void Verify(Recycler * recycler);
#endif
#ifdef RECYCLER_VERIFY_MARK
    void VerifyMark();
    virtual bool VerifyMark(void * objectAddress, void * target) override;
#endif
#ifdef RECYCLER_PERF_COUNTERS
    virtual void UpdatePerfCountersOnFree() override;
#endif
#ifdef PROFILE_RECYCLER_ALLOC
    virtual void * GetTrackerData(void * address) override;
    virtual void SetTrackerData(void * address, void * data) override;
#endif
private:
    friend class LargeHeapBucket;
#ifdef RECYCLER_MEMORY_VERIFY
    friend class Recycler;
#endif

    LargeHeapBlock(__in char * address, DECLSPEC_GUARD_OVERFLOW size_t pageCount, Segment * segment, DECLSPEC_GUARD_OVERFLOW uint objectCount, LargeHeapBucket* bucket);
    static LargeObjectHeader * GetHeaderFromAddress(void * address);
    LargeObjectHeader * GetHeader(void * address) const;
    LargeObjectHeader ** HeaderList() const;
    LargeObjectHeader * GetHeaderByIndex(uint index) const
    {
        Assert(index < this->allocCount);
        LargeObjectHeader * header = this->HeaderList()[index];
#if ENABLE_PARTIAL_GC && ENABLE_CONCURRENT_GC
        if (IsPartialSweptHeader(header))
        {
            return nullptr;
        }
#endif
        return header;
    }

    uint GetMarkCount();
    bool GetObjectHeader(void* objectAddress, LargeObjectHeader** ppHeader);
    BOOL IsNewHeapBlock() const { return lastCollectAllocCount == 0; }
    static size_t GetAllocPlusSize(DECLSPEC_GUARD_OVERFLOW uint objectCount);
    char * AllocFreeListEntry(DECLSPEC_GUARD_OVERFLOW size_t size, ObjectInfoBits attributes, LargeHeapBlockFreeListEntry* entry);

#if ENABLE_CONCURRENT_GC
    bool IsPageDirty(char* page, RescanFlags flags, bool isWriteBarrier);
    bool RescanOnePage(Recycler * recycler, RescanFlags flags);
    size_t RescanMultiPage(Recycler * recycler, RescanFlags flags);
#else
    bool RescanOnePage(Recycler * recycler);
    size_t RescanMultiPage(Recycler * recycler);
#endif

    template <SweepMode>
    void SweepObject(Recycler * recycler, LargeObjectHeader * header);

    bool TrimObject(Recycler* recycler, LargeObjectHeader* header, size_t sizeOfObject, bool needSuspend = false);

    void FinalizeObject(Recycler* recycler, LargeObjectHeader* header);

    void FillFreeMemory(Recycler * recycler, __in_bcount(size) void * address, size_t size);
#if ENABLE_PARTIAL_GC && ENABLE_CONCURRENT_GC
    bool IsPartialSweptHeader(LargeObjectHeader * header) const
    {
        Assert(this->hasPartialFreeObjects || (((size_t)header & PartialFreeBit) != PartialFreeBit));
        return ((size_t)header & PartialFreeBit) == PartialFreeBit;
    }
    static const size_t PartialFreeBit = 0x1;
#endif
    size_t pageCount;

    // The number of allocations that have occurred from this heap block
    // This only increases, never decreases. Instead, we rely on the mark/weakRef/finalize counts
    // to determine if an object has been freed or not. So when we alloc, we keep alloc'ing
    // from the last allocation even if there are holes in the large heap block. If we free an object,
    // we simply set its header to null. But otherwise, we simply constantly keep increasing allocCount
    // till the heap block is full.
    uint allocCount;

    // Maximum number of objects that can be fit into this heap block
    // This is based on the fact that the largest small object size is 1024
    // So the smallest large object size is 1025. We can calculate the max object count
    // as follows. The total size available to us is the pageCount * pageSize.
    // When we allocate the large heap block, it's to fit a large object. So the amount
    // of space remaining is totalSize - sizeOfLargeObject - sizeOfLargeObjectHeader
    // So the max number of objects this heap block can support is remainingSize / maxSmallObjectSize + 1
    // where 1 is the initial object that was used to create the heap block
    uint objectCount;
    char * allocAddressEnd;
    char * addressEnd;

    LargeHeapBlock* next;
    LargeObjectHeader * pendingDisposeObject;

    LargeHeapBucket* bucket;
    HeapInfo * heapInfo;
    LargeHeapBlockFreeList freeList;

#ifdef RECYCLER_PAGE_HEAP
    PageHeapData* pageHeapData;
public:
    void VerifyPageHeapPattern();
    inline bool InPageHeapMode() const { return pageHeapData != nullptr && pageHeapData->pageHeapMode != PageHeapMode::PageHeapModeOff; }
    PageHeapData* GetPageHeapData() { return pageHeapData; }
    void PageHeapLockPages();
    void PageHeapUnLockPages();

    void CapturePageHeapAllocStack();
    void CapturePageHeapFreeStack();
#endif

    uint lastCollectAllocCount;
    uint finalizeCount;
    bool isInPendingDisposeList;

#if DBG
    bool hasDisposeBeenCalled;
    bool hasPartialFreeObjects;
    // The following get set if an object is swept and we freed its pages
    bool hadTrimmed;
    uint expectedSweepCount;
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    friend class ::ScriptMemoryDumper;
#endif
#ifdef PROFILE_RECYCLER_ALLOC
    void ** GetTrackerDataArray();
#endif

#if DBG && GLOBAL_ENABLE_WRITE_BARRIER
private:
    static CriticalSection wbVerifyBitsLock;
    BVSparse<HeapAllocator> wbVerifyBits;
public:
    virtual void WBSetBit(char* addr) override;
    virtual void WBSetBitRange(char* addr, uint count) override;
    virtual void WBClearBit(char* addr) override;
    virtual void WBVerifyBitIsSet(char* addr) override;
    virtual void WBClearObject(char* addr) override;
#endif
};
}
