//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

#if ENABLE_MEM_STATS
MemStats::MemStats()
    : objectByteCount(0), totalByteCount(0)
{}

void MemStats::Reset()
{
    objectByteCount = 0;
    totalByteCount = 0;
}

size_t MemStats::FreeBytes() const
{
    return totalByteCount - objectByteCount;
}

double MemStats::UsedRatio() const
{
    return (double)objectByteCount / totalByteCount;
}

void MemStats::Aggregate(const MemStats& other)
{
    objectByteCount += other.objectByteCount;
    totalByteCount += other.totalByteCount;
}

#ifdef DUMP_FRAGMENTATION_STATS
HeapBucketStats::HeapBucketStats()
    : totalBlockCount(0), objectCount(0), finalizeCount(0)
{}

void HeapBucketStats::Reset()
{
    MemStats::Reset();
    totalBlockCount = 0;
    objectCount = 0;
    finalizeCount = 0;
}

void HeapBucketStats::Dump() const
{
    Output::Print(_u("%5d %7d %7d %11lu %11lu %11lu   %6.2f%%\n"),
        totalBlockCount, objectCount, finalizeCount,
        static_cast<ULONG>(objectByteCount),
        static_cast<ULONG>(FreeBytes()),
        static_cast<ULONG>(totalByteCount),
        UsedRatio() * 100);
}
#endif

void HeapBucketStats::PreAggregate()
{
    // When first enter Pre-Aggregate state, clear data and mark state.
    if (!(totalByteCount & 1))
    {
        Reset();
        totalByteCount |= 1;
    }
}

void HeapBucketStats::BeginAggregate()
{
    // If was Pre-Aggregate state, keep data and clear state
    if (totalByteCount & 1)
    {
        totalByteCount &= ~1;
    }
    else
    {
        Reset();
    }
}

void HeapBucketStats::Aggregate(const HeapBucketStats& other)
{
    MemStats::Aggregate(other);
#ifdef DUMP_FRAGMENTATION_STATS
    totalBlockCount += other.totalBlockCount;
    objectCount += other.objectCount;
    finalizeCount += other.finalizeCount;
#endif
}
#endif  // ENABLE_MEM_STATS
