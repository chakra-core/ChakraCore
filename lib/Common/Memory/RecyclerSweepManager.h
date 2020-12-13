//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{

class RecyclerSweepManager
{
public:
#if ENABLE_PARTIAL_GC
    void BeginSweep(Recycler * recycler, size_t rescanRootBytes, bool adjustPartialHeuristics);
#else
    void BeginSweep(Recycler * recycler);
#endif
    void FinishSweep();
    void EndSweep();
    void ShutdownCleanup();

    bool IsBackground() const;
    bool HasSetupBackgroundSweep() const;

#if ENABLE_CONCURRENT_GC
    void BackgroundSweep();
    void BeginBackground(bool forceForeground);
    void EndBackground();

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
    template <typename TBlockType> size_t GetHeapBlockCount(HeapBucketT<TBlockType> const * heapBucket);
    size_t GetPendingMergeNewHeapBlockCount(HeapInfo const * heapInfo);
#endif
#endif

    template <typename TBlockAttributes>
    void AddUnaccountedNewObjectAllocBytes(SmallHeapBlockT<TBlockAttributes> * smallHeapBlock);
#if ENABLE_PARTIAL_GC
    bool InPartialCollect() const;
    void StartPartialCollectMode();
    bool DoPartialCollectMode();
    bool DoAdjustPartialHeuristics() const;
    bool AdjustPartialHeuristics();
    void SubtractSweepNewObjectAllocBytes(size_t newObjectExpectSweepByteCount);
    size_t GetNewObjectAllocBytes() const;
    size_t GetNewObjectFreeBytes() const;
    size_t GetPartialUnusedFreeByteCount() const;
    size_t GetPartialCollectSmallHeapBlockReuseMinFreeBytes() const;

    template <typename TBlockAttributes>
    void NotifyAllocableObjects(SmallHeapBlockT<TBlockAttributes> * smallHeapBlock);
    void AddUnusedFreeByteCount(uint expectedFreeByteCount);

    static const uint MinPartialUncollectedNewPageCount; // 4MB pages
    static const uint MaxPartialCollectRescanRootBytes; // 5MB
#endif

private:
    bool IsMemProtectMode() const;

    Recycler * recycler;
    RecyclerSweep defaultHeapRecyclerSweep;

    bool background;
    bool forceForeground;

    bool inPartialCollect;
#if ENABLE_PARTIAL_GC
    bool adjustPartialHeuristics;
    size_t lastPartialUncollectedAllocBytes;
    size_t nextPartialUncollectedAllocBytes;

    // Sweep data for partial activation heuristic
    size_t rescanRootBytes;
    size_t reuseHeapBlockCount;
    size_t reuseByteCount;

    // Partial reuse Heuristic
    size_t partialCollectSmallHeapBlockReuseMinFreeBytes;

    // Data to update unusedPartialCollectFreeBytes
    size_t partialUnusedFreeByteCount;

#if DBG
    bool partial;
#endif
#endif

    friend class HeapInfoManager;
};

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
template <typename TBlockType>
size_t
RecyclerSweepManager::GetHeapBlockCount(HeapBucketT<TBlockType> const * heapBucket)
{
    return this->defaultHeapRecyclerSweep.GetHeapBlockCount(heapBucket);
}
#endif
};
