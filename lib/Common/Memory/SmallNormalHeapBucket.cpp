//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"


template <typename TBlockType>
SmallNormalHeapBucketBase<TBlockType>::SmallNormalHeapBucketBase()
#if ENABLE_PARTIAL_GC
    : partialHeapBlockList(nullptr)
#if ENABLE_CONCURRENT_GC
    , partialSweptHeapBlockList(nullptr)
#endif
#endif
{
}

#ifdef DUMP_FRAGMENTATION_STATS
template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::AggregateBucketStats(HeapBucketStats& stats)
{
    __super::AggregateBucketStats(stats);

    HeapBlockList::ForEach(partialHeapBlockList, [&stats](SmallHeapBlock* heapBlock) {
        heapBlock->AggregateBlockStats(stats);
    });
    HeapBlockList::ForEach(partialSweptHeapBlockList, [&stats](SmallHeapBlock* heapBlock) {
        heapBlock->AggregateBlockStats(stats);
    });
}
#endif

template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::ScanInitialImplicitRoots(Recycler * recycler)
{
    HeapBlockList::ForEach(this->fullBlockList, [recycler](TBlockType * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (CONFIG_FLAG_RELEASE(EnableConcurrentSweepAlloc))
    {
        HeapBlockList::ForEach(this->sweepableHeapBlockList, [recycler](TBlockType * heapBlock)
        {
            heapBlock->ScanInitialImplicitRoots(recycler);
        });
    }
#endif

    HeapBlockList::ForEach(this->heapBlockList, [recycler](TBlockType * heapBlock)
    {
        heapBlock->ScanInitialImplicitRoots(recycler);
    });

#if ENABLE_PARTIAL_GC
    Assert(recycler->inPartialCollectMode || partialHeapBlockList == nullptr);
    if (recycler->inPartialCollectMode)
    {
        HeapBlockList::ForEach(partialHeapBlockList, [recycler](TBlockType * heapBlock)
        {
            heapBlock->ScanInitialImplicitRoots(recycler);
        });
#if ENABLE_CONCURRENT_GC
        HeapBlockList::ForEach(partialSweptHeapBlockList, [recycler](TBlockType * heapBlock)
        {
            heapBlock->ScanInitialImplicitRoots(recycler);
        });
#endif
    }
#endif
}

template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::ScanNewImplicitRoots(Recycler * recycler)
{
    __super::ScanNewImplicitRoots(recycler);

#if ENABLE_PARTIAL_GC
    Assert(recycler->inPartialCollectMode || partialHeapBlockList == nullptr);
    // Don't need to scan the partial heap block list for new implicit root as we don't allocate from them
#endif
}


template <typename TBlockType>
bool
SmallNormalHeapBucketBase<TBlockType>::RescanObjectsOnPage(TBlockType * block, char* pageAddress, char * blockStartAddress, BVStatic<TBlockAttributes::BitVectorCount> * heapBlockMarkBits, const uint localObjectSize, uint bucketIndex, __out_opt bool* anyObjectRescanned, Recycler * recycler)
{
    RECYCLER_STATS_ADD(recycler, markData.rescanPageCount, TBlockAttributes::PageCount);

    // By the time we get here, we should have ensured that there's a mark on any page somewhere.
    // REVIEW: Worth check on just the page's mark bits?
    Assert(!heapBlockMarkBits->IsAllClear());

    if (anyObjectRescanned != nullptr)
    {
        *anyObjectRescanned = false;
    }

    Assert((char*)pageAddress - blockStartAddress < TBlockAttributes::PageCount * AutoSystemInfo::PageSize);
    const uint pageByteOffset = static_cast<uint>((char*)pageAddress - blockStartAddress);
    uint firstObjectOnPageIndex = pageByteOffset / localObjectSize;

    // This is not necessarily the address on the first object that starts on the page
    // If the last object on the previous page spans two pages, this is the address of that object
    // We do it this way so that we can figure out if we need to rescan the first few bytes of the page
    // if the actual first object on this page is not located at the start of the page
    char* const startObjectAddress = blockStartAddress + (firstObjectOnPageIndex * localObjectSize);
    const uint startBitIndex = TBlockType::GetAddressBitIndex(startObjectAddress);
    const uint pageStartBitIndex = pageByteOffset >> HeapConstants::ObjectAllocationShift;

    Assert(pageByteOffset / AutoSystemInfo::PageSize < USHRT_MAX);
    const ushort pageNumber = static_cast<const ushort>(pageByteOffset / AutoSystemInfo::PageSize);
    const typename TBlockType::BlockInfo& blockInfoForPage = HeapInfo::GetBlockInfo<TBlockAttributes>(localObjectSize)[pageNumber];

    bool lastObjectOnPreviousPageMarked = false;
    // Calculate the mark count here since we no longer keep track during marking
    uint rescanMarkCount = TBlockType::CalculateMarkCountForPage(heapBlockMarkBits, bucketIndex, pageStartBitIndex);
    const uint pageObjectCount = blockInfoForPage.pageObjectCount;
    const uint localObjectCount = (TBlockAttributes::PageCount * AutoSystemInfo::PageSize) / localObjectSize;

    // With protected unallocatable ending pages and reset writewatch, we should never be scanning on these pages.
    if (firstObjectOnPageIndex >= localObjectCount)
    {
        ReportFatalException(NULL, E_FAIL, Fatal_Recycler_MemoryCorruption, 3);
    }

    // If all objects are marked, rescan whole block at once
    if (TBlockType::CanRescanFullBlock() && rescanMarkCount == pageObjectCount)
    {
        // REVIEW: Can we optimize this more?
        if (!recycler->AddMark(pageAddress, AutoSystemInfo::PageSize))
        {
            // Failed to add to the mark stack due to OOM.
            return false;
        }

        RECYCLER_STATS_ADD(recycler, markData.rescanObjectCount, pageObjectCount);
        RECYCLER_STATS_ADD(recycler, markData.rescanObjectByteCount, localObjectSize * pageObjectCount);
        if (anyObjectRescanned != nullptr)
        {
            *anyObjectRescanned = true;
        }

        return true;
    }

    if (startObjectAddress != pageAddress)
    {
        // If the last object on the previous page that spans into the current page is marked,
        // we need to count that in the markCount for rescan
        Assert(startObjectAddress >= blockStartAddress && startObjectAddress < pageAddress);
        lastObjectOnPreviousPageMarked = (heapBlockMarkBits->Test(startBitIndex) == TRUE);
        if (lastObjectOnPreviousPageMarked)
        {
            rescanMarkCount++;
        }
    }

    const uint objectBitDelta = SmallHeapBlockT<TBlockAttributes>::GetObjectBitDeltaForBucketIndex(bucketIndex);

    uint rescanCount = 0;
    uint objectIndex = firstObjectOnPageIndex;

    for (uint bitIndex = startBitIndex; rescanCount < rescanMarkCount; objectIndex++, bitIndex += objectBitDelta)
    {
        Assert(objectIndex < localObjectCount);
        Assert(!HeapInfo::GetInvalidBitVectorForBucket<TBlockAttributes>(bucketIndex)->Test(bitIndex));

        if (heapBlockMarkBits->Test(bitIndex))
        {
            char * objectAddress = blockStartAddress + objectIndex * localObjectSize;
            if (!TBlockType::RescanObject(block, objectAddress, localObjectSize, objectIndex, recycler))
            {
                // Failed to add to the mark stack due to OOM.
                return false;
            }

            rescanCount++;
        }
    }

    // Mark bits should not have changed during the Rescan
    if (startObjectAddress != pageAddress && lastObjectOnPreviousPageMarked)
    {
        Assert(rescanMarkCount == TBlockType::CalculateMarkCountForPage(heapBlockMarkBits, bucketIndex, pageStartBitIndex) + 1);
    }
    else
    {
        Assert(rescanMarkCount == TBlockType::CalculateMarkCountForPage(heapBlockMarkBits, bucketIndex, pageStartBitIndex));
    }

#if DBG
    // We stopped when we hit the rescanMarkCount.
    // Make sure no other objects were marked, otherwise our rescanMarkCount was wrong.
    for (uint i = objectIndex + 1; i < blockInfoForPage.lastObjectIndexOnPage; i++)
    {
        Assert(!heapBlockMarkBits->Test(i * objectBitDelta));
    }
#endif

    // Let the caller know if we rescanned anything on this page
    if (anyObjectRescanned != nullptr)
    {
        (*anyObjectRescanned) = (rescanCount > 0);
    }

    return true;
}

#if ENABLE_CONCURRENT_GC
template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::SweepPendingObjects(RecyclerSweep& recyclerSweep)
{
    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(recyclerSweep.IsBackground()));

    CompileAssert(!BaseT::IsLeafBucket);
    TBlockType *& pendingSweepList = recyclerSweep.GetPendingSweepBlockList(this);
    TBlockType * const list = pendingSweepList;
    Recycler * const recycler = recyclerSweep.GetRecycler();
#if ENABLE_PARTIAL_GC
    bool const partialSweep = recycler->inPartialCollectMode;
#endif
    if (list)
    {
        pendingSweepList = nullptr;
#if ENABLE_PARTIAL_GC
        if (partialSweep)
        {
            // We did a partial sweep.
            // Blocks in the pendingSweepList are the ones we decided not to reuse.

            HeapBlockList::ForEachEditing(list, [this, recycler](TBlockType * heapBlock)
            {
                // We are not going to reuse this block.
                // SweepMode_ConcurrentPartial will not actually collect anything, it will just update some state.
                // The sweepable objects will be collected in a future Sweep.

                // Note, page heap blocks are never swept concurrently
                heapBlock->template SweepObjects<SweepMode_ConcurrentPartial>(recycler);

                // page heap mode should never reach here, so don't check pageheap enabled or not
                if (heapBlock->HasFreeObject())
                {
                    // We have pre-existing free objects, so put this in the partialSweptHeapBlockList
                    heapBlock->SetNextBlock(this->partialSweptHeapBlockList);
                    this->partialSweptHeapBlockList = heapBlock;
                }
                else
                {
                    // No free objects, so put in the fullBlockList
                    heapBlock->SetNextBlock(this->fullBlockList);
                    this->fullBlockList = heapBlock;
                }
            });
        }
        else
#endif
        {
            // We decided not to do a partial sweep.
            // Blocks in the pendingSweepList need to have a regular sweep.

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
            if (this->AllowAllocationsDuringConcurrentSweep() && !this->AllocationsStartedDuringConcurrentSweep())
            {
                this->StartAllocationDuringConcurrentSweep();
            }
#endif

            TBlockType * tail = SweepPendingObjects<SweepMode_Concurrent>(recycler, list);

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
            // During concurrent sweep if allocations were allowed, the heap blocks directly go into the SLIST of
            // allocable heap blocks. They will be returned to the heapBlockList at the end of the sweep.
            if (!this->AllowAllocationsDuringConcurrentSweep())
#endif
            {
                tail->SetNextBlock(this->heapBlockList);
                this->heapBlockList = list;

                this->StartAllocationAfterSweep();
            }
        }

        RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(recyclerSweep.IsBackground()));
    }

    Assert(!this->IsAllocationStopped());
}

template <typename TBlockType>
template <SweepMode mode>
TBlockType *
SmallNormalHeapBucketBase<TBlockType>::SweepPendingObjects(Recycler * recycler, TBlockType * list)
{
    TBlockType * tail;
    HeapBlockList::ForEachEditing(list, [this, recycler, &tail](TBlockType * heapBlock)
    {
        // Note, page heap blocks are never swept concurrently
        heapBlock->template SweepObjects<mode>(recycler);
        tail = heapBlock;

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
        if (this->AllowAllocationsDuringConcurrentSweep())
        {
            bool blockAddedToSList = HeapBucketT<TBlockType>::PushHeapBlockToSList(this->allocableHeapBlockListHead, heapBlock);

            // If we encountered OOM while pushing the heapBlock to the SLIST we must add it to the heapBlockList so we don't lose track of it.
            if (!blockAddedToSList)
            {
                heapBlock->SetNextBlock(this->heapBlockList);
                this->heapBlockList = heapBlock;
            }
        }
#endif
    });
    return tail;
}
#endif

#if ENABLE_PARTIAL_GC
template <typename TBlockType>
SmallNormalHeapBucketBase<TBlockType>::~SmallNormalHeapBucketBase()
{
    this->DeleteHeapBlockList(this->partialHeapBlockList);
#if ENABLE_CONCURRENT_GC
    this->DeleteHeapBlockList(this->partialSweptHeapBlockList);
#endif
}

template <typename TBlockType>
template <class Fn>
void
SmallNormalHeapBucketBase<TBlockType>::SweepPartialReusePages(RecyclerSweep& recyclerSweep, TBlockType * heapBlockList,
    TBlockType *& reuseBlocklist, TBlockType *&unusedBlockList, bool allocationsAllowedDuringConcurrentSweep, Fn callback)
{
    HeapBlockList::ForEachEditing(heapBlockList,
        [&recyclerSweep, &reuseBlocklist, &unusedBlockList, callback, allocationsAllowedDuringConcurrentSweep](TBlockType * heapBlock)
    {
        uint expectFreeByteCount;
        if (heapBlock->DoPartialReusePage(recyclerSweep, expectFreeByteCount))
        {
            callback(heapBlock, true);


#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
            // During concurrent sweep if allocations were allowed, the heap blocks directly go into the SLIST of
            // allocable heap blocks. They will be returned to the heapBlockList at the end of the sweep.
            if(!allocationsAllowedDuringConcurrentSweep)
#endif
            {
                // Reuse the page
                heapBlock->SetNextBlock(reuseBlocklist);
                reuseBlocklist = heapBlock;
            }

            RECYCLER_STATS_ADD(recyclerSweep.GetRecycler(), smallNonLeafHeapBlockPartialReuseBytes[heapBlock->GetHeapBlockType()], expectFreeByteCount);
            RECYCLER_STATS_INC(recyclerSweep.GetRecycler(), smallNonLeafHeapBlockPartialReuseCount[heapBlock->GetHeapBlockType()]);
        }
        else
        {
            // Don't not reuse the page if it don't have much free memory.
            callback(heapBlock, false);

            heapBlock->SetNextBlock(unusedBlockList);
            unusedBlockList = heapBlock;

            recyclerSweep.AddUnusedFreeByteCount(expectFreeByteCount);
            RECYCLER_STATS_ADD(recyclerSweep.GetRecycler(), smallNonLeafHeapBlockPartialUnusedBytes[heapBlock->GetHeapBlockType()], expectFreeByteCount);
            RECYCLER_STATS_INC(recyclerSweep.GetRecycler(), smallNonLeafHeapBlockPartialUnusedCount[heapBlock->GetHeapBlockType()]);
        }
    });
}

template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::SweepPartialReusePages(RecyclerSweep& recyclerSweep)
{
    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(recyclerSweep.IsBackground()));
    Assert(this->GetRecycler()->inPartialCollectMode);

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (this->AllowAllocationsDuringConcurrentSweep() && !this->AllocationsStartedDuringConcurrentSweep())
    {
        this->StartAllocationDuringConcurrentSweep();
    }
#endif

    TBlockType * currentHeapBlockList = this->heapBlockList;
    this->heapBlockList = nullptr;
    SmallNormalHeapBucketBase<TBlockType>::SweepPartialReusePages(recyclerSweep, currentHeapBlockList, this->heapBlockList,
        this->partialHeapBlockList, this->AllowAllocationsDuringConcurrentSweep(),
        [this](TBlockType * heapBlock, bool isReused) 
    {
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
        if (isReused)
        {
            if (this->AllowAllocationsDuringConcurrentSweep())
            {
                bool blockAddedToSList = HeapBucketT<TBlockType>::PushHeapBlockToSList(this->allocableHeapBlockListHead, heapBlock);

                // If we encountered OOM while pushing the heapBlock to the SLIST we must add it to the heapBlockList so we don't lose track of it.
                if (!blockAddedToSList)
                {
                    heapBlock->SetNextBlock(this->heapBlockList);
                    this->heapBlockList = heapBlock;
                }
            }
        }
#endif
    });

#if ENABLE_CONCURRENT_GC
    // only collect data for pending sweep list but don't sweep yet
    // until we have adjusted the heuristics, and SweepPartialReusePages will
    // sweep the page that we are going to reuse in thread.
    TBlockType *& pendingSweepList = recyclerSweep.GetPendingSweepBlockList(this);
    currentHeapBlockList = pendingSweepList;
    pendingSweepList = nullptr;
    Recycler * recycler = recyclerSweep.GetRecycler();
    SmallNormalHeapBucketBase<TBlockType>::SweepPartialReusePages(recyclerSweep, currentHeapBlockList, this->heapBlockList,
        pendingSweepList, this->AllowAllocationsDuringConcurrentSweep(),
        [this, recycler](TBlockType * heapBlock, bool isReused)
        {
            if (isReused)
            {
                // Finalizable blocks are always swept in thread, so shouldn't be here
                Assert(!heapBlock->IsAnyFinalizableBlock());

                // Page heap blocks are never swept concurrently
                heapBlock->template SweepObjects<SweepMode_InThread>(recycler);
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
                if (this->AllowAllocationsDuringConcurrentSweep())
                {
                    bool blockAddedToSList = HeapBucketT<TBlockType>::PushHeapBlockToSList(this->allocableHeapBlockListHead, heapBlock);

                    // If we encountered OOM while pushing the heapBlock to the SLIST we must add it to the heapBlockList so we don't lose track of it.
                    if (!blockAddedToSList)
                    {
                        heapBlock->SetNextBlock(this->heapBlockList);
                        this->heapBlockList = heapBlock;
                    }
                }
#endif

                // This block has been counted as concurrently swept, and now we changed our mind
                // and sweep it in thread. Remove the count
                RECYCLER_STATS_DEC(recycler, heapBlockConcurrentSweptCount[heapBlock->GetHeapBlockType()]);
            }
        }
    );
#endif

    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(recyclerSweep.IsBackground()));

#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP && SUPPORT_WIN32_SLIST && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP_USE_SLIST
    if (!this->AllocationsStartedDuringConcurrentSweep())
#endif
    {
        this->StartAllocationAfterSweep();
    }

    // PARTIALGC-TODO: revisit partial heap blocks to see if they can be put back into use
    // since the heuristics limit may be been changed.
}

template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::FinishPartialCollect(RecyclerSweep * recyclerSweep)
{
    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(recyclerSweep != nullptr && recyclerSweep->IsBackground()));

    Assert(this->GetRecycler()->inPartialCollectMode);
    Assert(recyclerSweep == nullptr || this->IsAllocationStopped());

#if ENABLE_CONCURRENT_GC
    // Process the partial Swept block and move it to the partial heap block list
    TBlockType * partialSweptList = this->partialSweptHeapBlockList;
    if (partialSweptList)
    {
        this->partialSweptHeapBlockList = nullptr;
        TBlockType *  tail = nullptr;
        HeapBlockList::ForEach(partialSweptList, [this, &tail](TBlockType * heapBlock)
        {
            heapBlock->FinishPartialCollect();
            Assert(heapBlock->HasFreeObject());
            tail = heapBlock;
        });
        Assert(tail != nullptr);
        tail->SetNextBlock(this->partialHeapBlockList);
        this->partialHeapBlockList = partialSweptList;
    }
#endif

    TBlockType * currentPartialHeapBlockList = this->partialHeapBlockList;
    if (recyclerSweep == nullptr)
    {
        if (currentPartialHeapBlockList != nullptr)
        {
            this->partialHeapBlockList = nullptr;
            this->AppendAllocableHeapBlockList(currentPartialHeapBlockList);
        }
    }
    else
    {
        if (currentPartialHeapBlockList != nullptr)
        {
            this->partialHeapBlockList = nullptr;
            TBlockType * list = this->heapBlockList;
            if (list == nullptr)
            {
                this->heapBlockList = currentPartialHeapBlockList;
            }
            else
            {
                // CONCURRENT-TODO: Optimize this?
                TBlockType * tail = HeapBlockList::Tail(this->heapBlockList);
                tail->SetNextBlock(currentPartialHeapBlockList);
            }
        }
#if ENABLE_CONCURRENT_GC
        if (recyclerSweep->GetPendingSweepBlockList(this) == nullptr)
#endif
        {
            // nothing else to sweep now,  we can start allocating now.
            this->StartAllocationAfterSweep();
        }
    }

    RECYCLER_SLOW_CHECK(this->VerifyHeapBlockCount(recyclerSweep != nullptr && recyclerSweep->IsBackground()));
}

template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size))
{
    __super::EnumerateObjects(infoBits, CallBackFunction);
    HeapBucket::EnumerateObjects(partialHeapBlockList, infoBits, CallBackFunction);
#if ENABLE_CONCURRENT_GC
    HeapBucket::EnumerateObjects(partialSweptHeapBlockList, infoBits, CallBackFunction);
#endif
}

//------------------------------------------------------------------------------
// Debug and verify functions
//------------------------------------------------------------------------------
#if DBG
template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::ResetMarks(ResetMarkFlags flags)
{
    Assert(this->partialHeapBlockList == nullptr);
#if ENABLE_CONCURRENT_GC
    Assert(this->partialSweptHeapBlockList == nullptr);
#endif
    __super::ResetMarks(flags);
}

template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::SweepVerifyPartialBlocks(Recycler * recycler, TBlockType * heapBlockList)
{
    // PARTIALGC-TODO: Add assert to ensure nothing in the partialHeapBlockList is free-able
    HeapBlockList::ForEach(heapBlockList, [recycler](TBlockType * heapBlock)
    {
        heapBlock->SweepVerifyPartialBlock(recycler);
    });
}
#endif // DBG

#if DBG || defined(RECYCLER_SLOW_CHECK_ENABLED)
template <typename TBlockType>
size_t
SmallNormalHeapBucketBase<TBlockType>::GetNonEmptyHeapBlockCount(bool checkCount) const
{
    size_t currentHeapBlockCount = __super::GetNonEmptyHeapBlockCount(false);
    currentHeapBlockCount += HeapBlockList::Count(partialHeapBlockList);
#if ENABLE_CONCURRENT_GC
    currentHeapBlockCount += HeapBlockList::Count(partialSweptHeapBlockList);
#endif
    RECYCLER_SLOW_CHECK(Assert(!checkCount || this->heapBlockCount == currentHeapBlockCount));
    return currentHeapBlockCount;
}
#endif
#ifdef RECYCLER_SLOW_CHECK_ENABLED
template <typename TBlockType>
size_t
SmallNormalHeapBucketBase<TBlockType>::Check(bool checkCount)
{
    size_t smallHeapBlockCount = __super::Check(false);
    Assert(partialHeapBlockList == nullptr || this->GetRecycler()->inPartialCollectMode);
    smallHeapBlockCount += HeapInfo::Check(false, false, this->partialHeapBlockList);

#if ENABLE_CONCURRENT_GC
    Assert(partialSweptHeapBlockList == nullptr || this->GetRecycler()->inPartialCollectMode);
    smallHeapBlockCount += HeapInfo::Check(false, false, this->partialSweptHeapBlockList);
#endif
    Assert(!checkCount || this->heapBlockCount == smallHeapBlockCount);
    return smallHeapBlockCount;
}

#endif // RECYCLER_SLOW_CHECK_ENABLED

#ifdef RECYCLER_MEMORY_VERIFY
template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::Verify()
{
    __super::Verify();
    Assert(this->partialHeapBlockList == nullptr || this->GetRecycler()->inPartialCollectMode);
    HeapBlockList::ForEach(this->partialHeapBlockList, [](TBlockType * heapBlock)
    {
        Assert(heapBlock->HasFreeObject());
        heapBlock->Verify();
    });
#if ENABLE_CONCURRENT_GC
    Assert(this->partialSweptHeapBlockList == nullptr || this->GetRecycler()->inPartialCollectMode);
    HeapBlockList::ForEach(this->partialSweptHeapBlockList, [](TBlockType * heapBlock)
    {
        heapBlock->Verify();
    });
#endif
}
#endif // RECYCLER_MEMORY_VERIFY
#ifdef RECYCLER_VERIFY_MARK
template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::VerifyMark()
{
    __super::VerifyMark();
    HeapBlockList::ForEach(this->partialHeapBlockList, [](TBlockType * heapBlock)
    {
        heapBlock->VerifyMark();
    });

#if ENABLE_CONCURRENT_GC
    HeapBlockList::ForEach(this->partialSweptHeapBlockList, [](TBlockType * heapBlock)
    {
        heapBlock->VerifyMark();
    });
#endif
}
#endif // RECYCLER_VERIFY_MARK
#endif // ENABLE_PARTIAL_GC

template <typename TBlockType>
void
SmallNormalHeapBucketBase<TBlockType>::Sweep(RecyclerSweep& recyclerSweep)
{
#if ENABLE_PARTIAL_GC
#if DBG
    Recycler * recycler = recyclerSweep.GetRecycler();
    // Don't need sweep the partialHeapBlockList, the partially collected heap block list.
    // There should be nothing there that is free-able since the last time we swept

    Assert(recyclerSweep.InPartialCollect() || partialHeapBlockList == nullptr);
#if ENABLE_CONCURRENT_GC
    Assert(recyclerSweep.InPartialCollect() || partialSweptHeapBlockList == nullptr);
#endif
    this->SweepVerifyPartialBlocks(recycler, this->partialHeapBlockList);
#endif
#endif

    BaseT::SweepBucket(recyclerSweep, [](RecyclerSweep& recyclerSweep){});
}

namespace Memory
{
    template class SmallNormalHeapBucketBase<SmallNormalHeapBlock>;
    template class SmallNormalHeapBucketBase<MediumNormalHeapBlock>;

#ifdef RECYCLER_WRITE_BARRIER
    template class SmallNormalHeapBucketBase<SmallNormalWithBarrierHeapBlock>;
    template class SmallNormalHeapBucketBase<MediumNormalWithBarrierHeapBlock>;
#endif

    template class SmallNormalHeapBucketBase<SmallFinalizableHeapBlock>;
    template class SmallNormalHeapBucketBase<MediumFinalizableHeapBlock>;

#ifdef RECYCLER_WRITE_BARRIER
    template class SmallNormalHeapBucketBase<SmallFinalizableWithBarrierHeapBlock>;
    template class SmallNormalHeapBucketBase<MediumFinalizableWithBarrierHeapBlock>;
#endif

    template void SmallNormalHeapBucketBase<SmallNormalHeapBlock>::Sweep(RecyclerSweep& recyclerSweep);
    template void SmallNormalHeapBucketBase<MediumNormalHeapBlock>::Sweep(RecyclerSweep& recyclerSweep);

#ifdef RECYCLER_WRITE_BARRIER
    template void SmallNormalHeapBucketBase<SmallNormalWithBarrierHeapBlock>::Sweep(RecyclerSweep& recyclerSweep);
    template void SmallNormalHeapBucketBase<MediumNormalWithBarrierHeapBlock>::Sweep(RecyclerSweep& recyclerSweep);
#endif
}

