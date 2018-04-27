//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

namespace Memory
{
template <class Fn>
bool HeapInfoManager::AreAllHeapInfo(Fn fn)
{
    return fn(defaultHeap);
}

template <class Fn>
bool HeapInfoManager::IsAnyHeapInfo(Fn fn)
{
    return fn(defaultHeap);
}

template <class Fn>
void HeapInfoManager::ForEachHeapInfo(Fn fn)
{
    fn(defaultHeap);
}

template <class Fn>
void HeapInfoManager::ForEachHeapInfo(RecyclerSweepManager& recyclerSweepManager, Fn fn)
{
    fn(defaultHeap, recyclerSweepManager.defaultHeapRecyclerSweep);
}

template <class Fn>
void HeapInfoManager::ForEachHeapInfo(RecyclerSweepManager * recyclerSweepManager, Fn fn)
{
    fn(defaultHeap, recyclerSweepManager? &recyclerSweepManager->defaultHeapRecyclerSweep : nullptr);
}

HeapInfoManager::HeapInfoManager(AllocationPolicyManager * policyManager, Js::ConfigFlagsTable& configFlagsTable, IdleDecommitPageAllocator * leafPageAllocator) :
    defaultHeap(policyManager, configFlagsTable, leafPageAllocator),
#if ENABLE_PARTIAL_GC
    uncollectedNewPageCount(0),
    unusedPartialCollectFreeBytes(0),
#endif
    uncollectedAllocBytes(0),
    lastUncollectedAllocBytes(0),
    pendingZeroPageCount(0)
{
}

void HeapInfoManager::Initialize(Recycler * recycler
#ifdef RECYCLER_PAGE_HEAP
    , PageHeapMode pageheapmode
    , bool captureAllocCallStack
    , bool captureFreeCallStack
#endif
)
{
    ForEachHeapInfo([=](HeapInfo& heapInfo)
    {
        heapInfo.Initialize(recycler
#ifdef RECYCLER_PAGE_HEAP
            , pageheapmode
            , captureAllocCallStack
            , captureFreeCallStack
#endif
        );
    });
}

#if defined(PROFILE_RECYCLER_ALLOC) || defined(RECYCLER_MEMORY_VERIFY) || defined(MEMSPECT_TRACKING) || defined(ETW_MEMORY_TRACKING)
void HeapInfoManager::Initialize(Recycler * recycler, void(*trackNativeAllocCallBack)(Recycler *, void *, size_t)
#ifdef RECYCLER_PAGE_HEAP
    , PageHeapMode pageheapmode
    , bool captureAllocCallStack
    , bool captureFreeCallStack
#endif
)
{
    ForEachHeapInfo([=](HeapInfo& heapInfo)
    {
        heapInfo.Initialize(recycler, trackNativeAllocCallBack
#ifdef RECYCLER_PAGE_HEAP
            , pageheapmode
            , captureAllocCallStack
            , captureFreeCallStack
#endif
        );
    });
}
#endif

void
HeapInfoManager::ScanInitialImplicitRoots()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.ScanInitialImplicitRoots();
    });
}

void
HeapInfoManager::ScanNewImplicitRoots()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.ScanNewImplicitRoots();
    });
}

void
HeapInfoManager::ResetMarks(ResetMarkFlags flags)
{
    ForEachHeapInfo([=](HeapInfo& heapInfo)
    {
        heapInfo.ResetMarks(flags);
    });
}


size_t
HeapInfoManager::Rescan(RescanFlags flags)
{
    size_t rescannedPageCount = 0;
    ForEachHeapInfo([&](HeapInfo& heapInfo)
    {
        rescannedPageCount += heapInfo.Rescan(flags);
    });
    return rescannedPageCount;
}

void
HeapInfoManager::FinalizeAndSweep(RecyclerSweepManager& recyclerSweepManager, bool concurrent)
{
    SuspendIdleDecommitNonLeaf();

    // Call finalize before sweeping so that the finalizer can still access object it referenced
    ForEachHeapInfo(recyclerSweepManager, [&](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.Finalize(recyclerSweep);
    });
    ForEachHeapInfo(recyclerSweepManager, [&](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.Sweep(recyclerSweep, concurrent);
    });

    ResumeIdleDecommitNonLeaf();
}

void 
HeapInfoManager::SweepSmallNonFinalizable(RecyclerSweepManager& recyclerSweepManager)
{
    ForEachHeapInfo(recyclerSweepManager, [&](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.SweepSmallNonFinalizable(recyclerSweep);
    });
}

#if ENABLE_PARTIAL_GC || ENABLE_CONCURRENT_GC
#ifdef RECYCLER_WRITE_WATCH 
void HeapInfoManager::EnableWriteWatch()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.EnableWriteWatch();
    });
}

bool HeapInfoManager::ResetWriteWatch()
{
    return AreAllHeapInfo([](HeapInfo& heapInfo)
    {
        return heapInfo.ResetWriteWatch();
    });
}

#if DBG
size_t HeapInfoManager::GetWriteWatchPageCount()
{
    size_t writeWatchPageCount = 0;
    ForEachHeapInfo([&](HeapInfo& heapInfo)
    {
        writeWatchPageCount += heapInfo.GetWriteWatchPageCount();
    });
    return writeWatchPageCount;
}
#endif
#endif
void
HeapInfoManager::SweepPendingObjects(RecyclerSweepManager& recyclerSweepManager)
{
    ForEachHeapInfo(recyclerSweepManager, [&](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.SweepPendingObjects(recyclerSweep);
    });
}
#endif

#if ENABLE_PARTIAL_GC
void
HeapInfoManager::SweepPartialReusePages(RecyclerSweepManager& recyclerSweepManager)
{
    ForEachHeapInfo(recyclerSweepManager, [=](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.SweepPartialReusePages(recyclerSweep);
    });
}

void
HeapInfoManager::FinishPartialCollect(RecyclerSweepManager * recyclerSweepManager)
{
    ForEachHeapInfo(recyclerSweepManager, [=](HeapInfo& heapInfo, RecyclerSweep * recyclerSweep)
    {
        heapInfo.FinishPartialCollect(recyclerSweep);
    });
}
#endif

#if ENABLE_CONCURRENT_GC
void
HeapInfoManager::PrepareSweep()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.PrepareSweep();
    });
}

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
void
HeapInfoManager::StartAllocationsDuringConcurrentSweep()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.StartAllocationsDuringConcurrentSweep();
    });
}

bool
HeapInfoManager::DoTwoPassConcurrentSweepPreCheck()
{
    return IsAnyHeapInfo([](HeapInfo& heapInfo)
    {
        return heapInfo.DoTwoPassConcurrentSweepPreCheck();
    });
}

void
HeapInfoManager::FinishSweepPrep(RecyclerSweepManager& recyclerSweepManager)
{
    ForEachHeapInfo(recyclerSweepManager, [=](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.FinishSweepPrep(recyclerSweep);
    });
}

void
HeapInfoManager::FinishConcurrentSweepPass1(RecyclerSweepManager& recyclerSweepManager)
{
    ForEachHeapInfo(recyclerSweepManager, [=](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.FinishConcurrentSweepPass1(recyclerSweep);
    });
}

void
HeapInfoManager::FinishConcurrentSweep()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.FinishConcurrentSweep();
    });
}
#endif

void
HeapInfoManager::ConcurrentTransferSweptObjects(RecyclerSweepManager& recyclerSweepManager)
{
    ForEachHeapInfo(recyclerSweepManager, [&](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.ConcurrentTransferSweptObjects(recyclerSweep);
    });
}

#if ENABLE_PARTIAL_GC
void
HeapInfoManager::ConcurrentPartialTransferSweptObjects(RecyclerSweepManager& recyclerSweepManager)
{
    ForEachHeapInfo(recyclerSweepManager, [&](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.ConcurrentPartialTransferSweptObjects(recyclerSweep);
    });
}
#endif
#endif

void
HeapInfoManager::DisposeObjects()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.DisposeObjects();
    });
}

void
HeapInfoManager::TransferDisposedObjects()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.TransferDisposedObjects();
    });
}

void
HeapInfoManager::EnumerateObjects(ObjectInfoBits infoBits, void(*CallBackFunction)(void * address, size_t size))
{
    ForEachHeapInfo([=](HeapInfo& heapInfo)
    {
        heapInfo.EnumerateObjects(infoBits, CallBackFunction);
    });
}

#if DBG
bool
HeapInfoManager::AllocatorsAreEmpty()
{
    return AreAllHeapInfo([](HeapInfo& heapInfo)
    {
        return heapInfo.AllocatorsAreEmpty();
    });
}

void
HeapInfoManager::ResetThreadId()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        return heapInfo.ResetThreadId();
    });
}

void
HeapInfoManager::SetDisableThreadAccessCheck()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        return heapInfo.SetDisableThreadAccessCheck();
    });
}
#endif

#ifdef RECYCLER_FINALIZE_CHECK
size_t 
HeapInfoManager::GetFinalizeCount()
{
    size_t finalizeCount = 0;
    ForEachHeapInfo([&](HeapInfo& heapInfo)
    {
        finalizeCount += heapInfo.GetFinalizeCount();
    });
    return finalizeCount;
}
#endif

#ifdef RECYCLER_SLOW_CHECK_ENABLED
void
HeapInfoManager::Check()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.Check();
    });
}
#endif

#ifdef RECYCLER_MEMORY_VERIFY
void
HeapInfoManager::EnableVerify()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.EnableVerify();
    });
}

void
HeapInfoManager::Verify()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.Verify();
    });
}
#endif

#ifdef RECYCLER_VERIFY_MARK
void
HeapInfoManager::VerifyMark()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.VerifyMark();
    });
}
#endif
#ifdef RECYCLER_NO_PAGE_REUSE
void
HeapInfoManager::DisablePageReuse()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.DisablePageReuse();
    });
}
#endif
}

#ifdef ENABLE_MEM_STATS
#include "BucketStatsReporter.h"
namespace Memory
{
// REVIEW: this is in the wrong place
void
HeapInfoManager::ReportMemStats(Recycler * recycler)
{
    ::Memory::BucketStatsReporter report(recycler);
    if (!report.IsEnabled())
    {
        return;
    }
    ForEachHeapInfo([&](HeapInfo& heapInfo)
    {
        heapInfo.GetBucketStats(report);
    });
    report.Report();
}
}

#endif

#ifdef RECYCLER_PAGE_HEAP
bool
HeapInfoManager::IsPageHeapEnabled()
{
    return IsAnyHeapInfo([](HeapInfo& heapInfo)
    {
        return heapInfo.IsPageHeapEnabled();
    });
}
#endif

// ==============================================================
// Page allocator APIs
// ==============================================================
void
HeapInfoManager::Prime()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.Prime();
    });
}

void
HeapInfoManager::Close()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.CloseNonLeaf();
    });
}

void
HeapInfoManager::SuspendIdleDecommitNonLeaf()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.SuspendIdleDecommitNonLeaf();
    });
}

void
HeapInfoManager::ResumeIdleDecommitNonLeaf()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.ResumeIdleDecommitNonLeaf();
    });
}

void
HeapInfoManager::EnterIdleDecommit()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.EnterIdleDecommit();
    });
}

IdleDecommitSignal
HeapInfoManager::LeaveIdleDecommit(bool allowTimer)
{
    IdleDecommitSignal idleDecommitSignal = IdleDecommitSignal_None;
    ForEachHeapInfo([&idleDecommitSignal, allowTimer](HeapInfo& heapInfo)
    {
        IdleDecommitSignal signal = heapInfo.LeaveIdleDecommit(allowTimer);
        idleDecommitSignal = max(idleDecommitSignal, signal);
    });
    return idleDecommitSignal;
}

#ifdef IDLE_DECOMMIT_ENABLED
DWORD
HeapInfoManager::IdleDecommit()
{
    DWORD waitTime = INFINITE;
    ForEachHeapInfo([&](HeapInfo& heapInfo)
    {
        DWORD heapInfoWaitTime = heapInfo.IdleDecommit();
        waitTime = min(waitTime, heapInfoWaitTime);
    });
    return waitTime;
}

#endif

#if DBG
void
HeapInfoManager::ShutdownIdleDecommit()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.ShutdownIdleDecommit();
    });
}
#endif

#if ENABLE_BACKGROUND_PAGE_ZEROING
void HeapInfoManager::StartQueueZeroPage()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.StartQueueZeroPage();
    });
}

void HeapInfoManager::StopQueueZeroPage()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.StopQueueZeroPage();
    });
}

void HeapInfoManager::BackgroundZeroQueuedPages()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.BackgroundZeroQueuedPages();
    });
}

void HeapInfoManager::ZeroQueuedPages()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.ZeroQueuedPages();
    });
}
\
void HeapInfoManager::FlushBackgroundPages()
{
    ForEachHeapInfo([](HeapInfo& heapInfo)
    {
        heapInfo.FlushBackgroundPages();
    });
}

#if DBG
bool HeapInfoManager::HasZeroQueuedPages()
{
    return IsAnyHeapInfo([](HeapInfo& heapInfo)
    {
        return heapInfo.HasZeroQueuedPages();
    });
}
#endif
#endif

void
HeapInfoManager::DecommitNow(bool all)
{
    ForEachHeapInfo([=](HeapInfo& heapInfo)
    {
        heapInfo.DecommitNow(all);
    });
}

size_t
HeapInfoManager::GetUsedBytes()
{
    size_t usedBytes = 0;
    ForEachHeapInfo([&](HeapInfo& heapInfo)
    {
        usedBytes += heapInfo.GetUsedBytes();
    });
    return usedBytes;
}

size_t
HeapInfoManager::GetReservedBytes()
{
    size_t reservedBytes = 0;
    ForEachHeapInfo([&](HeapInfo& heapInfo)
    {
        reservedBytes += heapInfo.GetReservedBytes();
    });
    return reservedBytes;
}

size_t
HeapInfoManager::GetCommittedBytes()
{
    size_t commitedBytes = 0;
    ForEachHeapInfo([&](HeapInfo& heapInfo)
    {
        commitedBytes += heapInfo.GetCommittedBytes();
    });
    return commitedBytes;
}

size_t
HeapInfoManager::GetNumberOfSegments()
{
    size_t numberOfSegments = 0;
    ForEachHeapInfo([&](HeapInfo& heapInfo)
    {
        numberOfSegments += heapInfo.GetNumberOfSegments();
    });
    return numberOfSegments;
}

AllocationPolicyManager *
HeapInfoManager::GetAllocationPolicyManager()
{
    return this->defaultHeap.GetRecyclerLeafPageAllocator()->GetAllocationPolicyManager();
}

bool
HeapInfoManager::IsRecyclerPageAllocator(PageAllocator * pageAllocator)
{
    return IsAnyHeapInfo([=](HeapInfo& heapInfo)
    {
        return (heapInfo.GetRecyclerPageAllocator() == pageAllocator);
    });
}

bool
HeapInfoManager::IsRecyclerLeafPageAllocator(PageAllocator * pageAllocator)
{
    return IsAnyHeapInfo([=](HeapInfo& heapInfo)
    {
        return (heapInfo.GetRecyclerLeafPageAllocator() == pageAllocator);
    });
}

bool
HeapInfoManager::IsRecyclerLargeBlockPageAllocator(PageAllocator * pageAllocator)
{
    return IsAnyHeapInfo([=](HeapInfo& heapInfo)
    {
        return (heapInfo.GetRecyclerLargeBlockPageAllocator() == pageAllocator);
    });
}

#ifdef RECYCLER_WRITE_BARRIER
bool
HeapInfoManager::IsRecyclerWithBarrierPageAllocator(PageAllocator * pageAllocator)
{
    return IsAnyHeapInfo([=](HeapInfo& heapInfo)
    {
        return (heapInfo.GetRecyclerWithBarrierPageAllocator() == pageAllocator);
    });
}
#endif

#ifdef RECYCLER_PAGE_HEAP
bool
HeapInfoManager::DoCaptureAllocCallStack()
{
    return IsAnyHeapInfo([=](HeapInfo& heapInfo)
    {
        return heapInfo.captureAllocCallStack;
    });
}

bool
HeapInfoManager::DoCaptureFreeCallStack()
{
    return IsAnyHeapInfo([=](HeapInfo& heapInfo)
    {
        return heapInfo.captureFreeCallStack;
    });
}
#endif
