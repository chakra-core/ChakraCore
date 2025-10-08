//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonMemoryPch.h"


#ifdef ENABLE_BASIC_TELEMETRY

#include "Recycler.h"
#include "DataStructures/SList.h"
#include "HeapBucketStats.h"
#include "BucketStatsReporter.h"
#include <psapi.h>

namespace Memory
{
#ifdef DBG
#define AssertOnValidThread(recyclerTelemetryInfo, loc) { AssertMsg(this->IsOnScriptThread(), STRINGIZE("Unexpected thread ID at " ## loc)); }
#else
#define AssertOnValidThread(recyclerTelemetryInfo, loc)
#endif

    RecyclerTelemetryInfo::RecyclerTelemetryInfo(Recycler * recycler, RecyclerTelemetryHostInterface* hostInterface) :
        passCount(0),
        perfTrackPassCount(0),
        hostInterface(hostInterface),
        gcPassStats(&HeapAllocator::Instance),
        recyclerStartTime(Js::Tick::Now()),
        abortTelemetryCapture(false),
        inPassActiveState(false),
        recycler(recycler)
    {
        mainThreadID = ::GetCurrentThreadId();
    }

    RecyclerTelemetryInfo::~RecyclerTelemetryInfo()
    {
        if (this->hostInterface != nullptr)
        {
            AssertOnValidThread(this, RecyclerTelemetryInfo::~RecyclerTelemetryInfo);
            if (this->gcPassStats.Empty() == false)
            {
                this->hostInterface->TransmitGCTelemetryStats(*this);
                this->FreeGCPassStats();
            }
        }
    }

    const GUID& RecyclerTelemetryInfo::GetRecyclerID() const
    {
        return this->recycler->GetRecyclerID();
    }

    RecyclerFlagsTableSummary RecyclerTelemetryInfo::GetRecyclerConfigFlags() const
    {

        // select set of config flags that we can pack into an uint32
        RecyclerFlagsTableSummary flags = RecyclerFlagsTableSummary::None;

        if (this->recycler->IsMemProtectMode())                    { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::IsMemProtectMode);                     }
        if (this->recycler->IsConcurrentEnabled())                 { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::IsConcurrentEnabled);                  }
        if (this->recycler->enableScanInteriorPointers)            { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::EnableScanInteriorPointers);           }
        if (this->recycler->enableScanImplicitRoots)               { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::EnableScanImplicitRoots);              }
        if (this->recycler->disableCollectOnAllocationHeuristics)  { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::DisableCollectOnAllocationHeuristics); }
#ifdef RECYCLER_STRESS
        if (this->recycler->recyclerStress)                        { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::RecyclerStress);                       }
#if ENABLE_CONCURRENT_GC
        if (this->recycler->recyclerBackgroundStress)              { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::RecyclerBackgroundStress);             }
        if (this->recycler->recyclerConcurrentStress)              { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::RecyclerConcurrentStress);             }
        if (this->recycler->recyclerConcurrentRepeatStress)        { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::RecyclerConcurrentRepeatStress);       }
#endif
#if ENABLE_PARTIAL_GC
        if (this->recycler->recyclerPartialStress)                 { flags = static_cast<RecyclerFlagsTableSummary>(flags | RecyclerFlagsTableSummary::RecyclerPartialStress);                }
#endif
#endif
        return flags;
    }

    bool RecyclerTelemetryInfo::ShouldStartTelemetryCapture() const
    {
        return
            this->hostInterface != nullptr &&
            this->abortTelemetryCapture == false &&
            this->hostInterface->IsTelemetryProviderEnabled();
    }

    void RecyclerTelemetryInfo::FillInSizeData(IdleDecommitPageAllocator* allocator, AllocatorSizes* sizes) const
    {
        sizes->committedBytes = allocator->GetCommittedBytes();
        sizes->reservedBytes = allocator->GetReservedBytes();
        sizes->usedBytes = allocator->GetUsedBytes();
        sizes->numberOfSegments = allocator->GetNumberOfSegments();
    }

    GCPassStatsList::Iterator RecyclerTelemetryInfo::GetGCPassStatsIterator() const
    {
        return this->gcPassStats.GetIterator();
    }

    RecyclerTelemetryGCPassStats* RecyclerTelemetryInfo::GetLastPassStats() const
    {
        RecyclerTelemetryGCPassStats* stats = nullptr;
        if (this->gcPassStats.Empty() == false)
        {
            RecyclerTelemetryGCPassStats& ref = const_cast<RecyclerTelemetryGCPassStats&>(this->gcPassStats.Head());
            stats = &ref;
        }
        return stats;
    }

    void RecyclerTelemetryInfo::StartPass(CollectionState collectionState)
    {
        this->inPassActiveState = false;
        if (this->ShouldStartTelemetryCapture())
        {
            Js::Tick start = Js::Tick::Now();
            AssertOnValidThread(this, RecyclerTelemetryInfo::StartPass);

#if DBG
            // validate state of existing GC pass stats structs
            uint16 count = 0;
            if (this->gcPassStats.Empty() == false)
            {
                GCPassStatsList::Iterator it = this->GetGCPassStatsIterator();
                while (it.Next())
                {
                    RecyclerTelemetryGCPassStats& curr = it.Data();
                    AssertMsg(curr.isGCPassActive == false, "unexpected value for isGCPassActive");
                    ++count;
                }
            }
            AssertMsg(count == this->passCount, "RecyclerTelemetryInfo::StartPass() - mismatch between passCount and count.");
#endif


            RecyclerTelemetryGCPassStats* stats = this->gcPassStats.PrependNodeNoThrow(&HeapAllocator::Instance);
            if (stats == nullptr)
            {
                // failed to allocate memory - disable any further telemetry capture for this recycler 
                // and free any existing GC stats we've accumulated
                this->abortTelemetryCapture = true;
                FreeGCPassStats();
                this->hostInterface->TransmitTelemetryError(*this, "Memory Allocation Failed");
            }
            else
            {
                this->inPassActiveState = true;
                passCount++;
                perfTrackPassCount++;
                memset(stats, 0, sizeof(RecyclerTelemetryGCPassStats));

                stats->startPassCollectionState = collectionState;
                stats->isGCPassActive = true;
                stats->passStartTimeTick = Js::Tick::Now();
                GetSystemTimePreciseAsFileTime(&stats->passStartTimeFileTime);
                if (this->hostInterface != nullptr)
                {
                    LPFILETIME ft = this->hostInterface->GetLastScriptExecutionEndTime();
                    stats->lastScriptExecutionEndTime = *ft;

                    stats->closedContextCount = this->hostInterface->GetClosedContextCount();
                }

                stats->processCommittedBytes_start = RecyclerTelemetryInfo::GetProcessCommittedBytes();
                stats->processAllocaterUsedBytes_start = PageAllocator::GetProcessUsedBytes();
                stats->isInScript = this->recycler->GetIsInScript();
                stats->isScriptActive = this->recycler->GetIsScriptActive();

                this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerLeafPageAllocator(), &stats->threadPageAllocator_start);
                this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerPageAllocator(), &stats->recyclerLeafPageAllocator_start);
                this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerLargeBlockPageAllocator(), &stats->recyclerLargeBlockPageAllocator_start);
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
                this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerWithBarrierPageAllocator(), &stats->recyclerWithBarrierPageAllocator_start);
#endif
                stats->startPassProcessingElapsedTime = Js::Tick::Now() - start;

                stats->pinnedObjectCount = this->recycler->pinnedObjectMap.Count();
            }
        }
    }

    void RecyclerTelemetryInfo::EndPass(CollectionState collectionState)
    {
        if (!this->inPassActiveState)
        {
            return;
        }
        this->inPassActiveState = false;

        Js::Tick start = Js::Tick::Now();

        AssertOnValidThread(this, RecyclerTelemetryInfo::EndPass);
        RecyclerTelemetryGCPassStats* lastPassStats = this->GetLastPassStats();

        lastPassStats->endPassCollectionState = collectionState;
        lastPassStats->collectionStartReason = this->recycler->collectionStartReason;
        lastPassStats->collectionFinishReason = this->recycler->collectionFinishReason;
        lastPassStats->collectionStartFlags = this->recycler->collectionStartFlags;
        lastPassStats->isGCPassActive = false;
        lastPassStats->passEndTimeTick = Js::Tick::Now();

        lastPassStats->processCommittedBytes_end = RecyclerTelemetryInfo::GetProcessCommittedBytes();
        lastPassStats->processAllocaterUsedBytes_end = PageAllocator::GetProcessUsedBytes();

        this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerLeafPageAllocator(), &lastPassStats->threadPageAllocator_end);
        this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerPageAllocator(), &lastPassStats->recyclerLeafPageAllocator_end);
        this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerLargeBlockPageAllocator(), &lastPassStats->recyclerLargeBlockPageAllocator_end);
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
        this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerWithBarrierPageAllocator(), &lastPassStats->recyclerWithBarrierPageAllocator_end);
#endif

        // get bucket stats
        Js::Tick bucketStatsStart = Js::Tick::Now();
        BucketStatsReporter bucketReporter(this->recycler);
        this->recycler->GetHeapInfo()->GetBucketStats(bucketReporter);
        memcpy(&lastPassStats->bucketStats, bucketReporter.GetTotalStats(), sizeof(HeapBucketStats));
        lastPassStats->computeBucketStatsElapsedTime = Js::Tick::Now() - bucketStatsStart;

        lastPassStats->endPassProcessingElapsedTime = Js::Tick::Now() - start;

        // use separate events for perftrack specific data & general telemetry data
        if (this->ShouldTransmitPerfTrackEvents())
        {
            if (this->hostInterface->TransmitHeapUsage(bucketReporter.GetTotalStats()->totalByteCount, bucketReporter.GetTotalStats()->objectByteCount, bucketReporter.GetTotalStats()->UsedRatio()))
            {
                this->ResetPerfTrackCounts();
            }
        }

        if (this->ShouldTransmitGCStats() && this->hostInterface != nullptr)
        {
            if (this->hostInterface->TransmitGCTelemetryStats(*this))
            {
                this->lastTransmitTime = lastPassStats->passEndTimeTick;
                Reset();
            }
        }
    }

    void RecyclerTelemetryInfo::ResetPerfTrackCounts()
    {
        this->perfTrackPassCount = 0;
    }

    void RecyclerTelemetryInfo::Reset()
    {
        FreeGCPassStats();
        memset((void*)&this->threadPageAllocator_decommitStats, 0, sizeof(AllocatorDecommitStats));
        memset((void*)&this->recyclerLeafPageAllocator_decommitStats, 0, sizeof(AllocatorDecommitStats));
        memset((void*)&this->recyclerLargeBlockPageAllocator_decommitStats, 0, sizeof(AllocatorDecommitStats));
        memset((void*)&this->threadPageAllocator_decommitStats, 0, sizeof(AllocatorDecommitStats));
    }

    void RecyclerTelemetryInfo::FreeGCPassStats()
    {
        if (this->gcPassStats.Empty() == false)
        {
            AssertMsg(this->passCount > 0, "unexpected value for passCount");
            AssertOnValidThread(this, RecyclerTelemetryInfo::FreeGCPassStats);
            this->gcPassStats.Clear();
            this->passCount = 0;
        }
    }

    bool RecyclerTelemetryInfo::ShouldTransmitGCStats() const
    {
        // for now, try to transmit telemetry when we have >= 16
        return (this->hostInterface != nullptr &&  this->passCount >= 16);
    }

    bool RecyclerTelemetryInfo::ShouldTransmitPerfTrackEvents() const
    {
        // for now, try to transmit telemetry when we have >= 16
        return (this->hostInterface != nullptr &&  this->perfTrackPassCount >= 128);
    }


    void RecyclerTelemetryInfo::IncrementUserThreadBlockedCount(Js::TickDelta waitTime, RecyclerWaitReason caller)
    {
        RecyclerTelemetryGCPassStats* lastPassStats = this->GetLastPassStats();
#ifdef DBG
        if (this->inPassActiveState)
        {
            AssertMsg(lastPassStats != nullptr && lastPassStats->isGCPassActive == true, "unexpected Value in  RecyclerTelemetryInfo::IncrementUserThreadBlockedCount");
        }
#endif

        if (this->inPassActiveState && lastPassStats != nullptr)
        {
            AssertOnValidThread(this, RecyclerTelemetryInfo::IncrementUserThreadBlockedCount);
            lastPassStats->uiThreadBlockedTimes[caller] += waitTime;
        }
    }


    void RecyclerTelemetryInfo::IncrementUserThreadBlockedCpuTimeUser(uint64 userMicroseconds, RecyclerWaitReason caller)
    {
        RecyclerTelemetryGCPassStats* lastPassStats = this->GetLastPassStats();
#ifdef DBG
        if (this->inPassActiveState)
        {
            AssertMsg(lastPassStats != nullptr && lastPassStats->isGCPassActive == true, "unexpected Value in  RecyclerTelemetryInfo::IncrementUserThreadBlockedCpuTimeUser");
        }
#endif

        if (this->inPassActiveState && lastPassStats != nullptr)
        {
            AssertOnValidThread(this, RecyclerTelemetryInfo::IncrementUserThreadBlockedCpuTimeUser);
            lastPassStats->uiThreadBlockedCpuTimesUser[caller] += userMicroseconds;
        }
    }

    void RecyclerTelemetryInfo::IncrementUserThreadBlockedCpuTimeKernel(uint64 kernelMicroseconds, RecyclerWaitReason caller)
    {
        RecyclerTelemetryGCPassStats* lastPassStats = this->GetLastPassStats();
#ifdef DBG
        if (this->inPassActiveState)
        {
            AssertMsg(lastPassStats != nullptr && lastPassStats->isGCPassActive == true, "unexpected Value in  RecyclerTelemetryInfo::IncrementUserThreadBlockedCpuTimeKernel");
        }
#endif

        if (this->inPassActiveState && lastPassStats != nullptr)
        {
            AssertOnValidThread(this, RecyclerTelemetryInfo::IncrementUserThreadBlockedCpuTimeKernel);
            lastPassStats->uiThreadBlockedCpuTimesKernel[caller] += kernelMicroseconds;
        }
    }

    bool RecyclerTelemetryInfo::IsOnScriptThread() const
    {
        bool isValid = false;
        if (this->hostInterface != nullptr)
        {
            if (this->hostInterface->IsThreadBound())
            {
                isValid = ::GetCurrentThreadId() == this->hostInterface->GetCurrentScriptThreadID();
            }
            else
            {
                isValid = ::GetCurrentThreadId() == this->mainThreadID;
            }
        }
        return isValid;
    }

    size_t RecyclerTelemetryInfo::GetProcessCommittedBytes()
    {
        HANDLE process = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS memCounters = { 0 };
        size_t committedBytes = 0;
        if (GetProcessMemoryInfo(process, &memCounters, sizeof(PROCESS_MEMORY_COUNTERS)))
        {
            committedBytes = memCounters.PagefileUsage;
        }
        return committedBytes;
    }

}
#endif
