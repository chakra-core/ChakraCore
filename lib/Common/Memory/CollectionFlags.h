//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Memory
{

    enum CollectionFlags
    {
        CollectHeuristic_AllocSize = 0x00000001,
        CollectHeuristic_Time = 0x00000002,
        CollectHeuristic_TimeIfScriptActive = 0x00000004,
        CollectHeuristic_TimeIfInScript = 0x00000008,
        CollectHeuristic_Never = 0x00000080,
        CollectHeuristic_Mask = 0x000000FF,

        CollectOverride_FinishConcurrent = 0x00001000,
        CollectOverride_ExhaustiveCandidate = 0x00002000,
        CollectOverride_ForceInThread = 0x00004000,
        CollectOverride_AllowDispose = 0x00008000,
        CollectOverride_AllowReentrant = 0x00010000,
        CollectOverride_ForceFinish = 0x00020000,
        CollectOverride_Explicit = 0x00040000,
        CollectOverride_DisableIdleFinish = 0x00080000,
        CollectOverride_BackgroundFinishMark = 0x00100000,
        CollectOverride_FinishConcurrentTimeout = 0x00200000,
        CollectOverride_NoExhaustiveCollect = 0x00400000,
        CollectOverride_SkipStack = 0x01000000,
        CollectOverride_CheckScriptContextClose = 0x02000000,
        CollectMode_Partial = 0x08000000,
        CollectMode_Concurrent = 0x10000000,
        CollectMode_Exhaustive = 0x20000000,
        CollectMode_DecommitNow = 0x40000000,
        CollectMode_CacheCleanup = 0x80000000,

        CollectNowForceInThread = CollectOverride_ForceInThread,
        CollectNowForceInThreadExternal = CollectOverride_ForceInThread | CollectOverride_AllowDispose,
        CollectNowForceInThreadExternalNoStack = CollectOverride_ForceInThread | CollectOverride_AllowDispose | CollectOverride_SkipStack,
        CollectNowDefault = CollectOverride_FinishConcurrent,
        CollectNowDefaultLSCleanup = CollectOverride_FinishConcurrent | CollectOverride_AllowDispose,
        CollectNowDecommitNowExplicit = CollectNowDefault | CollectMode_DecommitNow | CollectMode_CacheCleanup | CollectOverride_Explicit | CollectOverride_AllowDispose,
        CollectNowConcurrent = CollectOverride_FinishConcurrent | CollectMode_Concurrent,
        CollectNowExhaustive = CollectOverride_FinishConcurrent | CollectMode_Exhaustive | CollectOverride_AllowDispose,
        CollectNowPartial = CollectOverride_FinishConcurrent | CollectMode_Partial,
        CollectNowConcurrentPartial = CollectMode_Concurrent | CollectNowPartial,

        CollectOnAllocation = CollectHeuristic_AllocSize | CollectHeuristic_Time | CollectMode_Concurrent | CollectMode_Partial | CollectOverride_FinishConcurrent | CollectOverride_AllowReentrant | CollectOverride_FinishConcurrentTimeout,
        CollectOnTypedArrayAllocation = CollectHeuristic_AllocSize | CollectHeuristic_Time | CollectMode_Concurrent | CollectMode_Partial | CollectOverride_FinishConcurrent | CollectOverride_AllowReentrant | CollectOverride_FinishConcurrentTimeout | CollectOverride_AllowDispose,
        CollectOnScriptIdle = CollectOverride_CheckScriptContextClose | CollectOverride_FinishConcurrent | CollectMode_Concurrent | CollectMode_CacheCleanup | CollectOverride_SkipStack,
        CollectOnScriptExit = CollectOverride_CheckScriptContextClose | CollectHeuristic_AllocSize | CollectOverride_FinishConcurrent | CollectMode_Concurrent | CollectMode_CacheCleanup,
        CollectExhaustiveCandidate = CollectHeuristic_Never | CollectOverride_ExhaustiveCandidate,
        CollectOnScriptCloseNonPrimary = CollectNowConcurrent | CollectOverride_ExhaustiveCandidate | CollectOverride_AllowDispose,
        CollectOnRecoverFromOutOfMemory = CollectOverride_ForceInThread | CollectMode_DecommitNow,
        CollectOnSuspendCleanup = CollectNowConcurrent | CollectMode_Exhaustive | CollectMode_DecommitNow | CollectOverride_DisableIdleFinish,

        FinishConcurrentOnIdle = CollectMode_Concurrent | CollectOverride_DisableIdleFinish,
        FinishConcurrentOnIdleAtRoot = CollectMode_Concurrent | CollectOverride_DisableIdleFinish | CollectOverride_SkipStack,
        FinishConcurrentDefault = CollectMode_Concurrent | CollectOverride_DisableIdleFinish | CollectOverride_BackgroundFinishMark,
        FinishConcurrentOnExitScript = FinishConcurrentDefault,
        FinishConcurrentOnEnterScript = FinishConcurrentDefault,
        FinishConcurrentOnAllocation = FinishConcurrentDefault,
        FinishDispose = CollectOverride_AllowDispose,
        FinishDisposeTimed = CollectOverride_AllowDispose | CollectHeuristic_TimeIfScriptActive,
        ForceFinishCollection = CollectOverride_ForceFinish | CollectOverride_ForceInThread,

#ifdef RECYCLER_STRESS
        CollectStress = CollectNowForceInThread,
#if ENABLE_PARTIAL_GC
        CollectPartialStress = CollectMode_Partial,
#endif
#if ENABLE_CONCURRENT_GC
        CollectBackgroundStress = CollectNowDefault,
        CollectConcurrentStress = CollectNowConcurrent,
#if ENABLE_PARTIAL_GC
        CollectConcurrentPartialStress = CollectConcurrentStress | CollectPartialStress,
#endif
#endif
#endif

#if defined(CHECK_MEMORY_LEAK) || defined(LEAK_REPORT)
        CollectNowFinalGC = CollectNowExhaustive | CollectOverride_ForceInThread | CollectOverride_SkipStack | CollectOverride_Explicit | CollectOverride_AllowDispose,
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        CollectNowExhaustiveSkipStack = CollectNowExhaustive | CollectOverride_SkipStack, // Used by test
#endif
    };

// NOTE: There is perf lab test infrastructure that takes a dependency on the events in this enumeration. Any modifications may cause
// errors in ETL analysis or report incorrect numbers. Please verify that the GC events are analyzed correctly with your changes.
enum ETWEventGCActivationKind : unsigned
{
    ETWEvent_GarbageCollect = 0,      // force in-thread GC
    ETWEvent_ThreadCollect = 1,      // thread GC with wait
    ETWEvent_ConcurrentCollect = 2,
    ETWEvent_PartialCollect = 3,

    ETWEvent_ConcurrentMark = 11,
    ETWEvent_ConcurrentRescan = 12,
    ETWEvent_ConcurrentSweep = 13,
    ETWEvent_ConcurrentTransferSwept = 14,
    ETWEvent_ConcurrentFinishMark = 15,
    ETWEvent_ConcurrentSweep_TwoPassSweepPreCheck = 16,     // Check whether we should do a 2-pass concurrent sweep.

    // The following events are only relevant to the 2-pass concurrent sweep and should not be seen otherwise.
    ETWEvent_ConcurrentSweep_Pass1 = 17,     // Concurrent sweep Pass1 of the blocks not getting allocated from during concurrent sweep.
    ETWEvent_ConcurrentSweep_FinishSweepPrep = 18,     // Stop allocations and remove all blocks from SLIST so we can finish Pass1 of the remaining blocks.
    ETWEvent_ConcurrentSweep_FinishPass1 = 19,     // Concurrent sweep Pass1 of the blocks that were set aside for allocations during concurrent sweep.
    ETWEvent_ConcurrentSweep_Pass2 = 20,     // Concurrent sweep Pass1 of the blocks not getting allocated from during concurrent sweep.
    ETWEvent_ConcurrentSweep_FinishTwoPassSweep = 21,     // Drain the SLIST at the end of the 2-pass concurrent sweep and begin normal allocations.
};

#define IS_UNKNOWN_GC_TRIGGER(v)    (v == ETWEvent_GC_Trigger_Unknown)

enum ETWEventGCActivationTrigger : unsigned
{
    ETWEvent_GC_Trigger_Unknown = 0,
    ETWEvent_GC_Trigger_IdleCollect = 1,
    ETWEvent_GC_Trigger_Partial_GC_AllocSize_Heuristic = 2,
    ETWEvent_GC_Trigger_TimeAndAllocSize_Heuristic = 3,
    ETWEvent_GC_Trigger_TimeAndAllocSizeIfScriptActive_Heuristic = 4,
    ETWEvent_GC_Trigger_TimeAndAllocSizeIfInScript_Heuristic = 5,
    ETWEvent_GC_Trigger_NoHeuristic = 6,
    ETWEvent_GC_Trigger_Status_Completed = 7,
    ETWEvent_GC_Trigger_Status_StartedConcurrent = 8,
    ETWEvent_GC_Trigger_Status_Failed = 9,
    ETWEvent_GC_Trigger_Status_FailedTimeout = 10
};

}
