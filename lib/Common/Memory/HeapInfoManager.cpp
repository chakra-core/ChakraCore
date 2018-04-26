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
    return fn(defaultHeap) && fn(isolatedHeap);
}

template <class Fn>
bool HeapInfoManager::IsAnyHeapInfo(Fn fn)
{
    return fn(defaultHeap) || fn(isolatedHeap);
}

template <class Fn>
void HeapInfoManager::ForEachHeapInfo(Fn fn)
{
    fn(defaultHeap);
    fn(isolatedHeap);
}

template <class Fn>
void HeapInfoManager::ForEachHeapInfo(RecyclerSweepManager& recyclerSweepManager, Fn fn)
{
    fn(defaultHeap, recyclerSweepManager.defaultHeapRecyclerSweep);
    fn(isolatedHeap, recyclerSweepManager.isolatedHeapRecyclerSweep);
}

template <class Fn>
void HeapInfoManager::ForEachHeapInfo(RecyclerSweepManager * recyclerSweepManager, Fn fn)
{
    fn(defaultHeap, recyclerSweepManager? &recyclerSweepManager->defaultHeapRecyclerSweep : nullptr);
    fn(isolatedHeap, recyclerSweepManager? &recyclerSweepManager->isolatedHeapRecyclerSweep : nullptr);
}

HeapInfoManager::HeapInfoManager() :
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
    // Call finalize before sweeping so that the finalizer can still access object it referenced
    ForEachHeapInfo(recyclerSweepManager, [&](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.Finalize(recyclerSweep);
    });
    ForEachHeapInfo(recyclerSweepManager, [&](HeapInfo& heapInfo, RecyclerSweep& recyclerSweep)
    {
        heapInfo.Sweep(recyclerSweep, concurrent);
    });
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