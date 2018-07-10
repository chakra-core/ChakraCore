//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "CommonDefines.h"
#include "CollectionFlags.h"
#include "RecyclerWaitReason.h"
#include "Common/Tick.h"
#include "AllocatorTelemetryStats.h"
#include "HeapBucketStats.h"
#include "DataStructures/SList.h"

#ifdef ENABLE_BASIC_TELEMETRY
#include "Windows.h"
#endif

namespace Memory
{
    class Recycler;
    class IdleDecommitPageAllocator;
    class RecyclerTelemetryInfo;

    // struct that defines an interface with host on how we can get key data stats
    class RecyclerTelemetryHostInterface
    {
    public:
        virtual LPFILETIME GetLastScriptExecutionEndTime() const = 0;
        virtual bool TransmitGCTelemetryStats(RecyclerTelemetryInfo& rti) = 0;
        virtual bool TransmitTelemetryError(const RecyclerTelemetryInfo& rti, const char* msg) = 0;
        virtual bool TransmitHeapUsage(size_t totalHeapBytes, size_t usedHeapBytes, double heapUsedRatio) = 0;
        virtual bool IsTelemetryProviderEnabled() const = 0;
        virtual bool IsThreadBound() const = 0;
        virtual DWORD GetCurrentScriptThreadID() const = 0;
        virtual uint GetClosedContextCount() const = 0;
    };

#ifdef ENABLE_BASIC_TELEMETRY

    /**
     *  struct with all data we want to capture for a specific GC pass.
     *
     *  This forms a linked list, one for each per-stats pass.
     *  The last element in the list points to the head instead of nullptr.
     */
    struct RecyclerTelemetryGCPassStats
    {
        FILETIME passStartTimeFileTime;
        CollectionState startPassCollectionState;
        CollectionState endPassCollectionState;
        ETWEventGCActivationTrigger collectionStartReason;
        ETWEventGCActivationTrigger collectionFinishReason;
        CollectionFlags collectionStartFlags;
        Js::Tick passStartTimeTick;
        Js::Tick passEndTimeTick;
        Js::TickDelta startPassProcessingElapsedTime;
        Js::TickDelta endPassProcessingElapsedTime;
        Js::TickDelta computeBucketStatsElapsedTime;
        FILETIME lastScriptExecutionEndTime;
        Js::TickDelta uiThreadBlockedTimes[static_cast<size_t>(RecyclerWaitReason::Other) + 1];
        uint64 uiThreadBlockedCpuTimesUser[static_cast<size_t>(RecyclerWaitReason::Other) + 1];
        uint64 uiThreadBlockedCpuTimesKernel[static_cast<size_t>(RecyclerWaitReason::Other) + 1];
        bool isInScript;
        bool isScriptActive;
        bool isGCPassActive;
        uint closedContextCount;
        uint pinnedObjectCount;

        size_t processAllocaterUsedBytes_start;
        size_t processAllocaterUsedBytes_end;
        size_t processCommittedBytes_start;
        size_t processCommittedBytes_end;

        HeapBucketStats bucketStats;

        AllocatorSizes threadPageAllocator_start;
        AllocatorSizes threadPageAllocator_end;
        AllocatorSizes recyclerLeafPageAllocator_start;
        AllocatorSizes recyclerLeafPageAllocator_end;
        AllocatorSizes recyclerLargeBlockPageAllocator_start;
        AllocatorSizes recyclerLargeBlockPageAllocator_end;

#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
        AllocatorSizes recyclerWithBarrierPageAllocator_start;
        AllocatorSizes recyclerWithBarrierPageAllocator_end;
#endif
    };

    /**
     * Consolidated summary of data from RecyclerFlagsTable that we want to
     * transmit via telemetry. Goal is to pack this into maximum of 64-bits.
     */
    enum RecyclerFlagsTableSummary : uint32
    {
        None                                 = 0x0000,
        IsMemProtectMode                     = 0x0001,
        IsConcurrentEnabled                  = 0x0002,
        EnableScanInteriorPointers           = 0x0004,
        EnableScanImplicitRoots              = 0x0008,
        DisableCollectOnAllocationHeuristics = 0x0016,
        RecyclerStress                       = 0x0032,
        RecyclerBackgroundStress             = 0x0064,
        RecyclerConcurrentStress             = 0x0128,
        RecyclerConcurrentRepeatStress       = 0x0256,
        RecyclerPartialStress                = 0x0512,
    };

    typedef SList<RecyclerTelemetryGCPassStats, HeapAllocator> GCPassStatsList;

    /**
     *
     */
    class RecyclerTelemetryInfo
    {
    public:
        RecyclerTelemetryInfo(Recycler* recycler, RecyclerTelemetryHostInterface* hostInterface);
        ~RecyclerTelemetryInfo();

        void StartPass(CollectionState collectionState);
        void EndPass(CollectionState collectionState);
        void IncrementUserThreadBlockedCount(Js::TickDelta waitTime, RecyclerWaitReason source);
        void IncrementUserThreadBlockedCpuTimeUser(uint64 userMicroseconds, RecyclerWaitReason caller);
        void IncrementUserThreadBlockedCpuTimeKernel(uint64 kernelMicroseconds, RecyclerWaitReason caller);

        inline const Js::Tick& GetRecyclerStartTime() const { return this->recyclerStartTime;  }
        RecyclerTelemetryGCPassStats* GetLastPassStats() const;
        inline const Js::Tick& GetLastTransmitTime() const { return this->lastTransmitTime; }
        inline const uint16 GetPassCount() const { return this->passCount; }
        const GUID& GetRecyclerID() const;
        bool IsOnScriptThread() const;
        GCPassStatsList::Iterator GetGCPassStatsIterator() const;
        RecyclerFlagsTableSummary GetRecyclerConfigFlags() const;

        AllocatorDecommitStats* GetThreadPageAllocator_decommitStats() { return &this->threadPageAllocator_decommitStats; }
        AllocatorDecommitStats* GetRecyclerLeafPageAllocator_decommitStats() { return &this->recyclerLeafPageAllocator_decommitStats; }
        AllocatorDecommitStats* GetRecyclerLargeBlockPageAllocator_decommitStats() { return &this->recyclerLargeBlockPageAllocator_decommitStats; }
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
        AllocatorDecommitStats* GetRecyclerWithBarrierPageAllocator_decommitStats() { return &this->recyclerWithBarrierPageAllocator_decommitStats; }
#endif

        bool ShouldStartTelemetryCapture() const;

    private:
        Recycler* recycler;
        DWORD mainThreadID;
        RecyclerTelemetryHostInterface * hostInterface;
        Js::Tick recyclerStartTime;
        bool inPassActiveState;

        GCPassStatsList gcPassStats;
        Js::Tick lastTransmitTime;
        uint16 passCount;
        uint16 perfTrackPassCount;
        bool abortTelemetryCapture;

        AllocatorDecommitStats threadPageAllocator_decommitStats;
        AllocatorDecommitStats recyclerLeafPageAllocator_decommitStats;
        AllocatorDecommitStats recyclerLargeBlockPageAllocator_decommitStats;
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
        AllocatorDecommitStats recyclerWithBarrierPageAllocator_decommitStats;
#endif

        bool ShouldTransmitGCStats() const;
        void FreeGCPassStats();
        void Reset();
        void FillInSizeData(IdleDecommitPageAllocator* allocator, AllocatorSizes* sizes) const;

        void ResetPerfTrackCounts();
        bool ShouldTransmitPerfTrackEvents() const;

        static size_t GetProcessCommittedBytes();
    };
#else
    class RecyclerTelemetryInfo
    {
    };
#endif // #ifdef ENABLE_BASIC_TELEMETRY

}

