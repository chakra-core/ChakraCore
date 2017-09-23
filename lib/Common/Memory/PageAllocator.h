//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include "PageAllocatorDefines.h"
#include "Exceptions/ExceptionBase.h"

#ifdef PROFILE_MEM
struct PageMemoryData;
#endif

class CodeGenNumberThreadAllocator;
struct XProcNumberPageSegmentManager;

namespace Memory
{
typedef void* FunctionTableHandle;

#if DBG_DUMP

#define GUARD_PAGE_TRACE(...) \
    if (Js::Configuration::Global.flags.PrintGuardPageBounds) \
{ \
    Output::Print(__VA_ARGS__); \
}

#define PAGE_ALLOC_TRACE(format, ...) PAGE_ALLOC_TRACE_EX(false, false, format, ##__VA_ARGS__)
#define PAGE_ALLOC_VERBOSE_TRACE(format, ...) PAGE_ALLOC_TRACE_EX(true, false, format, ##__VA_ARGS__)
#define PAGE_ALLOC_VERBOSE_TRACE_0(format) PAGE_ALLOC_TRACE_EX(true, false, format, "")

#define PAGE_ALLOC_TRACE_AND_STATS(format, ...) PAGE_ALLOC_TRACE_EX(false, true, format, ##__VA_ARGS__)
#define PAGE_ALLOC_TRACE_AND_STATS_0(format) PAGE_ALLOC_TRACE_EX(false, true, format, "")
#define PAGE_ALLOC_VERBOSE_TRACE_AND_STATS(format, ...) PAGE_ALLOC_TRACE_EX(true, true, format, ##__VA_ARGS__)
#define PAGE_ALLOC_VERBOSE_TRACE_AND_STATS_0(format) PAGE_ALLOC_TRACE_EX(true, true, format, "")

#define PAGE_ALLOC_TRACE_EX(verbose, stats, format, ...)                \
    if (this->pageAllocatorFlagTable.Trace.IsEnabled(Js::PageAllocatorPhase)) \
    { \
        if (!verbose || this->pageAllocatorFlagTable.Verbose) \
        {   \
            Output::Print(_u("%p : %p> PageAllocator(%p): "), GetCurrentThreadContextId(), ::GetCurrentThreadId(), this); \
            if (debugName != nullptr) \
            { \
                Output::Print(_u("[%s] "), this->debugName); \
            } \
            Output::Print(format, ##__VA_ARGS__);         \
            Output::Print(_u("\n")); \
            if (stats && this->pageAllocatorFlagTable.Stats.IsEnabled(Js::PageAllocatorPhase)) \
            { \
                this->DumpStats();         \
            }  \
            Output::Flush(); \
        } \
    }
#else
#define PAGE_ALLOC_TRACE(format, ...)
#define PAGE_ALLOC_VERBOSE_TRACE(format, ...)
#define PAGE_ALLOC_VERBOSE_TRACE_0(format)

#define PAGE_ALLOC_TRACE_AND_STATS(format, ...)
#define PAGE_ALLOC_VERBOSE_TRACE_AND_STATS(format, ...)
#define PAGE_ALLOC_TRACE_AND_STATS_0(format)
#define PAGE_ALLOC_VERBOSE_TRACE_AND_STATS_0(format)

#endif

#ifdef _M_X64
#define XDATA_RESERVE_PAGE_COUNT   (2)       // Number of pages per page segment (32 pages) reserved for xdata.
#else
#define XDATA_RESERVE_PAGE_COUNT   (0)       // ARM uses the heap, so it's not required.
#endif

//
// Allocation done by the secondary allocator
//
struct SecondaryAllocation
{
    BYTE* address;   // address of the allocation by the secondary allocator

    SecondaryAllocation() : address(nullptr)
    {
    }
};

#if defined(_M_X64)
struct XDataInfo
{
    RUNTIME_FUNCTION pdata;
    FunctionTableHandle functionTable;
};
#elif defined(_M_ARM32_OR_ARM64)
struct XDataInfo
{
    ushort pdataCount;
    ushort xdataSize;
    FunctionTableHandle functionTable;
};
#endif

//
// For every page segment a page allocator can create a secondary allocator which can have a specified
// number of pages reserved for secondary allocations. These pages are always reserved at the end of the
// segment. The PageAllocator itself cannot allocate from the region demarcated for the secondary allocator.
// Currently this is used for xdata allocations.
//
class SecondaryAllocator
{
public:
    virtual bool Alloc(ULONG_PTR functionStart, DWORD functionSize, DECLSPEC_GUARD_OVERFLOW ushort pdataCount, DECLSPEC_GUARD_OVERFLOW ushort xdataSize, SecondaryAllocation* xdata) = 0;
    virtual void Release(const SecondaryAllocation& allocation) = 0;
    virtual void Delete() = 0;
    virtual bool CanAllocate() = 0;
    virtual ~SecondaryAllocator() {};
};

class PageAllocatorBaseCommon;

class SegmentBaseCommon
{
protected:
    PageAllocatorBaseCommon* allocator;

public:
    SegmentBaseCommon(PageAllocatorBaseCommon* allocator);
    virtual ~SegmentBaseCommon() {}
    bool IsInPreReservedHeapPageAllocator() const;
};

/*
 * A segment is a collection of pages. A page corresponds to the concept of an
 * OS memory page. Segments allocate memory using the OS VirtualAlloc call.
 * It'll allocate the pageCount * page size number of bytes, the latter being
 * a system-wide constant.
 */
template<typename TVirtualAlloc>
class SegmentBase: public SegmentBaseCommon
{
public:
    SegmentBase(PageAllocatorBase<TVirtualAlloc> * allocator, DECLSPEC_GUARD_OVERFLOW size_t pageCount, bool enableWriteBarrier);
    virtual ~SegmentBase();

    size_t GetPageCount() const { return segmentPageCount; }

    // Some pages are reserved upfront for secondary allocations
    // which are done by a secondary allocator as opposed to the PageAllocator
    size_t GetAvailablePageCount() const { return segmentPageCount - secondaryAllocPageCount; }

    char* GetSecondaryAllocStartAddress() const { return (this->address + GetAvailablePageCount() * AutoSystemInfo::PageSize); }
    uint GetSecondaryAllocSize() const { return this->secondaryAllocPageCount * AutoSystemInfo::PageSize; }

    char* GetAddress() const { return address; }
    char* GetEndAddress() const { return GetSecondaryAllocStartAddress(); }

    bool CanAllocSecondary() { Assert(secondaryAllocator); return secondaryAllocator->CanAllocate(); }

    PageAllocatorBase<TVirtualAlloc>* GetAllocator() const
    {
        return static_cast<PageAllocatorBase<TVirtualAlloc>*>(allocator);
    }

    bool Initialize(DWORD allocFlags, bool excludeGuardPages);

#if DBG
    virtual bool IsPageSegment() const
    {
        return false;
    }
#endif

    bool IsInSegment(void* address) const
    {
        void* start = static_cast<void*>(GetAddress());
        void* end = static_cast<void*>(GetEndAddress());
        return (address >= start && address < end);
    }

    bool IsInCustomHeapAllocator() const
    {
        return this->GetAllocator()->type == PageAllocatorType::PageAllocatorType_CustomHeap;
    }

    SecondaryAllocator* GetSecondaryAllocator() { return secondaryAllocator; }

#if defined(_M_X64_OR_ARM64) && defined(RECYCLER_WRITE_BARRIER)
    bool IsWriteBarrierAllowed()
    {
        return isWriteBarrierAllowed;
    }
    bool IsWriteBarrierEnabled()
    {
        return this->isWriteBarrierEnabled;
    }
#endif

protected:
#if _M_IX86_OR_ARM32
    static const uint VirtualAllocThreshold =  524288; // 512kb As per spec
#else // _M_X64_OR_ARM64
    static const uint VirtualAllocThreshold = 1048576; // 1MB As per spec : when we cross this threshold of bytes, we should add guard pages
#endif
    static const uint maxGuardPages = 15;
    static const uint minGuardPages =  1;

    SecondaryAllocator* secondaryAllocator;
    char * address;
    size_t segmentPageCount;
    uint trailingGuardPageCount;
    uint leadingGuardPageCount;
    uint   secondaryAllocPageCount;

    bool   isWriteBarrierAllowed : 1;
    bool   isWriteBarrierEnabled : 1;
};

/*
 * Page Segments allows a client to deal with virtual memory on a page level
 * unlike Segment, which gives you access on a segment basis. Pages managed
 * by the page segment are initially in a "free list", and have the no access
 * bit set on them. When a client wants pages, we get them from the free list
 * and commit them into memory. When the client no longer needs those pages,
 * we simply decommit them- this means that the pages are still reserved for
 * the process but are not a part of its working set and has no physical
 * storage associated with it.
 */

template<typename TVirtualAlloc>
class PageSegmentBase : public SegmentBase<TVirtualAlloc>
{
    typedef SegmentBase<TVirtualAlloc> Base;
public:
    PageSegmentBase(PageAllocatorBase<TVirtualAlloc> * allocator, bool committed, bool allocated, bool enableWriteBarrier);
    PageSegmentBase(PageAllocatorBase<TVirtualAlloc> * allocator, void* address, uint pageCount, uint committedCount, bool enableWriteBarrier);
    // Maximum possible size of a PageSegment; may be smaller.
    static const uint MaxDataPageCount = 256;     // 1 MB
    static const uint MaxGuardPageCount = 16;
    static const uint MaxPageCount = MaxDataPageCount + MaxGuardPageCount;  // 272 Pages

    typedef BVStatic<MaxPageCount> PageBitVector;

    uint GetAvailablePageCount() const
    {
        size_t availablePageCount = Base::GetAvailablePageCount();
        Assert(availablePageCount < MAXUINT32);
        return static_cast<uint>(availablePageCount);
    }

    void Prime();
#ifdef PAGEALLOCATOR_PROTECT_FREEPAGE
    bool Initialize(DWORD allocFlags, bool excludeGuardPages);
#endif
    uint GetFreePageCount() const { return freePageCount; }
    uint GetDecommitPageCount() const { return decommitPageCount; }

    static bool IsAllocationPageAligned(__in char* address, size_t pageCount, uint *nextIndex = nullptr);

    template <typename T, bool notPageAligned>
    char * AllocDecommitPages(DECLSPEC_GUARD_OVERFLOW uint pageCount, T freePages, T decommitPages);

    template <bool notPageAligned>
    char * AllocPages(DECLSPEC_GUARD_OVERFLOW uint pageCount);

    void ReleasePages(__in void * address, uint pageCount);
    template <bool onlyUpdateState>
    void DecommitPages(__in void * address, uint pageCount);

    uint GetCountOfFreePages() const;
    uint GetNextBitInFreePagesBitVector(uint index) const;
    BOOLEAN TestRangeInFreePagesBitVector(uint index, uint pageCount) const;
    BOOLEAN TestInFreePagesBitVector(uint index) const;
    void ClearAllInFreePagesBitVector();
    void ClearRangeInFreePagesBitVector(uint index, uint pageCount);
    void SetRangeInFreePagesBitVector(uint index, uint pageCount);
    void ClearBitInFreePagesBitVector(uint index);

    uint GetCountOfDecommitPages() const;
    BOOLEAN TestInDecommitPagesBitVector(uint index) const;
    BOOLEAN TestRangeInDecommitPagesBitVector(uint index, uint pageCount) const;
    void SetRangeInDecommitPagesBitVector(uint index, uint pageCount);
    void SetBitInDecommitPagesBitVector(uint index);
    void ClearRangeInDecommitPagesBitVector(uint index, uint pageCount);

    template <bool notPageAligned>
    char * DoAllocDecommitPages(DECLSPEC_GUARD_OVERFLOW uint pageCount);
    uint GetMaxPageCount();

    size_t DecommitFreePages(size_t pageToDecommit);

    bool IsEmpty() const
    {
        return this->freePageCount == this->GetAvailablePageCount();
    }

    //
    // If a segment has decommitted pages - then it's not considered full as allocations can take place from it
    // However, if secondary allocations cannot be made from it - it's considered full nonetheless
    //
    bool IsFull() const
    {
        return (this->freePageCount == 0 && !ShouldBeInDecommittedList()) ||
            (this->secondaryAllocator != nullptr && !this->secondaryAllocator->CanAllocate());
    }

    bool IsAllDecommitted() const
    {
        return this->GetAvailablePageCount() == this->decommitPageCount;
    }

    bool ShouldBeInDecommittedList() const
    {
        return this->decommitPageCount != 0;
    }

    bool IsFreeOrDecommitted(void* address, uint pageCount) const
    {
        Assert(this->IsInSegment(address));

        uint base = GetBitRangeBase(address);
        return this->TestRangeInDecommitPagesBitVector(base, pageCount) || this->TestRangeInFreePagesBitVector(base, pageCount);
    }

    bool IsFreeOrDecommitted(void* address) const
    {
        Assert(this->IsInSegment(address));

        uint base = GetBitRangeBase(address);
        return this->TestInDecommitPagesBitVector(base) || this->TestInFreePagesBitVector(base);
    }

    PageBitVector GetUnAllocatedPages() const
    {
        PageBitVector unallocPages = freePages;
        unallocPages.Or(&decommitPages);
        return unallocPages;
    }

    void ChangeSegmentProtection(DWORD protectFlags, DWORD expectedOldProtectFlags);

#if DBG
    bool IsPageSegment() const override
    {
        return true;
    }
#endif

//---------- Private members ---------------/
private:
    void DecommitFreePagesInternal(uint index, uint pageCount);

    uint GetBitRangeBase(void* address) const
    {
        uint base = ((uint)(((char *)address) - this->address)) / AutoSystemInfo::PageSize;
        return base;
    }

    PageBitVector freePages;
    PageBitVector decommitPages;

    uint     freePageCount;
    uint     decommitPageCount;
};

template<typename TVirtualAlloc = VirtualAllocWrapper>
class HeapPageAllocator;

/*
 * A Page Allocation is an allocation made by a page allocator
 * This has a base address, and tracks the number of pages that
 * were allocated from the segment
 */
class PageAllocation
{
public:
    char * GetAddress() const { return ((char *)this) + sizeof(PageAllocation); }
    size_t GetSize() const { return pageCount * AutoSystemInfo::PageSize - sizeof(PageAllocation); }
    size_t GetPageCount() const { return pageCount; }
    void* GetSegment() const { return segment; }

private:
    size_t pageCount;
    void * segment;

    friend class PageAllocatorBase<VirtualAllocWrapper>;
    friend class PageAllocatorBase<PreReservedVirtualAllocWrapper>;
#if ENABLE_OOP_NATIVE_CODEGEN
    friend class PageAllocatorBase<SectionAllocWrapper>;
    friend class PageAllocatorBase<PreReservedSectionAllocWrapper>;
#endif
    friend class HeapPageAllocator<>;
};

class MemoryOperationLastError
{
public:
    static void RecordLastError()
    {
#if ENABLE_OOP_NATIVE_CODEGEN
        if (MemOpLastError == S_OK)
        {
            MemOpLastError = HRESULT_FROM_WIN32(::GetLastError());
        }
#endif
    }

    static void RecordError(HRESULT error)
    {
#if ENABLE_OOP_NATIVE_CODEGEN
        if (MemOpLastError == S_OK)
        {
            MemOpLastError = error;
        }
#endif
    }

    static void ClearLastError()
    {
#if ENABLE_OOP_NATIVE_CODEGEN
        MemOpLastError = S_OK;
#endif
    }
    static HRESULT GetLastError()
    {
#if ENABLE_OOP_NATIVE_CODEGEN
        return MemOpLastError;
#else
        return S_OK;
#endif
    }
#if ENABLE_OOP_NATIVE_CODEGEN
private:
    THREAD_LOCAL static HRESULT MemOpLastError;
#endif
};

class PageAllocatorBaseCommon
{
public:
    enum class AllocatorType
    {
        VirtualAlloc,
        PreReservedVirtualAlloc,
#if ENABLE_OOP_NATIVE_CODEGEN
        SectionAlloc,
        PreReservedSectionAlloc
#endif
    };

    template<typename TVirtualAlloc>
    static AllocatorType GetAllocatorType();

    AllocatorType GetAllocatorType() const { return this->allocatorType; }

protected:
    void* virtualAllocator;
    AllocatorType allocatorType;
public:

    PageAllocatorBaseCommon() :
        virtualAllocator(nullptr),
        allocatorType(AllocatorType::VirtualAlloc)
    {}
    virtual ~PageAllocatorBaseCommon() {}
};
template<> inline PageAllocatorBaseCommon::AllocatorType PageAllocatorBaseCommon::GetAllocatorType<VirtualAllocWrapper>() { return AllocatorType::VirtualAlloc; };
template<> inline PageAllocatorBaseCommon::AllocatorType PageAllocatorBaseCommon::GetAllocatorType<PreReservedVirtualAllocWrapper>() { return AllocatorType::PreReservedVirtualAlloc; };
#if ENABLE_OOP_NATIVE_CODEGEN
template<> inline PageAllocatorBaseCommon::AllocatorType PageAllocatorBaseCommon::GetAllocatorType<SectionAllocWrapper>() { return AllocatorType::SectionAlloc; };
template<> inline PageAllocatorBaseCommon::AllocatorType PageAllocatorBaseCommon::GetAllocatorType<PreReservedSectionAllocWrapper>() { return AllocatorType::PreReservedSectionAlloc; };
#endif

/*
 * This allocator is responsible for allocating and freeing pages. It does
 * so by virtue of allocating segments for groups of pages, and then handing
 * out memory in these segments. It's also responsible for free segments.
 * This class also controls the idle decommit thread, which decommits pages
 * when they're no longer needed
 */

template<typename TVirtualAlloc, typename TSegment, typename TPageSegment>
class PageAllocatorBase: public PageAllocatorBaseCommon
{
    friend class ::CodeGenNumberThreadAllocator;
    friend struct ::XProcNumberPageSegmentManager;
    // Allowing recycler to report external memory allocation.
    friend class Recycler;
public:
    static uint const DefaultMaxFreePageCount = 0x400;       // 4 MB
    static uint const DefaultLowMaxFreePageCount = 0x100;    // 1 MB for low-memory process

    static uint const MinPartialDecommitFreePageCount = 0x1000;  // 16 MB

    static uint const DefaultMaxAllocPageCount = 32;        // 128K
    static uint const DefaultSecondaryAllocPageCount = 0;

    static size_t GetProcessUsedBytes();

    static size_t GetAndResetMaxUsedBytes();

    // xplat TODO: implement a platform agnostic version of interlocked linked lists
#if ENABLE_BACKGROUND_PAGE_FREEING
    struct FreePageEntry
#if SUPPORT_WIN32_SLIST
        : public SLIST_ENTRY
#endif
    {
#if !SUPPORT_WIN32_SLIST
        FreePageEntry* Next;
#endif
        PageSegmentBase<TVirtualAlloc> * segment;
        uint pageCount;
    };
    struct BackgroundPageQueue
    {
#if SUPPORT_WIN32_SLIST
        SLIST_HEADER bgFreePageList;
#else
        FreePageEntry* bgFreePageList;
#endif
        CriticalSection backgroundPageQueueCriticalSection;

#if DBG
        bool isZeroPageQueue;
#endif

        BackgroundPageQueue()
#if !SUPPORT_WIN32_SLIST
            :bgFreePageList(nullptr)
#endif
        {
#if SUPPORT_WIN32_SLIST
            ::InitializeSListHead(&bgFreePageList);
#endif
            DebugOnly(this->isZeroPageQueue = false);
        }

        FreePageEntry* PopFreePageEntry()
        {
#if SUPPORT_WIN32_SLIST
            return (FreePageEntry *)::InterlockedPopEntrySList(&bgFreePageList);
#else
            AutoCriticalSection autoCS(&backgroundPageQueueCriticalSection);
            FreePageEntry* head = bgFreePageList;
            if (head)
            {
                bgFreePageList = bgFreePageList->Next;
            }
            return head;
#endif
        }

        void PushFreePageEntry(FreePageEntry* entry)
        {
#if SUPPORT_WIN32_SLIST
            ::InterlockedPushEntrySList(&bgFreePageList, entry);
#else
            AutoCriticalSection autoCS(&backgroundPageQueueCriticalSection);
            entry->Next = bgFreePageList;
            bgFreePageList = entry;
#endif
        }
    };

#if ENABLE_BACKGROUND_PAGE_ZEROING
    struct ZeroPageQueue : BackgroundPageQueue
    {
#if SUPPORT_WIN32_SLIST
        SLIST_HEADER pendingZeroPageList;
#else
        FreePageEntry* pendingZeroPageList;
#endif

        ZeroPageQueue()
#if !SUPPORT_WIN32_SLIST
            :BackgroundPageQueue(), pendingZeroPageList(nullptr)
#endif
        {
#if SUPPORT_WIN32_SLIST
            ::InitializeSListHead(&pendingZeroPageList);
#endif
            DebugOnly(this->isZeroPageQueue = true);
        }

        FreePageEntry* PopZeroPageEntry()
        {
#if SUPPORT_WIN32_SLIST
            return (FreePageEntry *)::InterlockedPopEntrySList(&pendingZeroPageList);
#else
            AutoCriticalSection autoCS(&this->backgroundPageQueueCriticalSection);
            FreePageEntry* head = pendingZeroPageList;
            if (head)
            {
                pendingZeroPageList = pendingZeroPageList->Next;
            }
            return head;
#endif
        }

        void PushZeroPageEntry(FreePageEntry* entry)
        {
#if SUPPORT_WIN32_SLIST
            ::InterlockedPushEntrySList(&pendingZeroPageList, entry);
#else
            AutoCriticalSection autoCS(&this->backgroundPageQueueCriticalSection);
            entry->Next = pendingZeroPageList;
            pendingZeroPageList = entry;
#endif
        }

        USHORT QueryDepth()
        {
#if SUPPORT_WIN32_SLIST
            return QueryDepthSList(&pendingZeroPageList);
#else
            AutoCriticalSection autoCS(&this->backgroundPageQueueCriticalSection);
            FreePageEntry* head = pendingZeroPageList;
            size_t count = 0;
            while (head)
            {
                head = head->Next;
                count++;
            }
            // If the specified singly linked list contains more than 65535 entries, QueryDepthSList returns the number of entries in the list modulo 65535
            return (USHORT)(count % 65536);
#endif
        }
    };
#endif
#endif

    PageAllocatorBase(AllocationPolicyManager * policyManager,
        Js::ConfigFlagsTable& flags = Js::Configuration::Global.flags,
        PageAllocatorType type = PageAllocatorType_Max,
        uint maxFreePageCount = DefaultMaxFreePageCount,
        bool zeroPages = false,
#if ENABLE_BACKGROUND_PAGE_FREEING
        BackgroundPageQueue * backgroundPageQueue = nullptr,
#endif
        uint maxAllocPageCount = DefaultMaxAllocPageCount,
        uint secondaryAllocPageCount = DefaultSecondaryAllocPageCount,
        bool stopAllocationOnOutOfMemory = false,
        bool excludeGuardPages = false,
        HANDLE processHandle = GetCurrentProcess(),
        bool enableWriteBarrier = false
        );

    virtual ~PageAllocatorBase();

    bool IsClosed() const { return isClosed; }
    void Close() { Assert(!isClosed); isClosed = true; }

    AllocationPolicyManager * GetAllocationPolicyManager() { return policyManager; }

    uint GetMaxAllocPageCount();

    //VirtualAllocator APIs
    TVirtualAlloc * GetVirtualAllocator() const;

    PageAllocation * AllocPagesForBytes(DECLSPEC_GUARD_OVERFLOW size_t requestedBytes);
    PageAllocation * AllocAllocation(DECLSPEC_GUARD_OVERFLOW size_t pageCount);

    void ReleaseAllocation(PageAllocation * allocation);
    void ReleaseAllocationNoSuspend(PageAllocation * allocation);

    char * Alloc(size_t * pageCount, TSegment ** segment);

    void Release(void * address, size_t pageCount, void * segment);

    char * AllocPages(DECLSPEC_GUARD_OVERFLOW uint pageCount, TPageSegment ** pageSegment);
    char * AllocPagesPageAligned(DECLSPEC_GUARD_OVERFLOW uint pageCount, TPageSegment ** pageSegment);

    void ReleasePages(__in void * address, uint pageCount, __in void * pageSegment);
#if ENABLE_BACKGROUND_PAGE_FREEING
    void BackgroundReleasePages(void * address, uint pageCount, TPageSegment * pageSegment);
#endif
    void MemSetLocal(_In_ void *dst, int val, size_t sizeInBytes);

    // Decommit
    void DecommitNow(bool all = true);
    void SuspendIdleDecommit();
    void ResumeIdleDecommit();

#if ENABLE_BACKGROUND_PAGE_ZEROING
    void StartQueueZeroPage();
    void StopQueueZeroPage();
    void ZeroQueuedPages();
    void BackgroundZeroQueuedPages();
#endif
#if ENABLE_BACKGROUND_PAGE_FREEING
    void FlushBackgroundPages();
#endif

    bool DisableAllocationOutOfMemory() const { return disableAllocationOutOfMemory; }
    void ResetDisableAllocationOutOfMemory() { disableAllocationOutOfMemory = false; }

#ifdef RECYCLER_MEMORY_VERIFY
    void EnableVerify() { verifyEnabled = true; }
#endif
#if defined(RECYCLER_NO_PAGE_REUSE) || defined(ARENA_MEMORY_VERIFY)
    void ReenablePageReuse() { Assert(disablePageReuse); disablePageReuse = false; }
    bool DisablePageReuse() { bool wasDisablePageReuse = disablePageReuse; disablePageReuse = true; return wasDisablePageReuse; }
    bool IsPageReuseDisabled() { return disablePageReuse; }
#endif

#if DBG
#if ENABLE_BACKGROUND_PAGE_ZEROING
    bool HasZeroQueuedPages() const;
#endif
    virtual void SetDisableThreadAccessCheck() { disableThreadAccessCheck = true;}
    virtual void SetEnableThreadAccessCheck() { disableThreadAccessCheck = false; }

    virtual bool IsIdleDecommitPageAllocator() const { return false; }
    virtual bool HasMultiThreadAccess() const { return false; }
    bool ValidThreadAccess()
    {
        DWORD currentThreadId = ::GetCurrentThreadId();
        return disableThreadAccessCheck ||
            (this->concurrentThreadId == -1 && this->threadContextHandle == NULL) ||  // JIT thread after close
            (this->concurrentThreadId != -1 && this->concurrentThreadId == currentThreadId) ||
            this->threadContextHandle == GetCurrentThreadContextId();
    }
    virtual void UpdateThreadContextHandle(ThreadContextId updatedThreadContextHandle) { threadContextHandle = updatedThreadContextHandle; }
    void SetConcurrentThreadId(DWORD threadId) { this->concurrentThreadId = threadId; }
    void ClearConcurrentThreadId() { this->concurrentThreadId = (DWORD)-1; }
    DWORD GetConcurrentThreadId() { return this->concurrentThreadId;  }
    DWORD HasConcurrentThreadId() { return this->concurrentThreadId != -1; }
#endif

    bool IsWriteWatchEnabled()
    {
        return (allocFlags & MEM_WRITE_WATCH) == MEM_WRITE_WATCH;
    }

#if DBG_DUMP
    char16 const * debugName;
#endif
protected:
    void InitVirtualAllocator(TVirtualAlloc * virtualAllocator);

    TSegment * AllocSegment(DECLSPEC_GUARD_OVERFLOW size_t pageCount);
    void ReleaseSegment(TSegment * segment);

    template <bool doPageAlign>
    char * AllocInternal(size_t * pageCount, TSegment ** segment);

    template <bool notPageAligned>
    char * SnailAllocPages(DECLSPEC_GUARD_OVERFLOW uint pageCount, TPageSegment ** pageSegment);
    void OnAllocFromNewSegment(DECLSPEC_GUARD_OVERFLOW uint pageCount, __in void* pages, TSegment* segment);

    template <bool notPageAligned>
    char * TryAllocFreePages(DECLSPEC_GUARD_OVERFLOW uint pageCount, TPageSegment ** pageSegment);
#if ENABLE_BACKGROUND_PAGE_FREEING
    char * TryAllocFromZeroPagesList(DECLSPEC_GUARD_OVERFLOW uint pageCount, TPageSegment ** pageSegment, BackgroundPageQueue* bgPageQueue, bool isPendingZeroList);
#endif
    char * TryAllocFromZeroPages(DECLSPEC_GUARD_OVERFLOW uint pageCount, TPageSegment ** pageSegment);

    template <bool notPageAligned>
    char * TryAllocDecommittedPages(DECLSPEC_GUARD_OVERFLOW uint pageCount, TPageSegment ** pageSegment);

    DListBase<TPageSegment> * GetSegmentList(TPageSegment * segment);
    void TransferSegment(TPageSegment * segment, DListBase<TPageSegment> * fromSegmentList);

    void FillAllocPages(__in void * address, uint pageCount);
    void FillFreePages(__in void * address, uint pageCount);

    bool IsPageSegment(TSegment* segment)
    {
        return segment->GetAvailablePageCount() <= maxAllocPageCount;
    }

#if DBG_DUMP
    virtual void DumpStats() const;
#endif
    TPageSegment * AddPageSegment(DListBase<TPageSegment>& segmentList);
    static TPageSegment * AllocPageSegment(
        DListBase<TPageSegment>& segmentList,
        PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment> * pageAllocator,
        void* address, uint pageCount, uint committedCount, bool enableWriteBarrier);
    static TPageSegment * AllocPageSegment(
        DListBase<TPageSegment>& segmentList,
        PageAllocatorBase<TVirtualAlloc, TSegment, TPageSegment> * pageAllocator,
        bool committed, bool allocated, bool enableWriteBarrier);

    // Zero Pages
#if ENABLE_BACKGROUND_PAGE_ZEROING
    void AddPageToZeroQueue(__in void * address, uint pageCount, __in TPageSegment * pageSegment);
    bool HasZeroPageQueue() const;
#endif

    bool ZeroPages() const { return zeroPages; }
#if ENABLE_BACKGROUND_PAGE_ZEROING
    bool QueueZeroPages() const { return queueZeroPages; }
#endif

#if ENABLE_BACKGROUND_PAGE_FREEING
    FreePageEntry * PopPendingZeroPage();
#endif

#if DBG
    void Check();
    bool disableThreadAccessCheck;
#endif

protected:
    // Data
    DListBase<TPageSegment> segments;
    DListBase<TPageSegment> fullSegments;
    DListBase<TPageSegment> emptySegments;
    DListBase<TPageSegment> decommitSegments;

    DListBase<TSegment> largeSegments;

    uint maxAllocPageCount;
    DWORD allocFlags;
    uint maxFreePageCount;
    size_t freePageCount;
    uint secondaryAllocPageCount;
    bool isClosed;
    bool stopAllocationOnOutOfMemory;
    bool disableAllocationOutOfMemory;
    bool excludeGuardPages;
    bool enableWriteBarrier;
    AllocationPolicyManager * policyManager;

    Js::ConfigFlagsTable& pageAllocatorFlagTable;

    // zero pages
    bool zeroPages;
#if ENABLE_BACKGROUND_PAGE_FREEING
    BackgroundPageQueue * backgroundPageQueue;
#if ENABLE_BACKGROUND_PAGE_ZEROING
    bool queueZeroPages;
    bool hasZeroQueuedPages;
#endif
#endif

    // Idle Decommit
    bool isUsed;
    size_t minFreePageCount;
    uint idleDecommitEnterCount;

    void UpdateMinFreePageCount();
    void ResetMinFreePageCount();
    void ClearMinFreePageCount();
    void AddFreePageCount(uint pageCount);

    static uint GetFreePageLimit() { return 0; }

#if DBG
    size_t debugMinFreePageCount;
    ThreadContextId threadContextHandle;
    DWORD concurrentThreadId;
#endif
#if DBG_DUMP
    size_t decommitPageCount;
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    bool verifyEnabled;
#endif
#if defined(RECYCLER_NO_PAGE_REUSE) || defined(ARENA_MEMORY_VERIFY)
    bool disablePageReuse;
#endif

    friend TSegment;
    friend TPageSegment;
    friend class IdleDecommit;

protected:
    virtual bool CreateSecondaryAllocator(TSegment* segment, bool committed, SecondaryAllocator** allocator)
    {
        *allocator = nullptr;
        return true;
    }

    bool IsAddressInSegment(__in void* address, const TPageSegment& segment);
    bool IsAddressInSegment(__in void* address, const TSegment& segment);

    HANDLE processHandle;
private:
    uint GetSecondaryAllocPageCount() const { return this->secondaryAllocPageCount; }
    void IntegrateSegments(DListBase<TPageSegment>& segmentList, uint segmentCount, size_t pageCount);
#if ENABLE_BACKGROUND_PAGE_FREEING
    void QueuePages(void * address, uint pageCount, TPageSegment * pageSegment);
#endif

    template <bool notPageAligned>
    char* AllocPagesInternal(DECLSPEC_GUARD_OVERFLOW uint pageCount, TPageSegment ** pageSegment);

#ifdef PROFILE_MEM
    PageMemoryData * memoryData;
#endif

    size_t usedBytes;
    PageAllocatorType type;

    size_t reservedBytes;
    size_t committedBytes;
    size_t numberOfSegments;

#ifdef PERF_COUNTERS
    PerfCounter::Counter& GetReservedSizeCounter() const
    {
        return PerfCounter::PageAllocatorCounterSet::GetReservedSizeCounter(type);
    }
    PerfCounter::Counter& GetCommittedSizeCounter() const
    {
        return PerfCounter::PageAllocatorCounterSet::GetCommittedSizeCounter(type);
    }
    PerfCounter::Counter& GetUsedSizeCounter() const
    {
        return PerfCounter::PageAllocatorCounterSet::GetUsedSizeCounter(type);
    }
    PerfCounter::Counter& GetTotalReservedSizeCounter() const
    {
        return PerfCounter::PageAllocatorCounterSet::GetTotalReservedSizeCounter();
    }
    PerfCounter::Counter& GetTotalCommittedSizeCounter() const
    {
        return PerfCounter::PageAllocatorCounterSet::GetTotalCommittedSizeCounter();
    }
    PerfCounter::Counter& GetTotalUsedSizeCounter() const
    {
        return PerfCounter::PageAllocatorCounterSet::GetTotalUsedSizeCounter();
    }
#endif
    void AddReservedBytes(size_t bytes);
    void SubReservedBytes(size_t bytes);
    void AddCommittedBytes(size_t bytes);
    void SubCommittedBytes(size_t bytes);
    void AddUsedBytes(size_t bytes);
    void SubUsedBytes(size_t bytes);
    void AddNumberOfSegments(size_t segmentCount);
    void SubNumberOfSegments(size_t segmentCount);

    bool RequestAlloc(size_t byteCount)
    {
        if (disableAllocationOutOfMemory)
        {
            return false;
        }

        if (policyManager != nullptr)
        {
            return policyManager->RequestAlloc(byteCount);
        }

        return true;
    }

    void ReportExternalAlloc(size_t byteCount)
    {
        if (policyManager != nullptr)
        {
            policyManager->RequestAlloc(byteCount, true);
        }
    }

    void ReportFree(size_t byteCount)
    {
        if (policyManager != nullptr)
        {
            policyManager->ReportFree(byteCount);
        }
    }

    template <typename T>
    void ReleaseSegmentList(DListBase<T> * segmentList);

protected:
    // Instrumentation
    void LogAllocSegment(TSegment * segment);
    void LogAllocSegment(uint segmentCount, size_t pageCount);
    void LogFreeSegment(TSegment * segment);
    void LogFreeDecommittedSegment(TSegment * segment);
    void LogFreePartiallyDecommittedPageSegment(TPageSegment * pageSegment);

    void LogAllocPages(size_t pageCount);
    void LogFreePages(size_t pageCount);
    void LogCommitPages(size_t pageCount);
    void LogRecommitPages(size_t pageCount);
    void LogDecommitPages(size_t pageCount);

    void ReportFailure(size_t byteCount)
    {
        if (this->stopAllocationOnOutOfMemory)
        {
            this->disableAllocationOutOfMemory = true;
        }

        if (policyManager != nullptr)
        {
            policyManager->ReportFailure(byteCount);
        }
    }
};

template<typename TVirtualAlloc>
class HeapPageAllocator : public PageAllocatorBase<TVirtualAlloc>
{
    typedef PageAllocatorBase<TVirtualAlloc> Base;
public:
    HeapPageAllocator(AllocationPolicyManager * policyManager, bool allocXdata, bool excludeGuardPages, TVirtualAlloc * virtualAllocator, HANDLE processHandle = nullptr);

    BOOL ProtectPages(__in char* address, size_t pageCount, __in void* segment, DWORD dwVirtualProtectFlags, DWORD desiredOldProtectFlag);
    bool AllocSecondary(void* segment, ULONG_PTR functionStart, DWORD functionSize, DECLSPEC_GUARD_OVERFLOW ushort pdataCount, DECLSPEC_GUARD_OVERFLOW ushort xdataSize, SecondaryAllocation* allocation);
    bool ReleaseSecondary(const SecondaryAllocation& allocation, void* segment);
    void TrackDecommittedPages(void * address, uint pageCount, __in void* segment);
    void DecommitPages(__in char* address, size_t pageCount = 1);

    // Release pages that has already been decommitted
    void    ReleaseDecommitted(void * address, size_t pageCount, __in void * segment);
    bool IsAddressFromAllocator(__in void* address);
    bool    AllocXdata() { return allocXdata; }

private:
    bool         allocXdata;
    void         ReleaseDecommittedSegment(__in SegmentBase<TVirtualAlloc>* segment);
#if PDATA_ENABLED
    virtual bool CreateSecondaryAllocator(SegmentBase<TVirtualAlloc>* segment, bool committed, SecondaryAllocator** allocator) override;
#endif

};

}
