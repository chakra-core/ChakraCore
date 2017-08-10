//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "CollectionState.h"

namespace Js
{
    class Profiler;
    enum Phase: unsigned short;
};

namespace JsUtil
{
    class ThreadService;
};

#ifdef STACK_BACK_TRACE
class StackBackTraceNode;
#endif
class ScriptEngineBase;
class JavascriptThreadService;

#ifdef PROFILE_MEM
struct RecyclerMemoryData;
#endif

namespace Memory
{
template <typename T> class RecyclerRootPtr;

class AutoBooleanToggle
{
public:
    AutoBooleanToggle(bool * b, bool value = true, bool valueMayChange = false)
        : b(b)
    {
        Assert(!(*b));
        *b = value;
#if DBG
        this->value = value;
        this->valueMayChange = valueMayChange;
#endif
    }

    ~AutoBooleanToggle()
    {
        if (b)
        {
            Assert(valueMayChange || *b == value);
            *b = false;
        }
    }

    void Leave()
    {
        Assert(valueMayChange || *b == value);
        *b = false;
        b = nullptr;
    }

private:
    bool * b;
#if DBG
    bool value;
    bool valueMayChange;
#endif
};

template <class T>
class AutoRestoreValue
{
public:
    AutoRestoreValue(T* var, const T& val):
        variable(var)
    {
        Assert(var);
        oldValue = (*variable);
        (*variable) = val;
#ifdef DEBUG
        debugSetValue = val;
#endif
    }

    ~AutoRestoreValue()
    {
        Assert((*variable) == debugSetValue);
        (*variable) = oldValue;
    }

private:
#ifdef DEBUG
    T  debugSetValue;
#endif

    T* variable;
    T  oldValue;
};

class Recycler;

class RecyclerScanMemoryCallback
{
public:
    RecyclerScanMemoryCallback(Recycler* recycler) : recycler(recycler) {}
    void operator()(void** obj, size_t byteCount);
private:
    Recycler* recycler;
};

template<ObjectInfoBits infoBits>
struct InfoBitsWrapper{};


// Allocation macro

#define RecyclerNew(recycler,T,...) AllocatorNewBase(Recycler, recycler, AllocInlined, T, __VA_ARGS__)
#define RecyclerNewPlus(recycler,size,T,...) AllocatorNewPlus(Recycler, recycler, size, T, __VA_ARGS__)
#define RecyclerNewPlusZ(recycler,size,T,...) AllocatorNewPlusZ(Recycler, recycler, size, T, __VA_ARGS__)
#define RecyclerNewZ(recycler,T,...) AllocatorNewBase(Recycler, recycler, AllocZeroInlined, T, __VA_ARGS__)
#define RecyclerNewStruct(recycler,T) AllocatorNewStructBase(Recycler, recycler, AllocInlined, T)
#define RecyclerNewStructZ(recycler,T) AllocatorNewStructBase(Recycler, recycler, AllocZeroInlined, T)
#define RecyclerNewStructPlus(recycler,size,T) AllocatorNewStructPlus(Recycler, recycler, size, T)
#define RecyclerNewArray(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, Alloc, T, count)
#define RecyclerNewArrayZ(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, AllocZero, T, count)
#define RecyclerNewFinalized(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocFinalizedInlined, T, __VA_ARGS__)))
#define RecyclerNewFinalizedPlus(recycler, size, T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewPlusBase(Recycler, recycler, AllocFinalized, size, T, __VA_ARGS__)))
#define RecyclerNewTracked(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocTrackedInlined, T, __VA_ARGS__)))
#define RecyclerNewEnumClass(recycler, enumClass, T, ...) new (TRACK_ALLOC_INFO(static_cast<Recycler *>(recycler), T, Recycler, 0, (size_t)-1), InfoBitsWrapper<enumClass>()) T(__VA_ARGS__)
#define RecyclerNewWithInfoBits(recycler, infoBits, T, ...) new (TRACK_ALLOC_INFO(static_cast<Recycler *>(recycler), T, Recycler, 0, (size_t)-1), InfoBitsWrapper<infoBits>()) T(__VA_ARGS__)
#define RecyclerNewFinalizedClientTracked(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocFinalizedClientTrackedInlined, T, __VA_ARGS__)))

#if defined(RECYCLER_WRITE_BARRIER_ALLOC)
#define RecyclerNewWithBarrier(recycler,T,...) AllocatorNewBase(Recycler, recycler, AllocWithBarrier, T, __VA_ARGS__)
#define RecyclerNewWithBarrierPlus(recycler,size,T,...) AllocatorNewPlusBase(Recycler, recycler, AllocWithBarrier, size, T, __VA_ARGS__)
#define RecyclerNewWithBarrierPlusZ(recycler,size,T,...) AllocatorNewPlusBase(Recycler, recycler, AllocZeroWithBarrier, size, T, __VA_ARGS__)
#define RecyclerNewWithBarrierZ(recycler,T,...) AllocatorNewBase(Recycler, recycler, AllocZeroWithBarrier, T, __VA_ARGS__)
#define RecyclerNewWithBarrierStruct(recycler,T) AllocatorNewStructBase(Recycler, recycler, AllocWithBarrier, T)
#define RecyclerNewWithBarrierStructZ(recycler,T) AllocatorNewStructBase(Recycler, recycler, AllocZeroWithBarrier, T)
#define RecyclerNewWithBarrierStructPlus(recycler,size,T) AllocatorNewStructPlusBase(Recycler, recycler, AllocWithBarrier, size, T)
#define RecyclerNewWithBarrierArray(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, AllocWithBarrier, T, count)
#define RecyclerNewWithBarrierArrayZ(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, AllocZeroWithBarrier, T, count)
#define RecyclerNewWithBarrierFinalized(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocFinalizedWithBarrierInlined, T, __VA_ARGS__)))
#define RecyclerNewWithBarrierFinalizedPlus(recycler, size, T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewPlusBase(Recycler, recycler, AllocFinalizedWithBarrier, size, T, __VA_ARGS__)))
#define RecyclerNewWithBarrierTracked(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocTrackedWithBarrierInlined, T, __VA_ARGS__)))
#define RecyclerNewWithBarrierEnumClass(recycler, enumClass, T, ...) new (TRACK_ALLOC_INFO(static_cast<Recycler *>(recycler), T, Recycler, 0, (size_t)-1), InfoBitsWrapper<(ObjectInfoBits)(enumClass | WithBarrierBit)>()) T(__VA_ARGS__)
#define RecyclerNewWithBarrierWithInfoBits(recycler, infoBits, T, ...) new (TRACK_ALLOC_INFO(static_cast<Recycler *>(recycler), T, Recycler, 0, (size_t)-1), InfoBitsWrapper<(ObjectInfoBits)(infoBits | WithBarrierBit)>()) T(__VA_ARGS__)
#define RecyclerNewWithBarrierFinalizedClientTracked(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocFinalizedClientTrackedWithBarrierInlined, T, __VA_ARGS__)))
#endif

#ifndef RECYCLER_WRITE_BARRIER
#define RecyclerNewWithBarrier                          RecyclerNew
#define RecyclerNewWithBarrierPlus                      RecyclerNewPlus
#define RecyclerNewWithBarrierPlusZ                     RecyclerNewPlusZ
#define RecyclerNewWithBarrierZ                         RecyclerNewZ
#define RecyclerNewWithBarrierStruct                    RecyclerNewStruct
#define RecyclerNewWithBarrierStructZ                   RecyclerNewStructZ
#define RecyclerNewWithBarrierStructPlus                RecyclerNewStructPlus
#define RecyclerNewWithBarrierArray                     RecyclerNewArray
#define RecyclerNewWithBarrierArrayZ                    RecyclerNewArrayZ
#define RecyclerNewWithBarrierFinalized                 RecyclerNewFinalized
#define RecyclerNewWithBarrierFinalizedPlus             RecyclerNewFinalizedPlus
#define RecyclerNewWithBarrierTracked                   RecyclerNewTracked
#define RecyclerNewWithBarrierEnumClass                 RecyclerNewEnumClass
#define RecyclerNewWithBarrierWithInfoBits              RecyclerNewWithInfoBits
#define RecyclerNewWithBarrierFinalizedClientTracked    RecyclerNewFinalizedClientTracked
#endif

// Leaf allocators
#define RecyclerNewLeaf(recycler,T,...) AllocatorNewBase(Recycler, recycler, AllocLeafInlined, T, __VA_ARGS__)
#define RecyclerNewLeafZ(recycler,T,...) AllocatorNewBase(Recycler, recycler, AllocLeafZeroInlined, T, __VA_ARGS__)
#define RecyclerNewPlusLeaf(recycler,size,T,...) AllocatorNewPlusLeaf(Recycler, recycler, size, T, __VA_ARGS__)
#define RecyclerNewPlusLeafZ(recycler,size,T,...) AllocatorNewPlusLeafZ(Recycler, recycler, size, T, __VA_ARGS__)
#define RecyclerNewStructLeaf(recycler,T) AllocatorNewStructBase(Recycler, recycler, AllocLeafInlined, T)
#define RecyclerNewStructLeafZ(recycler,T) AllocatorNewStructBase(Recycler, recycler, AllocLeafZeroInlined, T)
#define RecyclerNewArrayLeafZ(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, AllocLeafZero, T, count)
#define RecyclerNewArrayLeaf(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, AllocLeaf, T, count)
#define RecyclerNewFinalizedLeaf(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocFinalizedLeafInlined, T, __VA_ARGS__)))
#define RecyclerNewFinalizedLeafPlus(recycler, size, T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewPlusBase(Recycler, recycler, AllocFinalizedLeaf, size, T, __VA_ARGS__)))
#define RecyclerNewTrackedLeaf(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocTrackedLeafInlined, T, __VA_ARGS__)))
#define RecyclerNewTrackedLeafPlusZ(recycler,size,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewPlusBase(Recycler, recycler, AllocZeroTrackedLeafInlined, size, T, __VA_ARGS__)))



#ifdef TRACE_OBJECT_LIFETIME
#define RecyclerNewLeafTrace(recycler,T,...) AllocatorNewBase(Recycler, recycler, AllocLeafTrace, T, __VA_ARGS__)
#define RecyclerNewLeafZTrace(recycler,T,...) AllocatorNewBase(Recycler, recycler, AllocLeafZeroTrace, T, __VA_ARGS__)
#define RecyclerNewPlusLeafTrace(recycler,size,T,...) AllocatorNewPlusBase(Recycler, recycler, AllocLeafTrace, size, T, __VA_ARGS__)
#define RecyclerNewArrayLeafZTrace(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, AllocLeafZeroTrace, T, count)
#define RecyclerNewArrayTrace(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, AllocTrace, T, count)
#define RecyclerNewArrayZTrace(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, AllocZeroTrace, T, count)
#define RecyclerNewArrayLeafTrace(recycler,T,count) AllocatorNewArrayBase(Recycler, recycler, AllocLeafTrace, T, count)
#define RecyclerNewFinalizedTrace(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocFinalizedTrace, T, __VA_ARGS__)))
#define RecyclerNewFinalizedLeafTrace(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocFinalizedLeafTrace, T, __VA_ARGS__)))
#define RecyclerNewFinalizedPlusTrace(recycler, size, T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewPlusBase(Recycler, recycler, AllocFinalizedTrace, size, T, __VA_ARGS__)))
#define RecyclerNewTrackedTrace(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocTrackedTrace, T, __VA_ARGS__)))
#define RecyclerNewTrackedLeafTrace(recycler,T,...) static_cast<T *>(static_cast<FinalizableObject *>(AllocatorNewBase(Recycler, recycler, AllocTrackedLeafTrace, T, __VA_ARGS__)))
#else
#define RecyclerNewLeafTrace RecyclerNewLeaf
#define RecyclerNewLeafZTrace  RecyclerNewLeafZ
#define RecyclerNewPlusLeafTrace RecyclerNewPlusLeaf
#define RecyclerNewArrayLeafZTrace RecyclerNewArrayLeafZ
#define RecyclerNewArrayTrace RecyclerNewArray
#define RecyclerNewArrayZTrace RecyclerNewArrayZ
#define RecyclerNewArrayLeafTrace RecyclerNewArrayLeaf
#define RecyclerNewFinalizedTrace RecyclerNewFinalized
#define RecyclerNewFinalizedLeafTrace RecyclerNewFinalizedLeaf
#define RecyclerNewFinalizedPlusTrace RecyclerNewFinalizedPlus
#define RecyclerNewTrackedTrace RecyclerNewTracked
#define RecyclerNewTrackedLeafTrace RecyclerNewTrackedLeaf
#endif

#ifdef RECYCLER_TRACE
#define RecyclerVerboseTrace(flags, ...) \
    if (flags.Verbose && flags.Trace.IsEnabled(Js::RecyclerPhase)) \
        { \
        Output::Print(__VA_ARGS__); \
        }
#define AllocationVerboseTrace(flags, ...) \
    if (flags.Verbose && flags.Trace.IsEnabled(Js::MemoryAllocationPhase)) \
        { \
        Output::Print(__VA_ARGS__); \
        }

#define LargeAllocationVerboseTrace(flags, ...) \
    if (flags.Verbose && \
        (flags.Trace.IsEnabled(Js::MemoryAllocationPhase) || \
         flags.Trace.IsEnabled(Js::LargeMemoryAllocationPhase))) \
        { \
        Output::Print(__VA_ARGS__); \
        }
#define PageAllocatorAllocationVerboseTrace(flags, ...) \
    if (flags.Verbose && flags.Trace.IsEnabled(Js::PageAllocatorAllocPhase)) \
        { \
        Output::Print(__VA_ARGS__); \
        }

#else
#define RecyclerVerboseTrace(...)
#define AllocationVerboseTrace(...)
#define LargeAllocationVerboseTrace(...)

#endif

#define RecyclerHeapNew(recycler,heapInfo,T,...) new (recycler, heapInfo) T(__VA_ARGS__)
#define RecyclerHeapDelete(recycler,heapInfo,addr) (static_cast<Recycler *>(recycler)->HeapFree(heapInfo,addr))

typedef void (__cdecl* ExternalRootMarker)(void *);

enum CollectionFlags
{
    CollectHeuristic_AllocSize          = 0x00000001,
    CollectHeuristic_Time               = 0x00000002,
    CollectHeuristic_TimeIfScriptActive = 0x00000004,
    CollectHeuristic_TimeIfInScript     = 0x00000008,
    CollectHeuristic_Never              = 0x00000080,
    CollectHeuristic_Mask               = 0x000000FF,

    CollectOverride_FinishConcurrent    = 0x00001000,
    CollectOverride_ExhaustiveCandidate = 0x00002000,
    CollectOverride_ForceInThread       = 0x00004000,
    CollectOverride_AllowDispose        = 0x00008000,
    CollectOverride_AllowReentrant      = 0x00010000,
    CollectOverride_ForceFinish         = 0x00020000,
    CollectOverride_Explicit            = 0x00040000,
    CollectOverride_DisableIdleFinish   = 0x00080000,
    CollectOverride_BackgroundFinishMark= 0x00100000,
    CollectOverride_FinishConcurrentTimeout = 0x00200000,
    CollectOverride_NoExhaustiveCollect = 0x00400000,
    CollectOverride_SkipStack           = 0x01000000,
    CollectOverride_CheckScriptContextClose = 0x02000000,
    CollectMode_Partial                 = 0x08000000,
    CollectMode_Concurrent              = 0x10000000,
    CollectMode_Exhaustive              = 0x20000000,
    CollectMode_DecommitNow             = 0x40000000,
    CollectMode_CacheCleanup            = 0x80000000,

    CollectNowForceInThread         = CollectOverride_ForceInThread,
    CollectNowForceInThreadExternal = CollectOverride_ForceInThread | CollectOverride_AllowDispose,
    CollectNowForceInThreadExternalNoStack = CollectOverride_ForceInThread | CollectOverride_AllowDispose | CollectOverride_SkipStack,
    CollectNowDefault               = CollectOverride_FinishConcurrent,
    CollectNowDefaultLSCleanup      = CollectOverride_FinishConcurrent | CollectOverride_AllowDispose,
    CollectNowDecommitNowExplicit   = CollectNowDefault | CollectMode_DecommitNow | CollectMode_CacheCleanup | CollectOverride_Explicit | CollectOverride_AllowDispose,
    CollectNowConcurrent            = CollectOverride_FinishConcurrent | CollectMode_Concurrent,
    CollectNowExhaustive            = CollectOverride_FinishConcurrent | CollectMode_Exhaustive | CollectOverride_AllowDispose,
    CollectNowPartial               = CollectOverride_FinishConcurrent | CollectMode_Partial,
    CollectNowConcurrentPartial     = CollectMode_Concurrent | CollectNowPartial,

    CollectOnAllocation             = CollectHeuristic_AllocSize | CollectHeuristic_Time | CollectMode_Concurrent | CollectMode_Partial | CollectOverride_FinishConcurrent | CollectOverride_AllowReentrant | CollectOverride_FinishConcurrentTimeout,
    CollectOnTypedArrayAllocation   = CollectHeuristic_AllocSize | CollectHeuristic_Time | CollectMode_Concurrent | CollectMode_Partial | CollectOverride_FinishConcurrent | CollectOverride_AllowReentrant | CollectOverride_FinishConcurrentTimeout | CollectOverride_AllowDispose,
    CollectOnScriptIdle             = CollectOverride_CheckScriptContextClose | CollectOverride_FinishConcurrent | CollectMode_Concurrent | CollectMode_CacheCleanup | CollectOverride_SkipStack,
    CollectOnScriptExit             = CollectOverride_CheckScriptContextClose | CollectHeuristic_AllocSize | CollectOverride_FinishConcurrent | CollectMode_Concurrent | CollectMode_CacheCleanup,
    CollectExhaustiveCandidate      = CollectHeuristic_Never | CollectOverride_ExhaustiveCandidate,
    CollectOnScriptCloseNonPrimary  = CollectNowConcurrent | CollectOverride_ExhaustiveCandidate | CollectOverride_AllowDispose,
    CollectOnRecoverFromOutOfMemory = CollectOverride_ForceInThread | CollectMode_DecommitNow,
    CollectOnSuspendCleanup         = CollectNowConcurrent | CollectMode_Exhaustive | CollectMode_DecommitNow | CollectOverride_DisableIdleFinish,

    FinishConcurrentOnIdle          = CollectMode_Concurrent | CollectOverride_DisableIdleFinish,
    FinishConcurrentOnIdleAtRoot    = CollectMode_Concurrent | CollectOverride_DisableIdleFinish | CollectOverride_SkipStack,
    FinishConcurrentDefault         = CollectMode_Concurrent | CollectOverride_DisableIdleFinish | CollectOverride_BackgroundFinishMark,
    FinishConcurrentOnExitScript    = FinishConcurrentDefault,
    FinishConcurrentOnEnterScript   = FinishConcurrentDefault,
    FinishConcurrentOnAllocation    = FinishConcurrentDefault,
    FinishDispose                   = CollectOverride_AllowDispose,
    FinishDisposeTimed              = CollectOverride_AllowDispose | CollectHeuristic_TimeIfScriptActive,
    ForceFinishCollection           = CollectOverride_ForceFinish | CollectOverride_ForceInThread,

#ifdef RECYCLER_STRESS
    CollectStress                   = CollectNowForceInThread,
#if ENABLE_PARTIAL_GC
    CollectPartialStress            = CollectMode_Partial,
#endif
#if ENABLE_CONCURRENT_GC
    CollectBackgroundStress         = CollectNowDefault,
    CollectConcurrentStress         = CollectNowConcurrent,
#if ENABLE_PARTIAL_GC
    CollectConcurrentPartialStress  = CollectConcurrentStress | CollectPartialStress,
#endif
#endif
#endif

#if defined(CHECK_MEMORY_LEAK) || defined(LEAK_REPORT)
    CollectNowFinalGC                   = CollectNowExhaustive | CollectOverride_ForceInThread | CollectOverride_SkipStack | CollectOverride_Explicit | CollectOverride_AllowDispose,
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    CollectNowExhaustiveSkipStack   = CollectNowExhaustive | CollectOverride_SkipStack, // Used by test
#endif
};

class RecyclerCollectionWrapper
{
public:
    RecyclerCollectionWrapper() :
        _isScriptContextCloseGCPending(FALSE)
    { }

    typedef BOOL (Recycler::*CollectionFunction)(CollectionFlags flags);
    virtual void PreCollectionCallBack(CollectionFlags flags) = 0;
    virtual void PreSweepCallback() = 0;
    virtual void PreRescanMarkCallback() = 0;
    virtual size_t RootMarkCallback(RecyclerScanMemoryCallback& scanMemoryCallback, BOOL * stacksScannedByRuntime) = 0;
    virtual void RescanMarkTimeoutCallback() = 0;
    virtual void EndMarkCallback() = 0;
    virtual void ConcurrentCallback() = 0;
    virtual void WaitCollectionCallBack() = 0;
    virtual void PostCollectionCallBack() = 0;
    virtual BOOL ExecuteRecyclerCollectionFunction(Recycler * recycler, CollectionFunction function, CollectionFlags flags) = 0;
    virtual uint GetRandomNumber() = 0;
    virtual bool DoSpecialMarkOnScanStack() = 0;
    virtual void PostSweepRedeferralCallBack() = 0;

#ifdef FAULT_INJECTION
    virtual void DisposeScriptContextByFaultInjectionCallBack() = 0;
#endif
    virtual void DisposeObjects(Recycler * recycler) = 0;
    virtual void PreDisposeObjectsCallBack() = 0;
#ifdef ENABLE_PROJECTION
    virtual void MarkExternalWeakReferencedObjects(bool inPartialCollect) = 0;
    virtual void ResolveExternalWeakReferencedObjects() = 0;
#endif
#if DBG || defined(PROFILE_EXEC)
    virtual bool AsyncHostOperationStart(void *) = 0;
    virtual void AsyncHostOperationEnd(bool wasInAsync, void *) = 0;
#endif

    BOOL GetIsScriptContextCloseGCPending()
    {
        return _isScriptContextCloseGCPending;
    }

    void ClearIsScriptContextCloseGCPending()
    {
        _isScriptContextCloseGCPending = FALSE;
    }

    void SetIsScriptContextCloseGCPending()
    {
        _isScriptContextCloseGCPending = TRUE;
    }

protected:
    BOOL _isScriptContextCloseGCPending;
};

class DefaultRecyclerCollectionWrapper : public RecyclerCollectionWrapper
{
public:
    virtual void PreCollectionCallBack(CollectionFlags flags) override {}
    virtual void PreSweepCallback() override {}
    virtual void PreRescanMarkCallback() override {}
    virtual void RescanMarkTimeoutCallback() override {}
    virtual void EndMarkCallback() override {}
    virtual size_t RootMarkCallback(RecyclerScanMemoryCallback& scanMemoryCallback, BOOL * stacksScannedByRuntime) override { *stacksScannedByRuntime = FALSE; return 0; }
    virtual void ConcurrentCallback() override {}
    virtual void WaitCollectionCallBack() override {}
    virtual void PostCollectionCallBack() override {}
    virtual BOOL ExecuteRecyclerCollectionFunction(Recycler * recycler, CollectionFunction function, CollectionFlags flags) override;
    virtual uint GetRandomNumber() override { return 0; }
    virtual bool DoSpecialMarkOnScanStack() override { return false; }
    virtual void PostSweepRedeferralCallBack() override {}
#ifdef FAULT_INJECTION
    virtual void DisposeScriptContextByFaultInjectionCallBack() override {};
#endif
    virtual void DisposeObjects(Recycler * recycler) override;
    virtual void PreDisposeObjectsCallBack() override {};

#ifdef ENABLE_PROJECTION
    virtual void MarkExternalWeakReferencedObjects(bool inPartialCollect) override {};
    virtual void ResolveExternalWeakReferencedObjects() override {};
#endif
#if DBG || defined(PROFILE_EXEC)
    virtual bool AsyncHostOperationStart(void *) override { return false; };
    virtual void AsyncHostOperationEnd(bool wasInAsync, void *) override {};
#endif
    static DefaultRecyclerCollectionWrapper Instance;

private:
    static bool IsCollectionDisabled(Recycler * recycler);
};


#ifdef RECYCLER_STATS
struct RecyclerCollectionStats
{
    size_t startCollectAllocBytes;
#if ENABLE_PARTIAL_GC
    size_t startCollectNewPageCount;
#endif
    size_t continueCollectAllocBytes;

    size_t finishCollectTryCount;

    // Heuristic Stats
#if ENABLE_PARTIAL_GC
    size_t rescanRootBytes;
    size_t estimatedPartialReuseBytes;
    size_t uncollectedNewPageCountPartialCollect;
    size_t partialCollectSmallHeapBlockReuseMinFreeBytes;
    double collectEfficacy;
    double collectCost;
#endif

    // Mark stats
    size_t tryMarkCount;        // # of pointer try mark (* pointer size to get total number byte looked at)
    size_t tryMarkNullCount;
    size_t tryMarkUnalignedCount;
    size_t tryMarkNonRecyclerMemoryCount;
    size_t tryMarkInteriorCount;
    size_t tryMarkInteriorNullCount;
    size_t tryMarkInteriorNonRecyclerMemoryCount;
    size_t rootCount;
    size_t stackCount;
    size_t remarkCount;

    size_t scanCount;           // non-leaf objects marked.
    size_t trackCount;
    size_t finalizeCount;
    size_t markThruNewObjCount;
    size_t markThruFalseNewObjCount;

    struct MarkData
    {
        // Rescan stats
        size_t rescanPageCount;
        size_t rescanObjectCount;
        size_t rescanObjectByteCount;
        size_t rescanLargePageCount;
        size_t rescanLargeObjectCount;
        size_t rescanLargeByteCount;
        size_t markCount;           // total number of object marked
        size_t markBytes;           // size of all objects marked.
    } markData;

#if ENABLE_CONCURRENT_GC
    MarkData backgroundMarkData[RecyclerHeuristic::MaxBackgroundRepeatMarkCount];
    size_t trackedObjectCount;
#endif

#if ENABLE_PARTIAL_GC
    size_t clientTrackedObjectCount;
#endif

    // Sweep stats
    size_t heapBlockCount[HeapBlock::BlockTypeCount];                       // number of heap blocks (processed during swept)
    size_t heapBlockFreeCount[HeapBlock::BlockTypeCount];                   // number of heap blocks deleted
    size_t heapBlockConcurrentSweptCount[HeapBlock::SmallBlockTypeCount];
    size_t heapBlockSweptCount[HeapBlock::SmallBlockTypeCount];             // number of heap blocks swept

    size_t objectSweptCount;                // objects freed (free list + whole page freed)
    size_t objectSweptBytes;
    size_t objectSweptFreeListCount;        // objects freed (free list)
    size_t objectSweptFreeListBytes;
    size_t objectSweepScanCount;            // number of objects walked for sweeping (exclude whole page freed)
    size_t finalizeSweepCount;              // number of objects finalizer/dispose called

#if ENABLE_PARTIAL_GC
    size_t smallNonLeafHeapBlockPartialReuseCount[HeapBlock::SmallBlockTypeCount];
    size_t smallNonLeafHeapBlockPartialReuseBytes[HeapBlock::SmallBlockTypeCount];
    size_t smallNonLeafHeapBlockPartialUnusedCount[HeapBlock::SmallBlockTypeCount];
    size_t smallNonLeafHeapBlockPartialUnusedBytes[HeapBlock::SmallBlockTypeCount];
#endif

    // Memory Stats
    size_t heapBlockFreeByteCount[HeapBlock::BlockTypeCount]; // The remaining usable free byte count

    size_t largeHeapBlockUsedByteCount;     // Used byte count
    size_t largeHeapBlockTotalByteCount;    // Total byte count

    // Empty/zero heap block stats
    uint numEmptySmallBlocks[HeapBlock::SmallBlockTypeCount];
    uint numZeroedOutSmallBlocks;
};
#define RECYCLER_STATS_INC_IF(cond, r, f) if (cond) { RECYCLER_STATS_INC(r, f); }
#define RECYCLER_STATS_INC(r, f) ++r->collectionStats.f
#define RECYCLER_STATS_INTERLOCKED_INC(r, f) { InterlockedIncrement((LONG *)&r->collectionStats.f); }
#define RECYCLER_STATS_DEC(r, f) --r->collectionStats.f
#define RECYCLER_STATS_ADD(r, f, v) r->collectionStats.f += (v)
#define RECYCLER_STATS_INTERLOCKED_ADD(r, f, v) { InterlockedAdd((LONG *)&r->collectionStats.f, (LONG)(v)); }
#define RECYCLER_STATS_SUB(r, f, v) r->collectionStats.f -= (v)
#define RECYCLER_STATS_SET(r, f, v) r->collectionStats.f = v
#else
#define RECYCLER_STATS_INC_IF(cond, r, f)
#define RECYCLER_STATS_INC(r, f)
#define RECYCLER_STATS_INTERLOCKED_INC(r, f)
#define RECYCLER_STATS_DEC(r, f)
#define RECYCLER_STATS_ADD(r, f, v)
#define RECYCLER_STATS_INTERLOCKED_ADD(r, f, v)
#define RECYCLER_STATS_SUB(r, f, v)
#define RECYCLER_STATS_SET(r, f, v)
#endif
#ifdef RECYCLER_TRACE
struct CollectionParam
{
    CollectionFlags flags;
    bool finishOnly;
    bool repeat;
    bool priorityBoostConcurrentSweepOverride;
    bool domCollect;
    int timeDiff;
    size_t uncollectedAllocBytes;
    size_t uncollectedPinnedObjects;
#if ENABLE_PARTIAL_GC
    size_t uncollectedNewPageCountPartialCollect;
    size_t uncollectedNewPageCount;
    size_t unusedPartialCollectFreeBytes;
    bool inPartialCollectMode;
#endif
};
#endif

#include "RecyclerObjectGraphDumper.h"

#if ENABLE_CONCURRENT_GC
class RecyclerParallelThread
{
public:
    typedef void (Recycler::* WorkFunc)();

    RecyclerParallelThread(Recycler * recycler, WorkFunc workFunc) :
        recycler(recycler),
        workFunc(workFunc),
        concurrentWorkReadyEvent(NULL),
        concurrentWorkDoneEvent(NULL),
        concurrentThread(NULL)
    {
    }

    ~RecyclerParallelThread()
    {
        Assert(concurrentThread == NULL);
        Assert(concurrentWorkReadyEvent == NULL);
        Assert(concurrentWorkDoneEvent == NULL);
    }

    bool StartConcurrent();
    void WaitForConcurrent();
    void Shutdown();
    bool EnableConcurrent(bool synchronizeOnStartup);

private:
    // Static entry point for thread creation
    static unsigned int CALLBACK StaticThreadProc(LPVOID lpParameter);

    // Static entry point for thread service usage
    static void CALLBACK StaticBackgroundWorkCallback(void * callbackData);

private:
    WorkFunc workFunc;
    Recycler * recycler;
    HANDLE concurrentWorkReadyEvent;// main thread uses this event to tell concurrent threads that the work is ready
    HANDLE concurrentWorkDoneEvent;// concurrent threads use this event to tell main thread that the work allocated is done
    HANDLE concurrentThread;
    bool synchronizeOnStartup;
};
#endif

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
class AutoProtectPages
{
public:
    AutoProtectPages(Recycler* recycler, bool protectEnabled);
    ~AutoProtectPages();
    void Unprotect();

private:
    Recycler* recycler;
    bool isReadOnly;
};
#endif


class Recycler
{
    friend class RecyclerScanMemoryCallback;
    friend class RecyclerSweep;
    friend class MarkContext;
    friend class HeapBlock;
    friend class HeapBlockMap32;
#if ENABLE_CONCURRENT_GC
    friend class RecyclerParallelThread;
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    friend class AutoProtectPages;
#endif

    template <typename T> friend class RecyclerWeakReference;
    template <typename T> friend class WeakReferenceHashTable;
    template <typename TBlockType>
    friend class SmallHeapBlockAllocator;       // Needed for FindHeapBlock
#if defined(RECYCLER_TRACE)
    friend class JavascriptThreadService;
#endif
#ifdef HEAP_ENUMERATION_VALIDATION
    friend class ActiveScriptProfilerHeapEnum;
#endif
    friend class ScriptEngineBase;  // This is for disabling GC for certain Host operations.
    friend class ::CodeGenNumberThreadAllocator;
    friend struct ::XProcNumberPageSegmentManager;
public:
    static const uint ConcurrentThreadStackSize = 300000;
    static const bool FakeZeroLengthArray = true;

#ifdef RECYCLER_PAGE_HEAP
    // Keeping as constant in case we want to tweak the value here
    // Set to 0 so that the tool can do the filtering instead of the runtime
#if DBG
    static const int s_numFramesToSkipForPageHeapAlloc = 10;
    static const int s_numFramesToSkipForPageHeapFree = 0;
    static const int s_numFramesToCaptureForPageHeap = 32;
#else
    static const int s_numFramesToSkipForPageHeapAlloc = 0;
    static const int s_numFramesToSkipForPageHeapFree = 0;
    static const int s_numFramesToCaptureForPageHeap = 32;
#endif
#endif

    uint Cookie;

    class AutoEnterExternalStackSkippingGCMode
    {
    public:
        AutoEnterExternalStackSkippingGCMode(Recycler* recycler):
            _recycler(recycler)
        {
            // Setting this in a re-entrant mode is not allowed
            Assert(!recycler->isExternalStackSkippingGC);

#if DBG
            _recycler->isExternalStackSkippingGC = true;
#endif
        }

        ~AutoEnterExternalStackSkippingGCMode()
        {
#if DBG
            _recycler->isExternalStackSkippingGC = false;
#endif
        }

    private:
        Recycler* _recycler;
    };

private:
    IdleDecommitPageAllocator * threadPageAllocator;
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
    RecyclerPageAllocator recyclerWithBarrierPageAllocator;
#endif
    RecyclerPageAllocator recyclerPageAllocator;
    RecyclerPageAllocator recyclerLargeBlockPageAllocator;
public:
    template<typename Action>
    void ForEachPageAllocator(Action action)
    {
        action(&this->recyclerPageAllocator);
        action(&this->recyclerLargeBlockPageAllocator);
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
        action(&this->recyclerWithBarrierPageAllocator);
#endif
        action(threadPageAllocator);
    }
private:
    class AutoSwitchCollectionStates
    {
    public:
        AutoSwitchCollectionStates(Recycler* recycler, CollectionState entryState, CollectionState exitState):
            _recycler(recycler),
            _exitState(exitState)
        {
            _recycler->collectionState = entryState;
        }

        ~AutoSwitchCollectionStates()
        {
            _recycler->collectionState = _exitState;
        }

    private:
        Recycler* _recycler;
        CollectionState _exitState;
    };

    CollectionState collectionState;
    JsUtil::ThreadService *threadService;

    HeapBlockMap heapBlockMap;

#if defined(CHECK_MEMORY_LEAK) || defined(LEAK_REPORT)
    struct PinRecord
    {
#ifdef STACK_BACK_TRACE
        PinRecord() : refCount(0), stackBackTraces(nullptr) {}
#else
        PinRecord() : refCount(0) {}
#endif
        PinRecord& operator=(uint newRefCount)
        {
#ifdef STACK_BACK_TRACE
            Assert(stackBackTraces == nullptr);
#endif
            Assert(newRefCount == 0); refCount = 0; return *this;
        }
        PinRecord& operator++() { ++refCount; return *this; }
        PinRecord& operator--() { --refCount; return *this; }
        operator uint() const { return refCount; }
#ifdef STACK_BACK_TRACE
        StackBackTraceNode * stackBackTraces;
#endif
    private:
        uint refCount;
    };
#else
    typedef uint PinRecord;
#endif

    typedef SimpleHashTable<void *, PinRecord, HeapAllocator, DefaultComparer, true, PrimePolicy> PinnedObjectHashTable;
    PinnedObjectHashTable pinnedObjectMap;

    WeakReferenceHashTable<PrimePolicy> weakReferenceMap;
    uint weakReferenceCleanupId;

    void * transientPinnedObject;
#if defined(CHECK_MEMORY_LEAK) || defined(LEAK_REPORT)
#ifdef STACK_BACK_TRACE
    StackBackTrace * transientPinnedObjectStackBackTrace;
#endif
#endif

    struct GuestArenaAllocator : public ArenaAllocator
    {
        GuestArenaAllocator(__in_z char16 const*  name, PageAllocator * pageAllocator, void (*outOfMemoryFunc)())
            : ArenaAllocator(name, pageAllocator, outOfMemoryFunc), pendingDelete(false)
        {
        }
        bool pendingDelete;
    };
    DListBase<GuestArenaAllocator> guestArenaList;
    DListBase<ArenaData*> externalGuestArenaList;    // guest arenas are scanned for roots

#ifdef RECYCLER_PAGE_HEAP
    bool isPageHeapEnabled;
    bool capturePageHeapAllocStack;
    bool capturePageHeapFreeStack;

    inline bool IsPageHeapEnabled() const { return isPageHeapEnabled; }
    template<ObjectInfoBits attributes>
    bool IsPageHeapEnabled(size_t size);
    inline bool ShouldCapturePageHeapAllocStack() const { return capturePageHeapAllocStack; }
    void VerifyPageHeapFillAfterAlloc(char* memBlock, size_t size, ObjectInfoBits attributes);
#else
    inline const bool IsPageHeapEnabled() const { return false; }
    inline bool ShouldCapturePageHeapAllocStack() const { return false; }
#endif


#ifdef RECYCLER_MARK_TRACK
    MarkMap* markMap;
    CriticalSection markMapCriticalSection;

    void PrintMarkMap();
    void ClearMarkMap();
#endif

    // Number of pages to reserve for the primary mark stack
    // This is the minimum number of pages to guarantee that a single heap block
    // can be rescanned in the worst possible case where every object in a heap block
    // in the smallest bucket needs to be rescanned
    // These many pages being reserved guarantees that in OOM Rescan, we can make progress
    // on every rescan iteration
    // We add one because there is a small amount of the page reserved for page pool metadata
    // so we need to allocate an additional page to be sure
    // Currently, this works out to 2 pages on 32-bit and 5 pages on 64-bit
    static const int PrimaryMarkStackReservedPageCount =
        ((SmallAllocationBlockAttributes::PageCount * MarkContext::MarkCandidateSize) / SmallAllocationBlockAttributes::MinObjectSize) + 1;

    MarkContext markContext;

    // Contexts for parallel marking.
    // We support up to 4 way parallelism, main context + 3 additional parallel contexts.
    MarkContext parallelMarkContext1;
    MarkContext parallelMarkContext2;
    MarkContext parallelMarkContext3;

    // Page pools for above markContexts
    PagePool markPagePool;
    PagePool parallelMarkPagePool1;
    PagePool parallelMarkPagePool2;
    PagePool parallelMarkPagePool3;

    bool IsMarkStackEmpty();
    bool HasPendingMarkObjects() const { return markContext.HasPendingMarkObjects() || parallelMarkContext1.HasPendingMarkObjects() || parallelMarkContext2.HasPendingMarkObjects() || parallelMarkContext3.HasPendingMarkObjects(); }
    bool HasPendingTrackObjects() const { return markContext.HasPendingTrackObjects() || parallelMarkContext1.HasPendingTrackObjects() || parallelMarkContext2.HasPendingTrackObjects() || parallelMarkContext3.HasPendingTrackObjects(); }

    RecyclerCollectionWrapper * collectionWrapper;

    HANDLE mainThreadHandle;
    void * stackBase;
    class SavedRegisterState
    {
    public:
#if _M_IX86
        static const int NumRegistersToSave = 8;
#elif _M_ARM
        static const int NumRegistersToSave = 13;
#elif _M_ARM64
        static const int NumRegistersToSave = 13;
#elif _M_AMD64
        static const int NumRegistersToSave = 16;
#endif

        SavedRegisterState()
        {
            memset(registers, 0, sizeof(void*) * NumRegistersToSave);
        }

        void** GetRegisters()
        {
            return registers;
        }

        void*  GetStackTop()
        {
            // By convention, our register-saving routine will always
            // save the stack pointer as the first item in the array
            return registers[0];
        }

    private:
        void* registers[NumRegistersToSave];
    };

    SavedRegisterState savedThreadContext;

    bool inDispose;

#if DBG
    uint collectionCount;
#endif
#if DBG || defined RECYCLER_TRACE
    bool inResolveExternalWeakReferences;
#endif

    bool allowDispose;
    bool inDisposeWrapper;
    bool needOOMRescan;
    bool hasDisposableObject;
    DWORD tickCountNextDispose;
    bool hasPendingTransferDisposedObjects;
    bool inExhaustiveCollection;
    bool hasExhaustiveCandidate;
    bool inCacheCleanupCollection;
    bool inDecommitNowCollection;
    bool isScriptActive;
    bool isInScript;
    bool isShuttingDown;
    bool scanPinnedObjectMap;
    bool hasScannedInitialImplicitRoots;
    bool hasPendingUnpinnedObject;
    bool hasPendingDeleteGuestArena;
    bool inEndMarkOnLowMemory;
    bool decommitOnFinish;
    bool enableScanInteriorPointers;
    bool enableScanImplicitRoots;
    bool disableCollectOnAllocationHeuristics;
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    bool disableCollection;
#endif

#if ENABLE_PARTIAL_GC
    bool enablePartialCollect;
    bool inPartialCollectMode;
#if ENABLE_CONCURRENT_GC
    bool hasBackgroundFinishPartial;
    bool partialConcurrentNextCollection;
#endif
#endif
#ifdef RECYCLER_STRESS
    bool forcePartialScanStack;
    bool recyclerStress;
#if ENABLE_CONCURRENT_GC
    bool recyclerBackgroundStress;
    bool recyclerConcurrentStress;
    bool recyclerConcurrentRepeatStress;
#endif
#if ENABLE_PARTIAL_GC
    bool recyclerPartialStress;
#endif
#endif
#if DBG
    bool isExternalStackSkippingGC;
#endif
    bool skipStack;
#if ENABLE_CONCURRENT_GC
#if DBG
    bool isConcurrentGCOnIdle;
    bool isFinishGCOnIdle;
#endif

    bool queueTrackedObject;
    bool hasPendingConcurrentFindRoot;
    bool priorityBoost;
    bool disableConcurrent;
    bool enableConcurrentMark;
    bool enableParallelMark;
    bool enableConcurrentSweep;

    uint maxParallelism;        // Max # of total threads to run in parallel

    byte backgroundRescanCount;             // for ETW events and stats
    byte backgroundFinishMarkCount;
    size_t backgroundRescanRootBytes;
    HANDLE concurrentWorkReadyEvent; // main thread uses this event to tell concurrent threads that the work is ready
    HANDLE concurrentWorkDoneEvent; // concurrent threads use this event to tell main thread that the work allocated is done
    HANDLE concurrentThread;

    template <uint parallelId>
    void ParallelWorkFunc();

    RecyclerParallelThread parallelThread1;
    RecyclerParallelThread parallelThread2;

#if DBG
    // Variable indicating if the concurrent thread has exited or not
    // If the concurrent thread hasn't started yet, this is set to true
    // Once the concurrent thread starts, it sets this to false,
    // and when the concurrent thread exits, it sets this to true.
    bool concurrentThreadExited;
    bool disableConcurrentThreadExitedCheck;
    bool isProcessingTrackedObjects;
#endif

    uint tickCountStartConcurrent;

    bool isAborting;
#endif

#if DBG
    bool hasIncompleteDoCollect;
    // This is set to true when we begin a Rescan, and set to false when either:
    // (1) We finish the final in-thread Rescan and are about to Mark
    // (2) We do a conditional ResetWriteWatch and are about to Mark
    // When this flag is true, we should not be modifying existing mark-related state,
    // including markBits and rescanState.
    bool isProcessingRescan;
#endif

    Js::ConfigFlagsTable&  recyclerFlagsTable;
    RecyclerSweep recyclerSweepInstance;
    RecyclerSweep * recyclerSweep;

    static const uint tickDiffToNextCollect = 300;

#ifdef IDLE_DECOMMIT_ENABLED
    HANDLE concurrentIdleDecommitEvent;
    LONG needIdleDecommitSignal;
#endif

#if ENABLE_PARTIAL_GC
    SListBase<void *> clientTrackedObjectList;
    ArenaAllocator clientTrackedObjectAllocator;

    size_t partialUncollectedAllocBytes;

    // Dynamic Heuristics for partial GC
    size_t uncollectedNewPageCountPartialCollect;
#endif

    uint tickCountNextCollection;
    uint tickCountNextFinishCollection;

    void (*outOfMemoryFunc)();
#ifdef RECYCLER_TEST_SUPPORT
    BOOL (*checkFn)(char* addr, size_t size);
#endif
    ExternalRootMarker externalRootMarker;
    void * externalRootMarkerContext;

#ifdef PROFILE_EXEC
    Js::Profiler * profiler;
    Js::Profiler * backgroundProfiler;
    PageAllocator backgroundProfilerPageAllocator;
    DListBase<ArenaAllocator> backgroundProfilerArena;
#endif

    // destruct autoHeap after backgroundProfilerPageAllocator;
    HeapInfo autoHeap;

#ifdef PROFILE_MEM
    RecyclerMemoryData * memoryData;
#endif
    ThreadContextId mainThreadId;

#if DBG
    uint heapBlockCount;
    bool disableThreadAccessCheck;
#endif
#if DBG || defined(RECYCLER_STATS)
    bool isForceSweeping;
#endif
#ifdef NTBUILD
    RecyclerWatsonTelemetryBlock localTelemetryBlock;
    RecyclerWatsonTelemetryBlock * telemetryBlock;
#endif
#ifdef RECYCLER_STATS
    RecyclerCollectionStats collectionStats;
    void PrintHeapBlockStats(char16 const * name, HeapBlock::HeapBlockType type);
    void PrintHeapBlockMemoryStats(char16 const * name, HeapBlock::HeapBlockType type);
    void PrintCollectStats();
    void PrintHeuristicCollectionStats();
    void PrintMarkCollectionStats();
    void PrintBackgroundCollectionStats();
    void PrintMemoryStats();
    void PrintBackgroundCollectionStat(RecyclerCollectionStats::MarkData const& markData);
#endif
#ifdef RECYCLER_TRACE
    CollectionParam collectionParam;
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    uint verifyPad;
    bool verifyEnabled;
#endif

#ifdef RECYCLER_DUMP_OBJECT_GRAPH
    friend class RecyclerObjectGraphDumper;
    RecyclerObjectGraphDumper * objectGraphDumper;
public:
    bool dumpObjectOnceOnCollect;
#endif
public:

    Recycler(AllocationPolicyManager * policyManager, IdleDecommitPageAllocator * pageAllocator, void(*outOfMemoryFunc)(), Js::ConfigFlagsTable& flags);
    ~Recycler();

    void Initialize(const bool forceInThread, JsUtil::ThreadService *threadService, const bool deferThreadStartup = false
#ifdef RECYCLER_PAGE_HEAP
        , PageHeapMode pageheapmode = PageHeapMode::PageHeapModeOff
        , bool captureAllocCallStack = false
        , bool captureFreeCallStack = false
#endif
        );

    Js::ConfigFlagsTable& GetRecyclerFlagsTable() const { return this->recyclerFlagsTable; }
    void SetMemProtectMode();
    bool IsMemProtectMode();
    size_t GetUsedBytes();
    void LogMemProtectHeapSize(bool fromGC);
    char* Realloc(void* buffer, DECLSPEC_GUARD_OVERFLOW size_t existingBytes, DECLSPEC_GUARD_OVERFLOW size_t requestedBytes, bool truncate = true);
#ifdef NTBUILD
    void SetTelemetryBlock(RecyclerWatsonTelemetryBlock * telemetryBlock) { this->telemetryBlock = telemetryBlock; }
#endif

    void Prime();
    void* GetOwnerContext() { return (void*) this->collectionWrapper; }
    PageAllocator * GetPageAllocator() { return threadPageAllocator; }
    bool NeedOOMRescan() const;
    void SetNeedOOMRescan();
    void ClearNeedOOMRescan();
    BOOL RequestConcurrentWrapperCallback();
    BOOL CollectionInProgress() const;
    BOOL IsExiting() const;
    BOOL IsSweeping() const;

#ifdef RECYCLER_PAGE_HEAP
    inline bool ShouldCapturePageHeapFreeStack() const { return capturePageHeapFreeStack; }
#else
    inline bool ShouldCapturePageHeapFreeStack() const { return false; }
#endif

    void SetIsThreadBound();
    void SetIsScriptActive(bool isScriptActive);
    void SetIsInScript(bool isInScript);
    bool ShouldIdleCollectOnExit();
    void ScheduleNextCollection();

    IdleDecommitPageAllocator * GetRecyclerLeafPageAllocator();
    IdleDecommitPageAllocator * GetRecyclerPageAllocator();
    IdleDecommitPageAllocator * GetRecyclerLargeBlockPageAllocator();
#ifdef RECYCLER_WRITE_BARRIER_ALLOC_SEPARATE_PAGE
    IdleDecommitPageAllocator * GetRecyclerWithBarrierPageAllocator();
#endif

    BOOL IsShuttingDown() const { return this->isShuttingDown; }
#if ENABLE_CONCURRENT_GC
#if DBG
    BOOL IsConcurrentMarkEnabled() const { return enableConcurrentMark; }
    BOOL IsConcurrentSweepEnabled() const { return enableConcurrentSweep; }
#endif
    template <CollectionFlags flags>
    BOOL FinishConcurrent();
    void ShutdownThread();

    bool EnableConcurrent(JsUtil::ThreadService *threadService, bool startAllThreads);
    void DisableConcurrent();

    void StartQueueTrackedObject();
    bool DoQueueTrackedObject() const;
    void PrepareSweep();
#endif

    template <CollectionFlags flags>
    void SetupPostCollectionFlags();
    void EnsureNotCollecting();

#if ENABLE_CONCURRENT_GC
    bool QueueTrackedObject(FinalizableObject * trackableObject);
#endif

    // FindRoots
    void TryMarkNonInterior(void* candidate, void* parentReference = nullptr);
    void TryMarkInterior(void *candidate, void* parentReference = nullptr);

    bool InCacheCleanupCollection() { return inCacheCleanupCollection; }
    void ClearCacheCleanupCollection() { Assert(inCacheCleanupCollection); inCacheCleanupCollection = false; }

    // Finalizer support
    void SetExternalRootMarker(ExternalRootMarker fn, void * context);
    ArenaAllocator * CreateGuestArena(char16 const * name, void (*outOfMemoryFunc)());
    void DeleteGuestArena(ArenaAllocator * arenaAllocator);
    ArenaData ** RegisterExternalGuestArena(ArenaData* guestArena)
    {
        return externalGuestArenaList.PrependNode(&NoThrowHeapAllocator::Instance, guestArena);
    }
    void UnregisterExternalGuestArena(ArenaData* guestArena)
    {
        externalGuestArenaList.Remove(&NoThrowHeapAllocator::Instance, guestArena);

        // Any time a root is removed during a GC, it indicates that an exhaustive
        // collection is likely going to have work to do so trigger an exhaustive
        // candidate GC to indicate this fact
        this->CollectNow<CollectExhaustiveCandidate>();
    }

    void UnregisterExternalGuestArena(ArenaData** guestArena)
    {
        externalGuestArenaList.RemoveElement(&NoThrowHeapAllocator::Instance, guestArena);

        // Any time a root is removed during a GC, it indicates that an exhaustive
        // collection is likely going to have work to do so trigger an exhaustive
        // candidate GC to indicate this fact
        this->CollectNow<CollectExhaustiveCandidate>();
    }

#ifdef RECYCLER_TEST_SUPPORT
    void SetCheckFn(BOOL(*checkFn)(char* addr, size_t size));
#endif

    void SetCollectionWrapper(RecyclerCollectionWrapper * wrapper);
    static size_t GetAlignedSize(size_t size) { return HeapInfo::GetAlignedSize(size); }
    HeapInfo* GetAutoHeap() { return &autoHeap; }
    template <CollectionFlags flags>
    BOOL CollectNow();

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    void DisplayMemStats();
#endif

    void AddExternalMemoryUsage(size_t size);

    bool NeedDispose() { return this->hasDisposableObject; }

    template <CollectionFlags flags>
    bool FinishDisposeObjectsNow();
    bool RequestExternalMemoryAllocation(size_t size);
    void ReportExternalMemoryFailure(size_t size);
    void ReportExternalMemoryFree(size_t size);
    // ExternalAllocFunc returns true when allocation succeeds
    template <typename ExternalAllocFunc>
    bool DoExternalAllocation(size_t size, ExternalAllocFunc externalAllocFunc);

#ifdef TRACE_OBJECT_LIFETIME
#define DEFINE_RECYCLER_ALLOC_TRACE(AllocFunc, AllocWithAttributesFunc, attributes) \
    inline char* AllocFunc##Trace(size_t size) \
    { \
        return AllocWithAttributesFunc<(ObjectInfoBits)(attributes | TraceBit), /* nothrow = */ false>(size); \
    }
#else
#define DEFINE_RECYCLER_ALLOC_TRACE(AllocFunc, AllocWithAttributeFunc, attributes)
#endif
#define DEFINE_RECYCLER_ALLOC_BASE(AllocFunc, AllocWithAttributesFunc, attributes) \
    inline char * AllocFunc(DECLSPEC_GUARD_OVERFLOW size_t size) \
    { \
        return AllocWithAttributesFunc<attributes, /* nothrow = */ false>(size); \
    } \
    __forceinline char * AllocFunc##Inlined(DECLSPEC_GUARD_OVERFLOW size_t size) \
    { \
        return AllocWithAttributesFunc##Inlined<attributes, /* nothrow = */ false>(size);  \
    } \
    DEFINE_RECYCLER_ALLOC_TRACE(AllocFunc, AllocWithAttributesFunc, attributes);

#define DEFINE_RECYCLER_NOTHROW_ALLOC_BASE(AllocFunc, AllocWithAttributesFunc, attributes) \
    inline char * NoThrow##AllocFunc(DECLSPEC_GUARD_OVERFLOW size_t size) \
    { \
        return AllocWithAttributesFunc<attributes, /* nothrow = */ true>(size); \
    } \
    inline char * NoThrow##AllocFunc##Inlined(DECLSPEC_GUARD_OVERFLOW size_t size) \
    { \
        return AllocWithAttributesFunc##Inlined<attributes, /* nothrow = */ true>(size);  \
    } \
    DEFINE_RECYCLER_ALLOC_TRACE(AllocFunc, AllocWithAttributesFunc, attributes);

#define DEFINE_RECYCLER_ALLOC(AllocFunc, attributes) DEFINE_RECYCLER_ALLOC_BASE(AllocFunc, AllocWithAttributes, attributes)
#define DEFINE_RECYCLER_ALLOC_ZERO(AllocFunc, attributes) DEFINE_RECYCLER_ALLOC_BASE(AllocFunc, AllocZeroWithAttributes, attributes)

#define DEFINE_RECYCLER_NOTHROW_ALLOC(AllocFunc, attributes) DEFINE_RECYCLER_NOTHROW_ALLOC_BASE(AllocFunc, AllocWithAttributes, attributes)
#define DEFINE_RECYCLER_NOTHROW_ALLOC_ZERO(AllocFunc, attributes) DEFINE_RECYCLER_NOTHROW_ALLOC_BASE(AllocFunc, AllocZeroWithAttributes, attributes)

#if GLOBAL_ENABLE_WRITE_BARRIER && !defined(_WIN32)
    DEFINE_RECYCLER_ALLOC(Alloc, WithBarrierBit);
    DEFINE_RECYCLER_ALLOC_ZERO(AllocZero, WithBarrierBit);
    DEFINE_RECYCLER_ALLOC(AllocFinalized, FinalizableWithBarrierObjectBits);
    DEFINE_RECYCLER_ALLOC(AllocTracked, ClientTrackableObjectWithBarrierBits);
    DEFINE_RECYCLER_ALLOC(AllocFinalizedClientTracked, ClientTrackableObjectWithBarrierBits);
#else
    DEFINE_RECYCLER_ALLOC(Alloc, NoBit);
    DEFINE_RECYCLER_ALLOC_ZERO(AllocZero, NoBit);
    DEFINE_RECYCLER_ALLOC(AllocFinalized, FinalizableObjectBits);
    DEFINE_RECYCLER_ALLOC(AllocTracked, ClientTrackableObjectBits);
    DEFINE_RECYCLER_ALLOC(AllocFinalizedClientTracked, ClientFinalizableObjectBits);
#endif

#ifdef RECYCLER_WRITE_BARRIER_ALLOC
    DEFINE_RECYCLER_ALLOC(AllocWithBarrier, WithBarrierBit);
    DEFINE_RECYCLER_ALLOC_ZERO(AllocZeroWithBarrier, WithBarrierBit);
    DEFINE_RECYCLER_ALLOC(AllocFinalizedWithBarrier, FinalizableWithBarrierObjectBits);
    DEFINE_RECYCLER_ALLOC(AllocTrackedWithBarrier, ClientTrackableObjectWithBarrierBits);
    DEFINE_RECYCLER_ALLOC(AllocFinalizedClientTrackedWithBarrier, ClientFinalizableObjectWithBarrierBits);
#endif

    DEFINE_RECYCLER_ALLOC(AllocLeaf, LeafBit);
    DEFINE_RECYCLER_ALLOC(AllocFinalizedLeaf, FinalizableLeafBits);
    DEFINE_RECYCLER_ALLOC(AllocTrackedLeaf, ClientTrackableLeafBits);
    DEFINE_RECYCLER_ALLOC_ZERO(AllocLeafZero, LeafBit);
    DEFINE_RECYCLER_ALLOC_ZERO(AllocZeroTrackedLeaf, ClientTrackableLeafBits);
    DEFINE_RECYCLER_NOTHROW_ALLOC_ZERO(AllocImplicitRootLeaf, ImplicitRootLeafBits);

    DEFINE_RECYCLER_NOTHROW_ALLOC_ZERO(AllocImplicitRoot, ImplicitRootBit);

    template <ObjectInfoBits enumClass>
    char * AllocEnumClass(DECLSPEC_GUARD_OVERFLOW size_t size)
    {
        Assert((enumClass & EnumClassMask) != 0);
        //Assert((enumClass & ~EnumClassMask & ~WithBarrierBit) == 0);
        return AllocWithAttributes<(ObjectInfoBits)(enumClass), /* nothrow = */ false>(size);
    }

    template <ObjectInfoBits infoBits>
    char * AllocWithInfoBits(DECLSPEC_GUARD_OVERFLOW size_t size)
    {
        return AllocWithAttributes<infoBits, /* nothrow = */ false>(size);
    }

    template<typename T>
    RecyclerWeakReference<T>* CreateWeakReferenceHandle(T* pStrongReference);
    uint GetWeakReferenceCleanupId() const { return weakReferenceCleanupId; }

    template<typename T>
    bool FindOrCreateWeakReferenceHandle(T* pStrongReference, RecyclerWeakReference<T> **ppWeakRef);

    template<typename T>
    bool TryGetWeakReferenceHandle(T* pStrongReference, RecyclerWeakReference<T> **weakReference);

    template <ObjectInfoBits attributes>
    char* GetAddressOfAllocator(size_t sizeCat)
    {
        Assert(HeapInfo::IsAlignedSmallObjectSize(sizeCat));
        return (char*)this->autoHeap.GetBucket<attributes>(sizeCat).GetAllocator();
    }

    template <ObjectInfoBits attributes>
    uint32 GetEndAddressOffset(size_t sizeCat)
    {
        Assert(HeapInfo::IsAlignedSmallObjectSize(sizeCat));
        return this->autoHeap.GetBucket<attributes>(sizeCat).GetAllocator()->GetEndAddressOffset();
    }

    template <ObjectInfoBits attributes>
    uint32 GetFreeObjectListOffset(size_t sizeCat)
    {
        Assert(HeapInfo::IsAlignedSmallObjectSize(sizeCat));
        return this->autoHeap.GetBucket<attributes>(sizeCat).GetAllocator()->GetFreeObjectListOffset();
    }

    void GetNormalHeapBlockAllocatorInfoForNativeAllocation(size_t sizeCat, void*& allocatorAddress, uint32& endAddressOffset, uint32& freeListOffset, bool allowBumpAllocation, bool isOOPJIT);
    static void GetNormalHeapBlockAllocatorInfoForNativeAllocation(void* recyclerAddr, size_t sizeCat, void*& allocatorAddress, uint32& endAddressOffset, uint32& freeListOffset, bool allowBumpAllocation, bool isOOPJIT);
    bool AllowNativeCodeBumpAllocation();
    static void TrackNativeAllocatedMemoryBlock(Recycler * recycler, void * memBlock, size_t sizeCat);

    void Free(void* buffer, size_t size)
    {
        Assert(false);
    }

    bool ExplicitFreeLeaf(void* buffer, size_t size);
    bool ExplicitFreeNonLeaf(void* buffer, size_t size);

    template <ObjectInfoBits attributes>
    bool ExplicitFreeInternalWrapper(void* buffer, size_t allocSize);

    template <ObjectInfoBits attributes, typename TBlockAttributes>
    bool ExplicitFreeInternal(void* buffer, size_t size, size_t sizeCat);

    size_t GetAllocSize(size_t size);

    template <typename TBlockAttributes>
    void SetExplicitFreeBitOnSmallBlock(HeapBlock* heapBlock, size_t sizeCat, void* buffer, ObjectInfoBits attributes);

    char* HeapAllocR(HeapInfo* eHeap, DECLSPEC_GUARD_OVERFLOW size_t size)
    {
        return RealAlloc<LeafBit, /* nothrow = */ false>(eHeap, size);
    }

    void HeapFree(HeapInfo* eHeap,void* candidate);

    void EnumerateObjects(ObjectInfoBits infoBits, void (*CallBackFunction)(void * address, size_t size));

    void RootAddRef(void* obj, uint *count = nullptr);
    void RootRelease(void* obj, uint *count = nullptr);

    template <ObjectInfoBits attributes, bool nothrow>
    inline char* RealAlloc(HeapInfo* heap, DECLSPEC_GUARD_OVERFLOW size_t size);

    template <ObjectInfoBits attributes, bool isSmallAlloc, bool nothrow>
    inline char* RealAllocFromBucket(HeapInfo* heap, DECLSPEC_GUARD_OVERFLOW size_t size);

    void EnterIdleDecommit();
    void LeaveIdleDecommit();

    void DisposeObjects();
    BOOL IsValidObject(void* candidate, size_t minimumSize = 0);

#if DBG
    void SetDisableThreadAccessCheck();
    void SetDisableConcurrentThreadExitedCheck();
    void CheckAllocExternalMark() const;
    BOOL IsFreeObject(void * candidate);
    BOOL IsReentrantState() const;
#endif
#if DBG_DUMP
    void PrintMarkStack();
#endif

#ifdef PROFILE_EXEC
    Js::Profiler * GetProfiler() const { return this->profiler; }
    ArenaAllocator * AddBackgroundProfilerArena();
    void ReleaseBackgroundProfilerArena(ArenaAllocator * arena);
    void SetProfiler(Js::Profiler * profiler, Js::Profiler * backgroundProfiler);
#endif
#ifdef RECYCLER_MEMORY_VERIFY
    BOOL VerifyEnabled() const { return verifyEnabled; }
    uint GetVerifyPad() const { return verifyPad; }
    void Verify(Js::Phase phase);

    static void VerifyCheck(BOOL cond, char16 const * msg, void * address, void * corruptedAddress);
    static void VerifyCheckFill(void * address, size_t size);
    void FillCheckPad(void * address, size_t size, size_t alignedAllocSize, bool objectAlreadyInitialized);
    void FillCheckPad(void * address, size_t size, size_t alignedAllocSize)
    {
        FillCheckPad(address, size, alignedAllocSize, false);
    }
    static void FillPadNoCheck(void * address, size_t size, size_t alignedAllocSize, bool objectAlreadyInitialized);

    void VerifyCheckPad(void * address, size_t size);
    void VerifyCheckPadExplicitFreeList(void * address, size_t size);
    static const byte VerifyMemFill = 0xCA;
#endif
#ifdef RECYCLER_ZERO_MEM_CHECK
    void VerifyZeroFill(void * address, size_t size);
#endif
#ifdef RECYCLER_DUMP_OBJECT_GRAPH
    bool DumpObjectGraph(RecyclerObjectGraphDumper::Param * param = nullptr);
    void DumpObjectDescription(void *object);
#endif
#ifdef LEAK_REPORT
    void ReportLeaks();
    void ReportLeaksOnProcessDetach();
#endif
#ifdef CHECK_MEMORY_LEAK
    void CheckLeaks(char16 const * header);
    void CheckLeaksOnProcessDetach(char16 const * header);
#endif
#ifdef RECYCLER_TRACE
    void SetDomCollect(bool isDomCollect) { collectionParam.domCollect = isDomCollect; }
    void CaptureCollectionParam(CollectionFlags flags, bool repeat = false);
#endif

private:
    // RecyclerRootPtr has implicit conversion to pointers, prevent it to be
    // passed to RootAddRef/RootRelease directly
    template <typename T>
    void RootAddRef(RecyclerRootPtr<T>& ptr, uint *count = nullptr);
    template <typename T>
    void RootRelease(RecyclerRootPtr<T>& ptr, uint *count = nullptr);

    template <CollectionFlags flags>
    BOOL CollectInternal();
    template <CollectionFlags flags>
    BOOL Collect();
    template <CollectionFlags flags>
    BOOL CollectWithHeuristic();
    template <CollectionFlags flags>
    BOOL CollectWithExhaustiveCandidate();
    template <CollectionFlags flags>
    BOOL GetPartialFlag();

    bool NeedExhaustiveRepeatCollect() const;

#if DBG
    bool ExpectStackSkip() const;
#endif

    static size_t const InvalidScanRootBytes = (size_t)-1;

    // Small Allocator
    template <typename SmallHeapBlockAllocatorType>
    void AddSmallAllocator(SmallHeapBlockAllocatorType * allocator, size_t sizeCat);
    template <typename SmallHeapBlockAllocatorType>
    void RemoveSmallAllocator(SmallHeapBlockAllocatorType * allocator, size_t sizeCat);
    template <ObjectInfoBits attributes, typename SmallHeapBlockAllocatorType>
    char * SmallAllocatorAlloc(SmallHeapBlockAllocatorType * allocator, size_t sizeCat, size_t size);

    // Allocation
    template <ObjectInfoBits attributes, bool nothrow>
    inline char * AllocWithAttributesInlined(DECLSPEC_GUARD_OVERFLOW size_t size);
    template <ObjectInfoBits attributes, bool nothrow>
    char * AllocWithAttributes(DECLSPEC_GUARD_OVERFLOW size_t size)
    {
        return AllocWithAttributesInlined<attributes, nothrow>(size);
    }

    template <ObjectInfoBits attributes, bool nothrow>
    inline char* AllocZeroWithAttributesInlined(DECLSPEC_GUARD_OVERFLOW size_t size);

    template <ObjectInfoBits attributes, bool nothrow>
    char* AllocZeroWithAttributes(DECLSPEC_GUARD_OVERFLOW size_t size)
    {
        return AllocZeroWithAttributesInlined<attributes, nothrow>(size);
    }

    char* AllocWeakReferenceEntry(DECLSPEC_GUARD_OVERFLOW size_t size)
    {
        return AllocWithAttributes<WeakReferenceEntryBits, /* nothrow = */ false>(size);
    }

    bool NeedDisposeTimed()
    {
        DWORD ticks = ::GetTickCount();
        return (ticks > tickCountNextDispose && this->hasDisposableObject);
    }

    char* TryLargeAlloc(HeapInfo* heap, DECLSPEC_GUARD_OVERFLOW size_t size, ObjectInfoBits attributes, bool nothrow);

    template <bool nothrow>
    char* LargeAlloc(HeapInfo* heap, DECLSPEC_GUARD_OVERFLOW size_t size, ObjectInfoBits attributes);
    void OutOfMemory();

    // Collection
    BOOL DoCollect(CollectionFlags flags);
    BOOL DoCollectWrapped(CollectionFlags flags);
    BOOL CollectOnAllocatorThread();

#if DBG
    void ResetThreadId();
#endif

    template <bool background>
    size_t ScanPinnedObjects();
    size_t ScanStack();
    size_t ScanArena(ArenaData * alloc, bool background);
    void ScanImplicitRoots();
    void ScanInitialImplicitRoots();
    void ScanNewImplicitRoots();
    size_t FindRoots();
    size_t TryMarkArenaMemoryBlockList(ArenaMemoryBlock * memoryBlocks);
    size_t TryMarkBigBlockList(BigBlock * memoryBlocks);
#if ENABLE_CONCURRENT_GC
#if FALSE // REVIEW: remove this code since not using
    size_t TryMarkBigBlockListWithWriteWatch(BigBlock * memoryBlocks);
#endif
#endif

    // Mark
    void ResetMarks(ResetMarkFlags flags);
    void Mark();
    bool EndMark();
    bool EndMarkCheckOOMRescan();
    void EndMarkOnLowMemory();
#if ENABLE_CONCURRENT_GC
    void DoParallelMark();
    void DoBackgroundParallelMark();
#endif

    size_t RootMark(CollectionState markState);

    void ProcessMark(bool background);
    void ProcessParallelMark(bool background, MarkContext * markContext);
    template <bool parallel, bool interior>
    void ProcessMarkContext(MarkContext * markContext);

public:
    bool IsObjectMarked(void* candidate) { return this->heapBlockMap.IsMarked(candidate); }
#ifdef RECYCLER_STRESS
    bool StressCollectNow();
#endif
private:
    HeapBlock* FindHeapBlock(void * candidate);

    struct FindBlockCache
    {
        FindBlockCache():
            heapBlock(nullptr),
            candidate(nullptr)
        {
        }

        HeapBlock* heapBlock;
        void* candidate;
    } blockCache;

    inline void ScanObjectInline(void ** obj, size_t byteCount);
    inline void ScanObjectInlineInterior(void ** obj, size_t byteCount);
    template <bool doSpecialMark>
    inline void ScanMemoryInline(void ** obj, size_t byteCount);
    template <bool doSpecialMark>
    void ScanMemory(void ** obj, size_t byteCount) { if (byteCount != 0) { ScanMemoryInline<doSpecialMark>(obj, byteCount); } }
    bool AddMark(void * candidate, size_t byteCount);

    // Sweep
#if ENABLE_PARTIAL_GC
    bool Sweep(size_t rescanRootBytes = (size_t)-1, bool concurrent = false, bool adjustPartialHeuristics = false);
#else
    bool Sweep(bool concurrent = false);
#endif
    void SweepWeakReference();
    void SweepHeap(bool concurrent, RecyclerSweep& recyclerSweep);
    void FinishSweep(RecyclerSweep& recyclerSweep);

#if ENABLE_CONCURRENT_GC && ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    void FinishConcurrentSweep();
#endif

    bool FinishDisposeObjects();
    template <CollectionFlags flags>
    bool FinishDisposeObjectsWrapped();

    // end collection
    void FinishCollection();
    void FinishCollection(bool needConcurrentSweep);
    void EndCollection();

    void ResetCollectionState();
    void ResetMarkCollectionState();
    void ResetHeuristicCounters();
    void ResetPartialHeuristicCounters();
    BOOL IsMarkState() const;
    BOOL IsFindRootsState() const;
    BOOL IsInThreadFindRootsState() const;

    template <Js::Phase phase>
    void CollectionBegin();
    template <Js::Phase phase>
    void CollectionEnd();

#if ENABLE_PARTIAL_GC
    void ProcessClientTrackedObjects();
    bool PartialCollect(bool concurrent);
    void FinishPartialCollect(RecyclerSweep * recyclerSweep = nullptr);
    void ClearPartialCollect();
#if ENABLE_CONCURRENT_GC
    void BackgroundFinishPartialCollect(RecyclerSweep * recyclerSweep);
#endif

#endif

    size_t RescanMark(DWORD waitTime);
    size_t FinishMark(DWORD waitTime);
    size_t FinishMarkRescan(bool background);
#if ENABLE_CONCURRENT_GC
    void ProcessTrackedObjects();
#endif

    BOOL IsAllocatableCallbackState()
    {
        return (collectionState & (Collection_PostSweepRedeferralCallback | Collection_PostCollectionCallback));
    }
#if ENABLE_CONCURRENT_GC
    // Concurrent GC
    BOOL IsConcurrentEnabled() const { return this->enableConcurrentMark || this->enableParallelMark || this->enableConcurrentSweep; }
    BOOL IsConcurrentMarkState() const;
    BOOL IsConcurrentMarkExecutingState() const;
    BOOL IsConcurrentResetMarksState() const;
    BOOL IsConcurrentFindRootState() const;
    BOOL IsConcurrentExecutingState() const;
    BOOL IsConcurrentSweepExecutingState() const;
    BOOL IsConcurrentSweepSetupState() const;
    BOOL IsConcurrentSweepState() const;
    BOOL IsConcurrentState() const;
    BOOL InConcurrentSweep()
    {
        return ((collectionState & Collection_ConcurrentSweep) == Collection_ConcurrentSweep);
    }
#if DBG
    BOOL IsConcurrentFinishedState() const;
#endif // DBG

    bool InitializeConcurrent(JsUtil::ThreadService* threadService);
    bool AbortConcurrent(bool restoreState);
    void FinalizeConcurrent(bool restoreState);

    static unsigned int CALLBACK StaticThreadProc(LPVOID lpParameter);
    static int ExceptFilter(LPEXCEPTION_POINTERS pEP);
    DWORD ThreadProc();

    void DoBackgroundWork(bool forceForeground = false);
    static void CALLBACK StaticBackgroundWorkCallback(void * callbackData);

    BOOL CollectOnConcurrentThread();
    bool StartConcurrent(CollectionState const state);
    BOOL StartBackgroundMarkCollect();
    BOOL StartSynchronousBackgroundMark();
    BOOL StartAsynchronousBackgroundMark();
    BOOL StartBackgroundMark(bool foregroundResetMark, bool foregroundFindRoots);
    BOOL StartConcurrentSweepCollect();

    template <CollectionFlags flags>
    BOOL TryFinishConcurrentCollect();
    BOOL WaitForConcurrentThread(DWORD waitTime);
    void FlushBackgroundPages();
    BOOL FinishConcurrentCollect(CollectionFlags flags);
    void FinishTransferSwept(CollectionFlags flags);
    BOOL FinishConcurrentCollectWrapped(CollectionFlags flags);
    void BackgroundMark();
    void BackgroundResetMarks();
    void PrepareBackgroundFindRoots();
    void RevertPrepareBackgroundFindRoots();
    size_t BackgroundFindRoots();
    size_t BackgroundScanStack();
    size_t BackgroundRepeatMark();
    size_t BackgroundRescan(RescanFlags rescanFlags);
    void BackgroundResetWriteWatchAll();
    size_t BackgroundFinishMark();

    char* GetScriptThreadStackTop();

    void SweepPendingObjects(RecyclerSweep& recyclerSweep);
    void ConcurrentTransferSweptObjects(RecyclerSweep& recyclerSweep);
#if ENABLE_PARTIAL_GC
    void ConcurrentPartialTransferSweptObjects(RecyclerSweep& recyclerSweep);
#endif // ENABLE_PARTIAL_GC
#endif // ENABLE_CONCURRENT_GC

    bool ForceSweepObject();
    void NotifyFree(__in char * address, size_t size);
    template <typename T>
    void NotifyFree(T * heapBlock);

    void CleanupPendingUnroot();

#ifdef ENABLE_JS_ETW
    ULONG EventWriteFreeMemoryBlock(HeapBlock* heapBlock);
    void FlushFreeRecord();
    void AppendFreeMemoryETWRecord(__in char *address, size_t size);
    static const uint BulkFreeMemoryCount = 400;
    uint bulkFreeMemoryWrittenCount;
    struct ETWFreeRecord {
        char* memoryAddress;
        uint32 objectSize;
    };
    ETWFreeRecord etwFreeRecords[BulkFreeMemoryCount];
#endif

    template <ObjectInfoBits attributes>
    bool IntegrateBlock(char * blockAddress, PageSegment * segment, size_t allocSize, size_t objectSize);

    template <class TBlockAttributes> friend class SmallHeapBlockT;
    template <class TBlockAttributes> friend class SmallNormalHeapBlockT;
    template <class TBlockAttributes> friend class SmallLeafHeapBlockT;
    template <class TBlockAttributes> friend class SmallFinalizableHeapBlockT;
    friend class LargeHeapBlock;
    friend class HeapInfo;
    friend class LargeHeapBucket;

    template <typename TBlockType>
    friend class HeapBucketT;
    template <typename TBlockType>
    friend class SmallNormalHeapBucketBase;
    template <typename T, ObjectInfoBits attributes>
    friend class RecyclerFastAllocator;

#ifdef RECYCLER_TRACE
    void PrintCollectTrace(Js::Phase phase, bool finish = false, bool noConcurrentWork = false);
#endif
#ifdef RECYCLER_VERIFY_MARK
    void VerifyMark();
    void VerifyMarkRoots();
    void VerifyMarkStack();
    void VerifyMarkArena(ArenaData * arena);
    void VerifyMarkBigBlockList(BigBlock * memoryBlocks);
    void VerifyMarkArenaMemoryBlockList(ArenaMemoryBlock * memoryBlocks);
    bool VerifyMark(void * objectAddress, void * target);
    bool VerifyMark(void * target);
#endif
#if DBG_DUMP
    bool forceTraceMark;
#endif
    bool isHeapEnumInProgress;
#if DBG
    bool allowAllocationDuringHeapEnum;
    bool allowAllocationDuringRenentrance;
#ifdef ENABLE_PROJECTION
    bool isInRefCountTrackingForProjection;
#endif
#endif
    // There are two scenarios we allow limited allocation but disallow GC during those allocations:
    // in heapenum when we allocate PropertyRecord, and
    // in projection ExternalMark allowing allocating VarToDispEx. This is the common flag
    // while we have debug only flag for each of the two scenarios.
    bool isCollectionDisabled;

#ifdef TRACK_ALLOC
public:
    Recycler * TrackAllocInfo(TrackAllocData const& data);
    void ClearTrackAllocInfo(TrackAllocData* data = NULL);

#ifdef PROFILE_RECYCLER_ALLOC
    void PrintAllocStats();
private:
    static bool DoProfileAllocTracker();
    void InitializeProfileAllocTracker();
    void TrackUnallocated(__in  char* address, __in char *endAddress, size_t sizeCat);
    void TrackAllocCore(void * object, size_t size, const TrackAllocData& trackAllocData, bool traceLifetime = false);
    void* TrackAlloc(void * object, size_t size, const TrackAllocData& trackAllocData, bool traceLifetime = false);

    void TrackIntegrate(__in_ecount(blockSize) char * blockAddress, size_t blockSize, size_t allocSize, size_t objectSize, const TrackAllocData& trackAllocData);
    BOOL TrackFree(const char* address, size_t size);

    void TrackAllocWeakRef(RecyclerWeakReferenceBase * weakRef);
    void TrackFreeWeakRef(RecyclerWeakReferenceBase * weakRef);

    struct TrackerData
    {
        TrackerData(type_info const * typeinfo, bool isArray) : typeinfo(typeinfo), isArray(isArray),
            ItemSize(0), ItemCount(0), AllocCount(0), ReqSize(0), AllocSize(0), FreeCount(0), FreeSize(0), TraceLifetime(false)
#ifdef PERF_COUNTERS
            , counter(PerfCounter::RecyclerTrackerCounterSet::GetPerfCounter(typeinfo, isArray))
            , sizeCounter(PerfCounter::RecyclerTrackerCounterSet::GetPerfSizeCounter(typeinfo, isArray))
#endif
        {
        }

        type_info const * typeinfo;
        bool isArray;
#ifdef TRACE_OBJECT_LIFETIME
        bool TraceLifetime;
#endif

        size_t ItemSize;
        size_t ItemCount;
        int AllocCount;
        int64 ReqSize;
        int64 AllocSize;
        int FreeCount;
        int64 FreeSize;
#ifdef PERF_COUNTERS
        PerfCounter::Counter& counter;
        PerfCounter::Counter& sizeCounter;
#endif

        static TrackerData EmptyData;
        static TrackerData ExplicitFreeListObjectData;
    };
    TrackerData * GetTrackerData(void * address);
    void SetTrackerData(void * address, TrackerData * data);

    struct TrackerItem
    {
        TrackerItem(type_info const * typeinfo) : instanceData(typeinfo, false), arrayData(typeinfo, true)
#ifdef PERF_COUNTERS
            , weakRefCounter(PerfCounter::RecyclerTrackerCounterSet::GetWeakRefPerfCounter(typeinfo))
#endif
        {}
        TrackerData instanceData;
        TrackerData arrayData;
#ifdef PERF_COUNTERS
        PerfCounter::Counter& weakRefCounter;
#endif
    };

    typedef JsUtil::BaseDictionary<type_info const *, TrackerItem *, NoCheckHeapAllocator, PrimeSizePolicy, DefaultComparer, JsUtil::SimpleDictionaryEntry, JsUtil::NoResizeLock> TypeInfotoTrackerItemMap;
    typedef JsUtil::BaseDictionary<void *, TrackerData *, NoCheckHeapAllocator, PrimeSizePolicy, RecyclerPointerComparer, JsUtil::SimpleDictionaryEntry, JsUtil::NoResizeLock> PointerToTrackerDataMap;

    TypeInfotoTrackerItemMap * trackerDictionary;
    CriticalSection * trackerCriticalSection;
#endif
    TrackAllocData nextAllocData;
#endif

public:
    // Enumeration
    class AutoSetupRecyclerForNonCollectingMark
    {
    private:
        Recycler& m_recycler;
        bool m_setupDone;
        CollectionState m_previousCollectionState;
#ifdef RECYCLER_STATS
        RecyclerCollectionStats m_previousCollectionStats;
#endif
    public:
        AutoSetupRecyclerForNonCollectingMark(Recycler& recycler, bool setupForHeapEnumeration = false);
        ~AutoSetupRecyclerForNonCollectingMark();
        void DoCommonSetup();
        void SetupForHeapEnumeration();
    };

    friend class RecyclerHeapObjectInfo;

    bool FindImplicitRootObject(void* candidate, RecyclerHeapObjectInfo& heapObject);
    bool FindHeapObject(void* candidate, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject);
    bool FindHeapObjectWithClearedAllocators(void* candidate, RecyclerHeapObjectInfo& heapObject);
    bool IsCollectionDisabled() const { return isCollectionDisabled; }
    bool IsHeapEnumInProgress() const { Assert(isHeapEnumInProgress ? isCollectionDisabled : true); return isHeapEnumInProgress; }

#if DBG
    // There are limited cases that we have to allow allocation during heap enumeration. GC is explicitly
    // disabled during heap enumeration for these limited cases. (See DefaultRecyclerCollectionWrapper)
    // The only case of allocation right now is allocating property record for string based type handler
    // so we can use the propertyId as the relation Id.
    // Allocation during enumeration is still frown upon and should still be avoid if possible.
    bool AllowAllocationDuringHeapEnum() const { return allowAllocationDuringHeapEnum; }
    class AutoAllowAllocationDuringHeapEnum : public AutoBooleanToggle
    {
    public:
        AutoAllowAllocationDuringHeapEnum(Recycler * recycler) : AutoBooleanToggle(&recycler->allowAllocationDuringHeapEnum) {};
    };

#ifdef ENABLE_PROJECTION
    bool IsInRefCountTrackingForProjection() const { return isInRefCountTrackingForProjection;}
    class AutoIsInRefCountTrackingForProjection : public AutoBooleanToggle
    {
    public:
        AutoIsInRefCountTrackingForProjection(Recycler * recycler) : AutoBooleanToggle(&recycler->isInRefCountTrackingForProjection) {};
    };
#endif
#endif

    class AutoAllowAllocationDuringReentrance : public AutoBooleanToggle
    {
    public:
        AutoAllowAllocationDuringReentrance(Recycler * recycler) :
            AutoBooleanToggle(&recycler->isCollectionDisabled)
#if DBG
            , allowAllocationDuringRenentrance(&recycler->allowAllocationDuringRenentrance)
#endif
        {};
#if DBG
    private:
        AutoBooleanToggle allowAllocationDuringRenentrance;
#endif
    };
#ifdef HEAP_ENUMERATION_VALIDATION
    typedef void(*PostHeapEnumScanCallback)(const HeapObject& heapObject, void *data);
    PostHeapEnumScanCallback pfPostHeapEnumScanCallback;
    void *postHeapEnunScanData;
    void PostHeapEnumScan(PostHeapEnumScanCallback callback, void*data);
    bool IsPostEnumHeapValidationInProgress() const { return pfPostHeapEnumScanCallback != NULL; }
#endif

private:
    void* GetRealAddressFromInterior(void* candidate);
    void BeginNonCollectingMark();
    void EndNonCollectingMark();

#if defined(RECYCLER_DUMP_OBJECT_GRAPH) || defined(LEAK_REPORT) || defined(CHECK_MEMORY_LEAK)
public:
    bool IsInDllCanUnloadNow() const { return inDllCanUnloadNow; }
    bool IsInDetachProcess() const { return inDetachProcess; }
    void SetInDllCanUnloadNow();
    void SetInDetachProcess();
private:
    bool inDllCanUnloadNow;
    bool inDetachProcess;
    bool isPrimaryMarkContextInitialized;
#endif
#if defined(LEAK_REPORT) || defined(CHECK_MEMORY_LEAK)
    template <class Fn>
    void ReportOnProcessDetach(Fn fn);
    void PrintPinnedObjectStackTraces();
#endif

public:
    typedef void (CALLBACK *ObjectBeforeCollectCallback)(void* object, void* callbackState); // same as jsrt JsObjectBeforeCollectCallback
    // same as jsrt JsObjectBeforeCollectCallbackWrapper
    typedef void (CALLBACK *ObjectBeforeCollectCallbackWrapper)(ObjectBeforeCollectCallback callback, void* object, void* callbackState, void* threadContext);
    void SetObjectBeforeCollectCallback(void* object,
        ObjectBeforeCollectCallback callback,
        void* callbackState,
        ObjectBeforeCollectCallbackWrapper callbackWrapper,
        void* threadContext);
    void ClearObjectBeforeCollectCallbacks();
    bool IsInObjectBeforeCollectCallback() const { return objectBeforeCollectCallbackState != ObjectBeforeCollectCallback_None; }
private:
    struct ObjectBeforeCollectCallbackData
    {
        ObjectBeforeCollectCallback callback;
        void* callbackState;
        void* threadContext;
        ObjectBeforeCollectCallbackWrapper callbackWrapper;

        ObjectBeforeCollectCallbackData() {}
        ObjectBeforeCollectCallbackData(ObjectBeforeCollectCallbackWrapper callbackWrapper, ObjectBeforeCollectCallback callback, void* callbackState, void* threadContext) :
            callbackWrapper(callbackWrapper), callback(callback), callbackState(callbackState), threadContext(threadContext) {}
    };
    typedef JsUtil::BaseDictionary<void*, ObjectBeforeCollectCallbackData, HeapAllocator,
        PrimeSizePolicy, RecyclerPointerComparer, JsUtil::SimpleDictionaryEntry, JsUtil::NoResizeLock> ObjectBeforeCollectCallbackMap;
    ObjectBeforeCollectCallbackMap* objectBeforeCollectCallbackMap;

    enum ObjectBeforeCollectCallbackState
    {
      ObjectBeforeCollectCallback_None,
      ObjectBeforeCollectCallback_Normal,   // Normal GC BeforeCollect callback
      ObjectBeforeCollectCallback_Shutdown, // At shutdown invoke all BeforeCollect callback
    } objectBeforeCollectCallbackState;

    bool ProcessObjectBeforeCollectCallbacks(bool atShutdown = false);

#if GLOBAL_ENABLE_WRITE_BARRIER
private:
    typedef JsUtil::BaseDictionary<void *, size_t, HeapAllocator, PrimeSizePolicy, RecyclerPointerComparer, JsUtil::SimpleDictionaryEntry, JsUtil::AsymetricResizeLock> PendingWriteBarrierBlockMap;

    PendingWriteBarrierBlockMap pendingWriteBarrierBlockMap;
public:
    void RegisterPendingWriteBarrierBlock(void* address, size_t bytes);
    void UnRegisterPendingWriteBarrierBlock(void* address);
#endif

#if DBG && GLOBAL_ENABLE_WRITE_BARRIER
private:
    static Recycler* recyclerList;
    Recycler* next;
public:
    static void WBSetBitJIT(char* addr)
    {
        return WBSetBit(addr);
    }
    static void WBSetBit(char* addr);
    static void WBSetBitRange(char* addr, uint length);
    static void WBVerifyBitIsSet(char* addr, char* target);
    static bool WBCheckIsRecyclerAddress(char* addr);
#endif
};


class RecyclerHeapObjectInfo
{
    void* m_address;
    Recycler * m_recycler;
    HeapBlock* m_heapBlock;

#if LARGEHEAPBLOCK_ENCODING
    union
    {
        byte * m_attributes;
        LargeObjectHeader * m_largeHeapBlockHeader;
    };
    bool isUsingLargeHeapBlock = false;
#else
    byte * m_attributes;
#endif


public:
    RecyclerHeapObjectInfo() : m_address(NULL), m_recycler(NULL), m_heapBlock(NULL), m_attributes(NULL) {}
    RecyclerHeapObjectInfo(void* address, Recycler * recycler, HeapBlock* heapBlock, byte * attributes) :
        m_address(address), m_recycler(recycler), m_heapBlock(heapBlock), m_attributes(attributes) { }

    void* GetObjectAddress() const { return m_address; }

#ifdef RECYCLER_PAGE_HEAP
    bool IsPageHeapAlloc() const
    {
        return isUsingLargeHeapBlock && ((LargeHeapBlock*)m_heapBlock)->InPageHeapMode();
    }
    void PageHeapLockPages() const
    {
        Assert(IsPageHeapAlloc());
        ((LargeHeapBlock*)m_heapBlock)->PageHeapLockPages();
    }
#endif

    bool IsLeaf() const
    {
#if LARGEHEAPBLOCK_ENCODING
        if (isUsingLargeHeapBlock)
        {
            return (m_largeHeapBlockHeader->GetAttributes(m_recycler->Cookie) & LeafBit) != 0;
        }
#endif
        return ((*m_attributes & LeafBit) != 0 || this->m_heapBlock->IsLeafBlock());
    }

    bool IsImplicitRoot() const
    {
#if LARGEHEAPBLOCK_ENCODING
        if (isUsingLargeHeapBlock)
        {
            return (m_largeHeapBlockHeader->GetAttributes(m_recycler->Cookie) & ImplicitRootBit) != 0;
        }
#endif
        return (*m_attributes & ImplicitRootBit) != 0;
    }
    bool IsObjectMarked() const { Assert(m_recycler); return m_recycler->heapBlockMap.IsMarked(m_address); }
    void SetObjectMarked()  { Assert(m_recycler); m_recycler->heapBlockMap.SetMark(m_address); }
    ObjectInfoBits GetAttributes() const
    {
#if LARGEHEAPBLOCK_ENCODING
        if (isUsingLargeHeapBlock)
        {
            return (ObjectInfoBits)m_largeHeapBlockHeader->GetAttributes(m_recycler->Cookie);
        }
#endif
        return (ObjectInfoBits)*m_attributes;
    }
    size_t GetSize() const;

#if LARGEHEAPBLOCK_ENCODING
    void SetLargeHeapBlockHeader(LargeObjectHeader * largeHeapBlockHeader)
    {
        m_largeHeapBlockHeader = largeHeapBlockHeader;
        isUsingLargeHeapBlock = true;
    }
#endif

    bool SetMemoryProfilerHasEnumerated()
    {
        Assert(m_heapBlock);
#if LARGEHEAPBLOCK_ENCODING
        if (isUsingLargeHeapBlock)
        {
            return SetMemoryProfilerHasEnumeratedForLargeHeapBlock();
        }
#endif
        bool wasMemoryProfilerOldObject = (*m_attributes & MemoryProfilerOldObjectBit) != 0;
        *m_attributes |= MemoryProfilerOldObjectBit;
        return wasMemoryProfilerOldObject;
    }

    bool ClearImplicitRootBit()
    {
        // This can only be called on the main thread for non-finalizable block
        // As finalizable block requires that the bit not be change during concurrent mark
        // since the background thread change the NewTrackBit
        Assert(!m_heapBlock->IsAnyFinalizableBlock());

#ifdef RECYCLER_PAGE_HEAP
        Recycler* recycler = this->m_recycler;
        if (recycler->IsPageHeapEnabled() && recycler->ShouldCapturePageHeapFreeStack())
        {
#ifdef STACK_BACK_TRACE
            if (this->isUsingLargeHeapBlock)
            {
                LargeHeapBlock* largeHeapBlock = (LargeHeapBlock*)this->m_heapBlock;
                if (largeHeapBlock->InPageHeapMode())
                {
                    largeHeapBlock->CapturePageHeapFreeStack();
                }
            }
#endif
        }
#endif

#if LARGEHEAPBLOCK_ENCODING
        if (isUsingLargeHeapBlock)
        {
            return ClearImplicitRootBitsForLargeHeapBlock();
        }
#endif
        Assert(m_attributes);
        bool wasImplicitRoot = (*m_attributes & ImplicitRootBit) != 0;
        *m_attributes &= ~ImplicitRootBit;

        return wasImplicitRoot;
    }

    void ExplicitFree()
    {
        if (*m_attributes == ObjectInfoBits::LeafBit)
        {
            m_recycler->ExplicitFreeLeaf(m_address, GetSize());
        }
        else
        {
            Assert(*m_attributes == ObjectInfoBits::NoBit);
            m_recycler->ExplicitFreeNonLeaf(m_address, GetSize());
        }
    }

#if LARGEHEAPBLOCK_ENCODING
    bool ClearImplicitRootBitsForLargeHeapBlock()
    {
        Assert(m_largeHeapBlockHeader);
        byte attributes = m_largeHeapBlockHeader->GetAttributes(m_recycler->Cookie);
        bool wasImplicitRoot = (attributes & ImplicitRootBit) != 0;
        m_largeHeapBlockHeader->SetAttributes(m_recycler->Cookie, attributes & ~ImplicitRootBit);
        return wasImplicitRoot;
    }

    bool SetMemoryProfilerHasEnumeratedForLargeHeapBlock()
    {
        Assert(m_largeHeapBlockHeader);
        byte attributes = m_largeHeapBlockHeader->GetAttributes(m_recycler->Cookie);
        bool wasMemoryProfilerOldObject = (attributes & MemoryProfilerOldObjectBit) != 0;
        m_largeHeapBlockHeader->SetAttributes(m_recycler->Cookie, attributes | MemoryProfilerOldObjectBit);
        return wasMemoryProfilerOldObject;
    }

#endif
};
// A fake heap block to replace the original heap block where the strong ref is when it has been collected
// as the original heap block may have been freed
class CollectedRecyclerWeakRefHeapBlock : public HeapBlock
{
public:
#if DBG && GLOBAL_ENABLE_WRITE_BARRIER
    virtual void WBVerifyBitIsSet(char* addr) override { Assert(false); }
    virtual void WBSetBit(char* addr) override { Assert(false); }
    virtual void WBSetBitRange(char* addr, uint count) override { Assert(false); }
    virtual void WBClearBit(char* addr) override { Assert(false); }
    virtual void WBClearObject(char* addr) override { Assert(false); }
#endif

#if DBG
    virtual BOOL IsFreeObject(void* objectAddress) override { Assert(false); return false; }
#endif
    virtual BOOL IsValidObject(void* objectAddress) override { Assert(false); return false; }
    virtual byte* GetRealAddressFromInterior(void* interiorAddress) override { Assert(false); return nullptr; }
    virtual size_t GetObjectSize(void* object) const override { Assert(false); return 0; }
    virtual bool FindHeapObject(void* objectAddress, Recycler * recycler, FindHeapObjectFlags flags, RecyclerHeapObjectInfo& heapObject) override { Assert(false); return false; }
    virtual bool TestObjectMarkedBit(void* objectAddress) override { Assert(false); return false; }
    virtual void SetObjectMarkedBit(void* objectAddress) override { Assert(false); }

#ifdef RECYCLER_VERIFY_MARK
    virtual bool VerifyMark(void * objectAddress, void * target) override { Assert(false); return false; }
#endif
#ifdef RECYCLER_PERF_COUNTERS
    virtual void UpdatePerfCountersOnFree() override { Assert(false); }
#endif
#ifdef PROFILE_RECYCLER_ALLOC
    virtual void * GetTrackerData(void * address) override { Assert(false); return nullptr; }
    virtual void SetTrackerData(void * address, void * data) override { Assert(false); }
#endif
    static CollectedRecyclerWeakRefHeapBlock Instance;
private:

    CollectedRecyclerWeakRefHeapBlock() : HeapBlock(BlockTypeCount)
    {
#if ENABLE_CONCURRENT_GC
        isPendingConcurrentSweep = false;
#endif
    }
};

class AutoIdleDecommit
{
public:
    AutoIdleDecommit(Recycler * recycler) : recycler(recycler) { recycler->EnterIdleDecommit(); }
    ~AutoIdleDecommit() { recycler->LeaveIdleDecommit(); }
private:
    Recycler * recycler;
};

template <typename SmallHeapBlockAllocatorType>
void
Recycler::AddSmallAllocator(SmallHeapBlockAllocatorType * allocator, size_t sizeCat)
{
    autoHeap.AddSmallAllocator(allocator, sizeCat);
}

template <typename SmallHeapBlockAllocatorType>
void
Recycler::RemoveSmallAllocator(SmallHeapBlockAllocatorType * allocator, size_t sizeCat)
{
    autoHeap.RemoveSmallAllocator(allocator, sizeCat);
}

template <ObjectInfoBits attributes, typename SmallHeapBlockAllocatorType>
char *
Recycler::SmallAllocatorAlloc(SmallHeapBlockAllocatorType * allocator, DECLSPEC_GUARD_OVERFLOW size_t sizeCat, size_t size)
{
    return autoHeap.SmallAllocatorAlloc<attributes>(this, allocator, sizeCat, size);
}

// Dummy recycler allocator policy classes to choose the allocation function
class _RecyclerLeafPolicy;
class _RecyclerNonLeafPolicy;

#ifdef RECYCLER_WRITE_BARRIER
class _RecyclerWriteBarrierPolicy;
#endif

template <typename Policy>
class _RecyclerAllocatorFunc
{};

template <>
class _RecyclerAllocatorFunc<_RecyclerLeafPolicy>
{
public:
    typedef char * (Recycler::*AllocFuncType)(size_t);
    typedef bool (Recycler::*FreeFuncType)(void*, size_t);

    static AllocFuncType GetAllocFunc()
    {
        return &Recycler::AllocLeaf;
    }

    static AllocFuncType GetAllocZeroFunc()
    {
        return &Recycler::AllocLeafZero;
    }

    static FreeFuncType GetFreeFunc()
    {
        return &Recycler::ExplicitFreeLeaf;
    }
};

template <>
class _RecyclerAllocatorFunc<_RecyclerNonLeafPolicy>
{
public:
    typedef char * (Recycler::*AllocFuncType)(size_t);
    typedef bool (Recycler::*FreeFuncType)(void*, size_t);

    static AllocFuncType GetAllocFunc()
    {
        return &Recycler::Alloc;
    }

    static AllocFuncType GetAllocZeroFunc()
    {
        return &Recycler::AllocZero;
    }

    static FreeFuncType GetFreeFunc()
    {
        return &Recycler::ExplicitFreeNonLeaf;
    }
};

#ifdef RECYCLER_WRITE_BARRIER
template <>
class _RecyclerAllocatorFunc<_RecyclerWriteBarrierPolicy>
{
public:
    typedef char * (Recycler::*AllocFuncType)(size_t);
    typedef bool (Recycler::*FreeFuncType)(void*, size_t);

    static AllocFuncType GetAllocFunc()
    {
        return &Recycler::AllocWithBarrier;
    }

    static AllocFuncType GetAllocZeroFunc()
    {
        return &Recycler::AllocZeroWithBarrier;
    }

    static FreeFuncType GetFreeFunc()
    {
        return &Recycler::ExplicitFreeNonLeaf;
    }
};
#endif

// This is used by the compiler; when T is NOT a pointer i.e. a value type - it causes leaf allocation
template <typename T>
class TypeAllocatorFunc<Recycler, T> : public _RecyclerAllocatorFunc<_RecyclerLeafPolicy>
{
};

#if GLOBAL_ENABLE_WRITE_BARRIER
template <typename T>
class TypeAllocatorFunc<Recycler, T *> : public _RecyclerAllocatorFunc<_RecyclerWriteBarrierPolicy>
{
};
#else
// Partial template specialization; applies to T when it is a pointer
template <typename T>
class TypeAllocatorFunc<Recycler, T *> : public _RecyclerAllocatorFunc<_RecyclerNonLeafPolicy>
{
};
#endif

// Dummy class to choose the allocation function
class RecyclerLeafAllocator
{
public:
    static const bool FakeZeroLengthArray = true;
};
class RecyclerNonLeafAllocator
{
public:
    static const bool FakeZeroLengthArray = true;
};
class RecyclerWriteBarrierAllocator
{
public:
    static const bool FakeZeroLengthArray = true;
};

// Choose RecyclerLeafAllocator / RecyclerNonLeafAllocator based on "bool isLeaf"
template <bool isLeaf>
struct _RecyclerLeaf { typedef RecyclerLeafAllocator AllocatorType; };
template <>
struct _RecyclerLeaf<false> { typedef RecyclerNonLeafAllocator AllocatorType; };

template <bool isLeaf>
class ListTypeAllocatorFunc<Recycler, isLeaf>
{
public:
    // RecyclerLeafAllocator / RecyclerNonLeafAllocator based on "bool isLeaf"
    // used by write barrier type traits
    typedef typename _RecyclerLeaf<isLeaf>::AllocatorType EffectiveAllocatorType;

    typedef char * (Recycler::*AllocFuncType)(size_t);
    typedef bool (Recycler::*FreeFuncType)(void*, size_t);

    static AllocFuncType GetAllocFunc()
    {
        return isLeaf ? &Recycler::AllocLeaf : &Recycler::Alloc;
    }

    static FreeFuncType GetFreeFunc()
    {
        if (isLeaf)
        {
            return &Recycler::ExplicitFreeLeaf;
        }
        else
        {
            return &Recycler::ExplicitFreeNonLeaf;
        }
    }
};

// Partial template specialization to allocate as non leaf
template <typename T>
class TypeAllocatorFunc<RecyclerNonLeafAllocator, T> :
#if GLOBAL_ENABLE_WRITE_BARRIER
    public _RecyclerAllocatorFunc<_RecyclerWriteBarrierPolicy>
#else
    public _RecyclerAllocatorFunc<_RecyclerNonLeafPolicy>
#endif
{
};

#ifdef RECYCLER_WRITE_BARRIER
template <typename T>
class TypeAllocatorFunc<RecyclerWriteBarrierAllocator, T> : public _RecyclerAllocatorFunc<_RecyclerWriteBarrierPolicy>
{
};
#endif

template <typename T>
class TypeAllocatorFunc<RecyclerLeafAllocator, T> : public _RecyclerAllocatorFunc<_RecyclerLeafPolicy>
{
};

template <typename TAllocType>
struct AllocatorInfo<Recycler, TAllocType>
{
    typedef Recycler AllocatorType;
    typedef TypeAllocatorFunc<Recycler, TAllocType> AllocatorFunc;
    typedef _RecyclerAllocatorFunc<_RecyclerNonLeafPolicy> InstAllocatorFunc; // By default any instance considered non-leaf
};

template <typename TAllocType>
struct AllocatorInfo<RecyclerNonLeafAllocator, TAllocType>
{
    typedef Recycler AllocatorType;
    typedef TypeAllocatorFunc<RecyclerNonLeafAllocator, TAllocType> AllocatorFunc;
    typedef TypeAllocatorFunc<RecyclerNonLeafAllocator, TAllocType> InstAllocatorFunc; // Same as TypeAllocatorFunc
};

template <typename TAllocType>
struct AllocatorInfo<RecyclerWriteBarrierAllocator, TAllocType>
{
    typedef Recycler AllocatorType;
    typedef TypeAllocatorFunc<RecyclerWriteBarrierAllocator, TAllocType> AllocatorFunc;
    typedef TypeAllocatorFunc<RecyclerWriteBarrierAllocator, TAllocType> InstAllocatorFunc; // Same as TypeAllocatorFunc
};

template <typename TAllocType>
struct AllocatorInfo<RecyclerLeafAllocator, TAllocType>
{
    typedef Recycler AllocatorType;
    typedef TypeAllocatorFunc<RecyclerLeafAllocator, TAllocType> AllocatorFunc;
    typedef TypeAllocatorFunc<RecyclerLeafAllocator, TAllocType> InstAllocatorFunc; // Same as TypeAllocatorFunc
};

template <>
struct ForceNonLeafAllocator<Recycler>
{
    typedef RecyclerNonLeafAllocator AllocatorType;
};

template <>
struct ForceNonLeafAllocator<RecyclerLeafAllocator>
{
    typedef RecyclerNonLeafAllocator AllocatorType;
};

template <>
struct ForceLeafAllocator<Recycler>
{
    typedef RecyclerLeafAllocator AllocatorType;
};

template <>
struct ForceLeafAllocator<RecyclerNonLeafAllocator>
{
    typedef RecyclerLeafAllocator AllocatorType;
};

// TODO: enable -profile for GC phases.
// access the same profiler object from multiple GC threads which shares one recyler object,
// but profiler object is not thread safe
#if defined(PROFILE_EXEC) && 0
#define RECYCLER_PROFILE_EXEC_BEGIN(recycler, phase) if (recycler->profiler != nullptr) { recycler->profiler->Begin(phase); }
#define RECYCLER_PROFILE_EXEC_END(recycler, phase) if (recycler->profiler != nullptr) { recycler->profiler->End(phase); }

#define RECYCLER_PROFILE_EXEC_BEGIN2(recycler, phase1, phase2) if (recycler->profiler != nullptr) { recycler->profiler->Begin(phase1); recycler->profiler->Begin(phase2);}
#define RECYCLER_PROFILE_EXEC_END2(recycler, phase1, phase2) if (recycler->profiler != nullptr) { recycler->profiler->End(phase1); recycler->profiler->End(phase2);}
#define RECYCLER_PROFILE_EXEC_CHANGE(recycler, phase1, phase2) if  (recycler->profiler != nullptr) { recycler->profiler->End(phase1); recycler->profiler->Begin(phase2); }
#define RECYCLER_PROFILE_EXEC_BACKGROUND_BEGIN(recycler, phase) if (recycler->backgroundProfiler != nullptr) { recycler->backgroundProfiler->Begin(phase); }
#define RECYCLER_PROFILE_EXEC_BACKGROUND_END(recycler, phase) if (recycler->backgroundProfiler != nullptr) { recycler->backgroundProfiler->End(phase); }

#define RECYCLER_PROFILE_EXEC_THREAD_BEGIN(background, recycler, phase) if (background) { RECYCLER_PROFILE_EXEC_BACKGROUND_BEGIN(recycler, phase); } else { RECYCLER_PROFILE_EXEC_BEGIN(recycler, phase); }
#define RECYCLER_PROFILE_EXEC_THREAD_END(background, recycler, phase) if (background) { RECYCLER_PROFILE_EXEC_BACKGROUND_END(recycler, phase); } else { RECYCLER_PROFILE_EXEC_END(recycler, phase); }
#else
#define RECYCLER_PROFILE_EXEC_BEGIN(recycler, phase)
#define RECYCLER_PROFILE_EXEC_END(recycler, phase)
#define RECYCLER_PROFILE_EXEC_BEGIN2(recycler, phase1, phase2)
#define RECYCLER_PROFILE_EXEC_END2(recycler, phase1, phase2)
#define RECYCLER_PROFILE_EXEC_CHANGE(recycler, phase1, phase2)
#define RECYCLER_PROFILE_EXEC_BACKGROUND_BEGIN(recycler, phase)
#define RECYCLER_PROFILE_EXEC_BACKGROUND_END(recycler, phase)
#define RECYCLER_PROFILE_EXEC_THREAD_BEGIN(background, recycler, phase)
#define RECYCLER_PROFILE_EXEC_THREAD_END(background, recycler, phase)
#endif
}

_Ret_notnull_ inline void * __cdecl
operator new(DECLSPEC_GUARD_OVERFLOW size_t byteSize, Recycler * alloc, HeapInfo * heapInfo)
{
    return alloc->HeapAllocR(heapInfo, byteSize);
}

inline void __cdecl
operator delete(void * obj, Recycler * alloc, HeapInfo * heapInfo)
{
    alloc->HeapFree(heapInfo, obj);
}

template<ObjectInfoBits infoBits>
_Ret_notnull_ inline void * __cdecl
operator new(DECLSPEC_GUARD_OVERFLOW size_t byteSize, Recycler * recycler, const InfoBitsWrapper<infoBits>&)
{
    AssertCanHandleOutOfMemory();
    Assert(byteSize != 0);
    void * buffer;

    if (infoBits & EnumClass_1_Bit)
    {
        buffer = recycler->AllocEnumClass<infoBits>(byteSize);
    }
    else
    {
        buffer = recycler->AllocWithInfoBits<infoBits>(byteSize);
    }
    // All of our allocation should throw on out of memory
    Assume(buffer != nullptr);
    return buffer;
}

#if DBG && defined(RECYCLER_VERIFY_MARK)
extern bool IsLikelyRuntimeFalseReference(
    char* objectStartAddress, size_t offset, const char* typeName);
#define DECLARE_RECYCLER_VERIFY_MARK_FRIEND() \
    private: \
        friend bool ::IsLikelyRuntimeFalseReference( \
            char* objectStartAddress, size_t offset, const char* typeName);
#else
#define DECLARE_RECYCLER_VERIFY_MARK_FRIEND()
#endif

template <typename ExternalAllocFunc>
bool Recycler::DoExternalAllocation(size_t size, ExternalAllocFunc externalAllocFunc)
{
    // Request external memory allocation
    if (!RequestExternalMemoryAllocation(size))
    {
        // Attempt to free some memory then try again
        CollectNow<CollectOnTypedArrayAllocation>();
        if (!RequestExternalMemoryAllocation(size))
        {
            return false;
        }
    }
    struct AutoExternalAllocation
    {
        bool allocationSucceeded = false;
        Recycler* recycler;
        size_t size;
        AutoExternalAllocation(Recycler* recycler, size_t size): recycler(recycler), size(size) {}
        // In case the externalAllocFunc throws or fails, the destructor will report the failure
        ~AutoExternalAllocation() { if (!allocationSucceeded) recycler->ReportExternalMemoryFailure(size); }
    };
    AutoExternalAllocation externalAllocation(this, size);
    if (externalAllocFunc())
    {
        this->AddExternalMemoryUsage(size);
        externalAllocation.allocationSucceeded = true;
        return true;
    }
    return false;
}
