//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{
//#define RECYCLER_WRITE_BARRIER_INLINE_RECYCLER

#ifdef RECYCLER_WRITE_BARRIER

// Controls whether we're using a 128 byte granularity card table or a 4K granularity byte array to indicate that a range of memory is dirty
#define RECYCLER_WRITE_BARRIER_BYTE

#if GLOBAL_ENABLE_WRITE_BARRIER
// Controls whether the JIT is software write barrier aware
#define RECYCLER_WRITE_BARRIER_JIT
#endif

// Controls whether we can allocate SWB memory or not
// Turning this on leaves the rest of the SWB infrastructure intact,
// it'll simply allocate SWB objects are regular objects
#define RECYCLER_WRITE_BARRIER_ALLOC

#ifdef RECYCLER_WRITE_BARRIER_ALLOC
// Controls which page allocator to allocate SWB objects in
// By default, they use the recycler page allocator
// The following switches allow the user to either use the thread (leaf) page allocator or an entirely new page allocator for SWB objects

//#define RECYCLER_WRITE_BARRIER_ALLOC_THREAD_PAGE
#define RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE

#if defined(RECYCLER_WRITE_BARRIER_ALLOC_THREAD_PAGE) && defined(RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE)
#error Not supported
#endif
#endif // RECYCLER_WRITE_BARRIER_ALLOC

#ifdef RECYCLER_WRITE_BARRIER_BYTE

#define DIRTYBIT 0x01

#if ENABLE_DEBUG_CONFIG_OPTIONS
#define WRITE_BARRIER_PAGE_BIT 0x2
#else
#define WRITE_BARRIER_PAGE_BIT 0x0
#endif

// indicate the barrier has ever been cleared
#if ENABLE_DEBUG_CONFIG_OPTIONS
#define WRITE_BARRIER_CLEAR_MARK 0x4
#else
#define WRITE_BARRIER_CLEAR_MARK 0x0
#endif

#endif

#ifdef _M_X64_OR_ARM64
#ifdef RECYCLER_WRITE_BARRIER_BYTE

#define X64_WB_DIAG 1


class X64WriteBarrierCardTableManager
{
public:
    X64WriteBarrierCardTableManager() :
        _cardTable(nullptr)
    {
    }

    ~X64WriteBarrierCardTableManager();

    BYTE * Initialize();

    // Called when a script thread is initialized
    bool OnThreadInit();

    // Called when a page allocator segment is allocated
    bool OnSegmentAlloc(_In_ char* segmentAddress, DECLSPEC_GUARD_OVERFLOW size_t numPages);

    // Called when a page allocator segment is freed
    bool OnSegmentFree(_In_ char* segmentAddress, size_t numPages);

    // Get the card table for the 64 bit address space
    BYTE * GetAddressOfCardTable() { return _cardTable; }

#if ENABLE_DEBUG_CONFIG_OPTIONS
    void AssertWriteToAddress(_In_ void* address)
    {
        Assert(_cardTable);
        Assert(committedSections.Test(GetSectionIndex(address)));
    }
    BOOLEAN IsCardTableCommited(_In_ uintptr_t index)
    {
        return committedSections.Test((BVIndex)(index/ AutoSystemInfo::PageSize));
    }
    BOOLEAN IsCardTableCommited(_In_ void* address)
    {
        return committedSections.Test(GetSectionIndex(address));
    }
#endif

private:
    BVIndex GetSectionIndex(void* address);

    // In our card table scheme, 4KB of memory is mapped to 1 byte indicating whether
    // it's written to or not. A page in Windows is also 4KB. Therefore, a page's worth
    // of memory in the card table will track the state of 16MB of memory, which is what
    // we term as a section. A bit set in this bit vector tracks whether a page corresponding
    // to said 16MB is committed in the card table.
    typedef BVSparse<HeapAllocator> CommittedSectionBitVector;
    BYTE* _cardTable;
    size_t _cardTableNumEntries;
    CriticalSection _cardTableInitCriticalSection;

#ifdef X64_WB_DIAG
    enum CommitState
    {
        CommitStateOnSegmentAlloc,
        CommitStateOnNeedCommit,
        CommitStateOnSectionCommitted,
        CommitStateOnCommitBitSet,

        CommitStateFailedMaxAddressExceeded,
        CommitStateFailedVirtualAlloc,
        CommitStateFailedCommitBitSet
    };

    char*  _lastSegmentAddress;
    BYTE*  _lastSectionStart;
    BYTE*  _lastSectionEnd;
    size_t _lastSegmentNumPages;
    size_t _lastSectionIndexStart;
    size_t _lastSectionIndexLast;
    CommitState _lastCommitState;
    char* _stackbase;
    char* _stacklimit;
#endif

#ifdef X64_WB_DIAG
#define SetCommitState(state) this->_lastCommitState = CommitState##state
#else
#define SetCommitState(state)
#endif

    static CommittedSectionBitVector committedSections;
};
#else
#error Not implemented
#endif
#endif

class RecyclerWriteBarrierManager
{
public:
    static void WriteBarrier(void * address);
    static void WriteBarrier(void * address, size_t bytes);
#if ENABLE_DEBUG_CONFIG_OPTIONS
    static void ToggleBarrier(void * address, size_t bytes, bool enable);
    static bool IsBarrierAddress(void * address);
    static bool IsBarrierAddress(uintptr_t index);
    static void VerifyIsBarrierAddress(void * address, size_t bytes);
    static void VerifyIsNotBarrierAddress(void * address, size_t bytes);
    static void VerifyIsBarrierAddress(void * address);
    static bool Initialize();
#endif

#if ENABLE_DEBUG_CONFIG_OPTIONS
    static bool IsCardTableCommited(_In_ uintptr_t index)
    {
#ifdef _M_X64_OR_ARM64
        return x64CardTableManager.IsCardTableCommited(index) != FALSE;
#else
        return true;
#endif
    }
    static bool IsCardTableCommitedAddress(_In_ void* address)
    {
#ifdef _M_X64_OR_ARM64
        return x64CardTableManager.IsCardTableCommited(address) != FALSE;
#else
        return true;
#endif
    }
#endif

    // For JIT
    static uintptr_t GetCardTableIndex(void * address);
#ifdef RECYCLER_WRITE_BARRIER_BYTE
#ifdef _M_X64_OR_ARM64
    static BYTE * GetAddressOfCardTable() { return x64CardTableManager.GetAddressOfCardTable(); }
#else
    static BYTE * GetAddressOfCardTable() { return cardTable; }
#endif
#else
    static DWORD * GetAddressOfCardTable() { return cardTable; }
#endif

    // For GC
#ifdef _M_X64_OR_ARM64
    static bool OnThreadInit();
    static bool OnSegmentAlloc(_In_ char* segment, DECLSPEC_GUARD_OVERFLOW size_t pageCount);
    static bool OnSegmentFree(_In_ char* segment, size_t pageCount);
#endif

    static void ResetWriteBarrier(void * address, size_t pageCount);
#ifdef RECYCLER_WRITE_BARRIER_BYTE
    static BYTE  GetWriteBarrier(void * address);
#else
    static DWORD GetWriteBarrier(void * address);
#endif

    static size_t const s_WriteBarrierPageSize = 4096;
    static uint const s_BitArrayCardTableShift = 7;
    static uint const s_BytesPerCardBit = 1 << s_BitArrayCardTableShift;  // 128 = 1 << 7
    static uint const s_BytesPerCard = s_BytesPerCardBit * 32;      // 4K  = 1 << 12 = 128 << 5

private:

#ifdef RECYCLER_WRITE_BARRIER_BYTE
#ifdef _M_X64_OR_ARM64
    // On AMD64, we use a different scheme
    // As of Windows 8.1, the process user-mode address space is 128TB
    // We still use a write barrier page size of 4KB
    // If we used a static card table, the address space needed for the array would be 32 GB
    // To get around this, we instead reserve 32 GB of address space for the card table
    // The page allocator will commit the relevant parts of the card table when
    // needed. Since the card table is dynamically allocated at runtime, we need one additional
    // indirection to look up the card.
    static X64WriteBarrierCardTableManager x64CardTableManager;
    static BYTE* cardTable;                         // 1 byte per 4096
#else
    static BYTE cardTable[1 * 1024 * 1024];         // 1 byte per 4096
#endif

#else
    static DWORD cardTable[1 * 1024 * 1024];        // 128 bytes per bit, 4096 per DWORD
#endif
};

#ifdef RECYCLER_TRACE
#define SwbTrace(flags, ...) \
if (flags.Trace.IsEnabled(Js::MemoryAllocationPhase)) \
{ \
    Output::Print(__VA_ARGS__); \
}
#define GlobalSwbVerboseTrace(...) \
if (Js::Configuration::Global.flags.Verbose && \
    Js::Configuration::Global.flags.Trace.IsEnabled(Js::MemoryAllocationPhase)) \
{ \
    Output::Print(__VA_ARGS__); \
}

#define SwbVerboseTrace(flags, ...) \
if (flags.Verbose && \
    flags.Trace.IsEnabled(Js::MemoryAllocationPhase)) \
{ \
    Output::Print(__VA_ARGS__); \
}
#else
#define SwbTrace(...)
#define SwbVerboseTrace(...)
#define GlobalSwbVerboseTrace(...)
#endif

#endif
}
