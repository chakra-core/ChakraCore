//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonMemoryPch.h"


#ifdef ENABLE_BASIC_TELEMETRY

#include "Recycler.h"
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
        hostInterface(hostInterface),
        lastPassStats(nullptr),
        recyclerStartTime(Js::Tick::Now()),
        abortTelemetryCapture(false)
    {
        this->recycler = recycler;
        mainThreadID = ::GetCurrentThreadId();
    }

    RecyclerTelemetryInfo::~RecyclerTelemetryInfo()
    {
        AssertOnValidThread(this, RecyclerTelemetryInfo::~RecyclerTelemetryInfo);
        if (this->hostInterface != nullptr && this->passCount > 0)
        {
            this->hostInterface->TransmitTelemetry(*this);
        }
        this->FreeGCPassStats();
    }

    const GUID& RecyclerTelemetryInfo::GetRecyclerID() const
    {
        return this->recycler->GetRecyclerID();
    }

    bool RecyclerTelemetryInfo::GetIsConcurrentEnabled() const
    {
        return this->recycler->IsConcurrentEnabled();
    }

    bool RecyclerTelemetryInfo::ShouldCaptureRecyclerTelemetry() const
    {
        return this->hostInterface != nullptr && this->abortTelemetryCapture == false;
    }

    void RecyclerTelemetryInfo::FillInSizeData(IdleDecommitPageAllocator* allocator, AllocatorSizes* sizes) const
    {
        sizes->committedBytes = allocator->GetCommittedBytes();
        sizes->reservedBytes = allocator->GetReservedBytes();
        sizes->usedBytes = allocator->GetUsedBytes();
        sizes->numberOfSegments = allocator->GetNumberOfSegments();
    }

    void RecyclerTelemetryInfo::StartPass()
    {
        Js::Tick start = Js::Tick::Now();
        if (!this->ShouldCaptureRecyclerTelemetry())
        {
            return;
        }

        AssertOnValidThread(this, RecyclerTelemetryInfo::StartPass);
#if DBG
        // validate state of existing GC pass stats structs
        uint16 count = 0;
        if (this->lastPassStats != nullptr)
        {
            RecyclerTelemetryGCPassStats* head = this->lastPassStats->next;
            RecyclerTelemetryGCPassStats* curr = head;
            do
            {
                AssertMsg(curr->isGCPassActive == false, "unexpected value for isGCPassActive");
                count++;
                curr = curr->next;
            } while (curr != head);
        }
        AssertMsg(count == this->passCount, "RecyclerTelemetryInfo::StartPass() - mismatch between passCount and count.");
#endif

        RecyclerTelemetryGCPassStats* p = HeapNewNoThrow(RecyclerTelemetryGCPassStats);
        if (p == nullptr)
        {
            // failed to allocate memory - disable any further telemetry capture for this recycler 
            // and free any existing GC stats we've accumulated
            this->abortTelemetryCapture = true;
            FreeGCPassStats();
            this->hostInterface->TransmitTelemetryError(*this, "Memory Allocation Failed");
        }
        else
        {
            passCount++;
            memset(p, 0, sizeof(RecyclerTelemetryGCPassStats));
            if (this->lastPassStats == nullptr)
            {
                p->next = p;
            }
            else
            {
                p->next = lastPassStats->next;
                this->lastPassStats->next = p;
            }
            this->lastPassStats = p;

            this->lastPassStats->isGCPassActive = true;
            this->lastPassStats->passStartTimeTick = Js::Tick::Now();
            GetSystemTimePreciseAsFileTime(&this->lastPassStats->passStartTimeFileTime);
            if (this->hostInterface != nullptr)
            {
                LPFILETIME ft = this->hostInterface->GetLastScriptExecutionEndTime();
                this->lastPassStats->lastScriptExecutionEndTime = *ft;
            }

            this->lastPassStats->processCommittedBytes_start = RecyclerTelemetryInfo::GetProcessCommittedBytes();
            this->lastPassStats->processAllocaterUsedBytes_start = PageAllocator::GetProcessUsedBytes();
            this->lastPassStats->isInScript = this->recycler->GetIsInScript();
            this->lastPassStats->isScriptActive = this->recycler->GetIsScriptActive();

            this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerLeafPageAllocator(), &this->lastPassStats->threadPageAllocator_start);
            this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerPageAllocator(), &this->lastPassStats->recyclerLeafPageAllocator_start);
            this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerLargeBlockPageAllocator(), &this->lastPassStats->recyclerLargeBlockPageAllocator_start);
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
            this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerWithBarrierPageAllocator(), &this->lastPassStats->recyclerWithBarrierPageAllocator_start);
#endif
            this->lastPassStats->startPassProcessingElapsedTime = Js::Tick::Now() - start;
        }


    }

    void RecyclerTelemetryInfo::EndPass()
    {
        if (!this->ShouldCaptureRecyclerTelemetry())
        {
            return;
        }

        Js::Tick start = Js::Tick::Now();

        AssertOnValidThread(this, RecyclerTelemetryInfo::EndPass);

        this->lastPassStats->isGCPassActive = false;
        this->lastPassStats->passEndTimeTick = Js::Tick::Now();

        this->lastPassStats->processCommittedBytes_end = RecyclerTelemetryInfo::GetProcessCommittedBytes();
        this->lastPassStats->processAllocaterUsedBytes_end = PageAllocator::GetProcessUsedBytes();

        this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerLeafPageAllocator(), &this->lastPassStats->threadPageAllocator_end);
        this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerPageAllocator(), &this->lastPassStats->recyclerLeafPageAllocator_end);
        this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerLargeBlockPageAllocator(), &this->lastPassStats->recyclerLargeBlockPageAllocator_end);
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
        this->FillInSizeData(this->recycler->GetHeapInfo()->GetRecyclerWithBarrierPageAllocator(), &this->lastPassStats->recyclerWithBarrierPageAllocator_end);
#endif

        // get bucket stats
        Js::Tick bucketStatsStart = Js::Tick::Now();
        BucketStatsReporter bucketReporter(this->recycler);
        this->recycler->GetHeapInfo()->GetBucketStats(bucketReporter);
        memcpy(&this->lastPassStats->bucketStats, bucketReporter.GetTotalStats(), sizeof(HeapBucketStats));
        this->lastPassStats->computeBucketStatsElapsedTime = Js::Tick::Now() - bucketStatsStart;

        this->lastPassStats->endPassProcessingElapsedTime = Js::Tick::Now() - start;

        if (ShouldTransmit() && this->hostInterface != nullptr)
        {
            if (this->hostInterface->TransmitTelemetry(*this))
            {
                this->lastTransmitTime = this->lastPassStats->passEndTimeTick;
                Reset();
            }
        }
    }

    void RecyclerTelemetryInfo::Reset()
    {
        FreeGCPassStats();
        memset(&this->threadPageAllocator_decommitStats, 0, sizeof(AllocatorDecommitStats));
        memset(&this->recyclerLeafPageAllocator_decommitStats, 0, sizeof(AllocatorDecommitStats));
        memset(&this->recyclerLargeBlockPageAllocator_decommitStats, 0, sizeof(AllocatorDecommitStats));
        memset(&this->threadPageAllocator_decommitStats, 0, sizeof(AllocatorDecommitStats));
    }

    void RecyclerTelemetryInfo::FreeGCPassStats()
    {
        AssertOnValidThread(this, RecyclerTelemetryInfo::FreeGCPassStats);

        if (this->lastPassStats != nullptr)
        {
            RecyclerTelemetryGCPassStats* head = this->lastPassStats->next;
            RecyclerTelemetryGCPassStats* curr = head;
#ifdef DBG
            uint16 freeCount = 0;
#endif
            do
            {
                RecyclerTelemetryGCPassStats* next = curr->next;
                HeapDelete(curr);
                curr = next;
#ifdef DBG
                freeCount++;
#endif
            } while (curr != head);
#ifdef DBG
            AssertMsg(freeCount == this->passCount, "Unexpected mismatch between freeCount and passCount");
#endif
            this->lastPassStats = nullptr;
            this->passCount = 0;
        }
    }

    bool RecyclerTelemetryInfo::ShouldTransmit()
    {
        // for now, try to transmit telemetry when we have >= 16
        return (this->hostInterface != nullptr &&  this->passCount >= 16);
    }

    void RecyclerTelemetryInfo::IncrementUserThreadBlockedCount(Js::TickDelta waitTime, RecyclerWaitReason caller)
    {
#ifdef DBG
        if (this->ShouldCaptureRecyclerTelemetry())
        {
            AssertMsg(this->lastPassStats != nullptr && this->lastPassStats->isGCPassActive == true, "unexpected Value in  RecyclerTelemetryInfo::IncrementUserThreadBlockedCount");
        }
#endif

        if (this->ShouldCaptureRecyclerTelemetry() && this->lastPassStats != nullptr)
        {
            AssertOnValidThread(this, RecyclerTelemetryInfo::IncrementUserThreadBlockedCount);
            this->lastPassStats->uiThreadBlockedTimes[caller] += waitTime;
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
