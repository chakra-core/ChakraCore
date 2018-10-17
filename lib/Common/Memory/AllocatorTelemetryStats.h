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

    // The following values correspond to when we last entered the critical section for Enter/Leave IdleDecommit and
    // how long we waited to enter the critical section if we had to wait.
    Js::Tick lastEnterLeaveIdleDecommitTick;
    Js::TickDelta lastEnterLeaveIdleDecommitCSWaitTime;
    Js::TickDelta maxEnterLeaveIdleDecommitCSWaitTime;
    Js::TickDelta totalEnterLeaveIdleDecommitCSWaitTime;

    int64 numDecommitCalls;
    int64 numPagesDecommitted;
    int64 numFreePageCount;

    AllocatorDecommitStats() :
        lastLeaveDecommitRegion(),
        maxDeltaBetweenDecommitRegionLeaveAndDecommit(0),
        lastEnterLeaveIdleDecommitTick(),
        lastEnterLeaveIdleDecommitCSWaitTime(0),
        maxEnterLeaveIdleDecommitCSWaitTime(0),
        totalEnterLeaveIdleDecommitCSWaitTime(0),
        numDecommitCalls(0),
        numPagesDecommitted(0),
        numFreePageCount(0)
    {}
};

struct AllocatorSizes
{
    size_t usedBytes;
    size_t reservedBytes;
    size_t committedBytes;
    size_t numberOfSegments;
};
