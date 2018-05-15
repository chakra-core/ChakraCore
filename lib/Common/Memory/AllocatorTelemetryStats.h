//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "Common/Tick.h"

struct AllocatorDecommitStats
{
    Js::Tick lastLeaveDecommitRegion;
    Js::TickDelta maxDeltaBetweenDecommitRegionLeaveAndDecommit;
    int64 numDecommitCalls;
    int64 numPagesDecommitted;
    int64 numFreePageCount;
};

struct AllocatorSizes
{
    size_t usedBytes;
    size_t reservedBytes;
    size_t committedBytes;
    size_t numberOfSegments;
};