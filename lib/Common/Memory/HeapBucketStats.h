//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "CommonDefines.h"

namespace Memory
{
#if ENABLE_MEM_STATS
    struct MemStats
    {
        size_t objectByteCount;
        size_t totalByteCount;

        MemStats();

        void Reset();
        size_t FreeBytes() const;
        double UsedRatio() const;
        void Aggregate(const MemStats& other);
    };

    struct HeapBucketStats : MemStats
    {
#ifdef DUMP_FRAGMENTATION_STATS
        uint totalBlockCount;
        uint objectCount;
        uint finalizeCount;

        HeapBucketStats();
        void Reset();
        void Dump() const;
#endif

        void PreAggregate();
        void BeginAggregate();
        void Aggregate(const HeapBucketStats& other);
    };
#endif

}
