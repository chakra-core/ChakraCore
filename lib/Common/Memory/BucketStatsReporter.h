//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "HeapBucketStats.h"

namespace Memory
{

#ifdef DUMP_FRAGMENTATION_STATS
template <ObjectInfoBits TBucketType>
struct DumpBucketTypeName { static char16 name[]; };
template<> char16 DumpBucketTypeName<NoBit>::name[] = _u("Normal ");
template<> char16 DumpBucketTypeName<LeafBit>::name[] = _u("Leaf   ");
template<> char16 DumpBucketTypeName<FinalizeBit>::name[] = _u("Fin    ");
#ifdef RECYCLER_WRITE_BARRIER
template<> char16 DumpBucketTypeName<WithBarrierBit>::name[] = _u("NormWB ");
template<> char16 DumpBucketTypeName<FinalizableWithBarrierBit>::name[] = _u("FinWB  ");
#endif
#ifdef RECYCLER_VISITED_HOST
template<> char16 DumpBucketTypeName<RecyclerVisitedHostBit>::name[] = _u("Visited");
#endif
template <typename TBlockType>
struct DumpBlockTypeName { static char16 name[]; };
template<> char16 DumpBlockTypeName<SmallAllocationBlockAttributes>::name[] = _u("(S)");
template<> char16 DumpBlockTypeName<MediumAllocationBlockAttributes>::name[] = _u("(M)");
#endif  // DUMP_FRAGMENTATION_STATS

template <ObjectInfoBits TBucketType>
struct EtwBucketTypeEnum { static uint16 code; };
template<> uint16 EtwBucketTypeEnum<NoBit>::code = 0;
template<> uint16 EtwBucketTypeEnum<LeafBit>::code = 1;
template<> uint16 EtwBucketTypeEnum<FinalizeBit>::code = 2;
#ifdef RECYCLER_WRITE_BARRIER
template<> uint16 EtwBucketTypeEnum<WithBarrierBit>::code = 3;
template<> uint16 EtwBucketTypeEnum<FinalizableWithBarrierBit>::code = 4;
#endif
#ifdef RECYCLER_VISITED_HOST
template<> uint16 EtwBucketTypeEnum<RecyclerVisitedHostBit>::code = 5;
#endif
template <typename TBlockType>
struct EtwBlockTypeEnum { static uint16 code; };
template<> uint16 EtwBlockTypeEnum<SmallAllocationBlockAttributes>::code = 0;
template<> uint16 EtwBlockTypeEnum<MediumAllocationBlockAttributes>::code = 1;

class BucketStatsReporter
{
private:
    static const uint16 LargeBucketNameCode = 2 << 8;
    static const uint16 TotalBucketNameCode = 3 << 8;

    Recycler* recycler;
    HeapBucketStats total;

    template <class TBlockAttributes, ObjectInfoBits TBucketType>
    uint16 BucketNameCode() const
    {
        return EtwBucketTypeEnum<TBucketType>::code + (EtwBlockTypeEnum<TBlockAttributes>::code << 8);
    }

    bool IsMemProtectMode() const
    {
        return recycler->IsMemProtectMode();
    }

public:
    BucketStatsReporter(Recycler* recycler)
        : recycler(recycler)
    {
        DUMP_FRAGMENTATION_STATS_ONLY(DumpHeader());
    }

    HeapBucketStats* GetTotalStats() { return &total; }

    bool IsEtwEnabled() const
    {
        return IS_GCETW_Enabled(GC_BUCKET_STATS);
    }

    bool IsDumpEnabled() const
    {
        return DUMP_FRAGMENTATION_STATS_IS(!!recycler->GetRecyclerFlagsTable().DumpFragmentationStats);
    }

    bool IsEnabled() const
    {
        return IsEtwEnabled() || IsDumpEnabled();
    }

    template <class TBlockType>
    void PreAggregateBucketStats(TBlockType* list)
    {
        HeapBlockList::ForEach(list, [](TBlockType* heapBlock)
        {
            // Process blocks not in allocator in pre-pass. They are not put into buckets yet.
            if (!heapBlock->IsInAllocator())
            {
                heapBlock->heapBucket->PreAggregateBucketStats(heapBlock);
            }
        });
    }

    template <class TBlockAttributes, ObjectInfoBits TBucketType>
    void GetBucketStats(HeapBucketGroup<TBlockAttributes>& group)
    {
        auto& bucket = group.template GetBucket<TBucketType>();
        bucket.AggregateBucketStats();

        const auto& stats = bucket.GetMemStats();
        total.Aggregate(stats);

        if (stats.totalByteCount > 0)
        {
            const uint16 bucketNameCode = BucketNameCode<TBlockAttributes, TBucketType>();
            const uint16 sizeCat = static_cast<uint16>(bucket.GetSizeCat());
            GCETW(GC_BUCKET_STATS, (recycler, bucketNameCode, sizeCat, stats.objectByteCount, stats.totalByteCount));
#ifdef DUMP_FRAGMENTATION_STATS
            DumpStats<TBlockAttributes, TBucketType>(sizeCat, stats);
#endif
        }
    }

    void GetBucketStats(LargeHeapBucket& bucket)
    {
        bucket.AggregateBucketStats();

        const auto& stats = bucket.GetMemStats();
        total.Aggregate(stats);

        if (stats.totalByteCount > 0)
        {
            const uint16 sizeCat = static_cast<uint16>(bucket.GetSizeCat());
            GCETW(GC_BUCKET_STATS, (recycler, LargeBucketNameCode, sizeCat, stats.objectByteCount, stats.totalByteCount));
            DUMP_FRAGMENTATION_STATS_ONLY(DumpLarge(stats));
        }
    }

    void Report()
    {
        GCETW(GC_BUCKET_STATS, (recycler, TotalBucketNameCode, 0, total.objectByteCount, total.totalByteCount));
        DUMP_FRAGMENTATION_STATS_ONLY(DumpFooter());
    }

#ifdef DUMP_FRAGMENTATION_STATS
    void DumpHeader()
    {
        if (IsDumpEnabled())
        {
            Output::Print(_u("[FRAG %d] Post-Collection State\n"), ::GetTickCount());
            Output::Print(_u("---------------------------------------------------------------------------------------\n"));
            Output::Print(_u("                  #Blk   #Objs    #Fin     ObjBytes   FreeBytes  TotalBytes UsedPercent\n"));
            Output::Print(_u("---------------------------------------------------------------------------------------\n"));
        }
    }

    template <class TBlockAttributes, ObjectInfoBits TBucketType>
    void DumpStats(uint sizeCat, const HeapBucketStats& stats)
    {
        if (IsDumpEnabled())
        {
            Output::Print(_u("%-7s%s %4d : "),
                DumpBucketTypeName<TBucketType>::name, DumpBlockTypeName<TBlockAttributes>::name, sizeCat);
            stats.Dump();
        }
    }

    void DumpLarge(const HeapBucketStats& stats)
    {
        if (IsDumpEnabled())
        {
            Output::Print(_u("Large           : "));
            stats.Dump();
        }
    }

    void DumpFooter()
    {
        if (IsDumpEnabled())
        {
            Output::Print(_u("---------------------------------------------------------------------------------------\n"));
            Output::Print(_u("Total           : "));
            total.Dump();
        }
    }
#endif
};

};
