//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

inline
bool MarkContext::AddMarkedObject(void * objectAddress, size_t objectSize)
{
    Assert(objectAddress != nullptr);
    Assert(objectSize > 0);
    Assert(objectSize % sizeof(void *) == 0);

    FAULTINJECT_MEMORY_MARK_NOTHROW(_u("AddMarkedObject"), objectSize);

#if DBG_DUMP
    if (recycler->forceTraceMark || recycler->GetRecyclerFlagsTable().Trace.IsEnabled(Js::MarkPhase))
    {
        Output::Print(_u(" %p"), objectAddress);
    }
#endif

    RECYCLER_STATS_INTERLOCKED_INC(recycler, scanCount);

    MarkCandidate markCandidate;
    markCandidate.obj = (void **) objectAddress;
    markCandidate.byteCount = objectSize;
    return markStack.Push(markCandidate);
}

#if ENABLE_CONCURRENT_GC
inline
bool MarkContext::AddTrackedObject(FinalizableObject * obj)
{
    Assert(obj != nullptr);
#if ENABLE_CONCURRENT_GC
    Assert(recycler->DoQueueTrackedObject());
#endif
#if ENABLE_PARTIAL_GC
    Assert(!recycler->inPartialCollectMode);
#endif

    FAULTINJECT_MEMORY_MARK_NOTHROW(_u("AddTrackedObject"), 0);

    return trackStack.Push(obj);
}
#endif

template <bool parallel, bool interior, bool doSpecialMark>
NO_SANITIZE_ADDRESS
inline
void MarkContext::ScanMemory(void ** obj, size_t byteCount)
{
    Assert(byteCount != 0);
    Assert(byteCount % sizeof(void *) == 0);

    void ** objEnd = obj + (byteCount / sizeof(void *));
    void * parentObject = (void*)obj;

#if DBG_DUMP
    if (recycler->forceTraceMark || recycler->GetRecyclerFlagsTable().Trace.IsEnabled(Js::MarkPhase))
    {
        Output::Print(_u("Scanning %p(%8d): "), obj, byteCount);
    }
#endif

    do
    {
        // We need to ensure that the compiler does not reintroduce reads to the object after inlining.
        // This could cause the value to change after the marking checks (e.g., the null/low address check).
        // Intrinsics avoid the expensive memory barrier on ARM (due to /volatile:ms).
#if defined(_M_ARM64)
        void * candidate = reinterpret_cast<void *>(__iso_volatile_load64(reinterpret_cast<volatile __int64 *>(obj)));
#elif defined(_M_ARM)
        void * candidate = reinterpret_cast<void *>(__iso_volatile_load32(reinterpret_cast<volatile __int32 *>(obj)));
#else
        void * candidate = *(static_cast<void * volatile *>(obj));
#endif

#if DBG
        this->parentRef = obj;
#endif
        Mark<parallel, interior, doSpecialMark>(candidate, parentObject);
        obj++;
    } while (obj != objEnd);

#if DBG
    this->parentRef = nullptr;
#endif

#if DBG_DUMP
    if (recycler->forceTraceMark || recycler->GetRecyclerFlagsTable().Trace.IsEnabled(Js::MarkPhase))
    {
        Output::Print(_u("\n"));
        Output::Flush();
    }
#endif
}


template <bool parallel, bool interior>
inline
void MarkContext::ScanObject(void ** obj, size_t byteCount)
{
    BEGIN_DUMP_OBJECT(recycler, obj);

    ScanMemory<parallel, interior, false>(obj, byteCount);

    END_DUMP_OBJECT(recycler);
}


template <bool parallel, bool interior, bool doSpecialMark>
inline
void MarkContext::Mark(void * candidate, void * parentReference)
{
    // We should never reach here while we are processing Rescan.
    // Otherwise our rescanState could be out of sync with mark state.
    Assert(!recycler->isProcessingRescan);

    if ((size_t)candidate < 0x10000)
    {
        RECYCLER_STATS_INTERLOCKED_INC(recycler, tryMarkNullCount);
        return;
    }

    if (interior)
    {
#if ENABLE_CONCURRENT_GC
        Assert(recycler->enableScanInteriorPointers
            || (!recycler->IsConcurrentState() && recycler->collectionState != CollectionStateParallelMark));
#else
        Assert(recycler->enableScanInteriorPointers || recycler->collectionState != CollectionStateParallelMark);
#endif
        recycler->heapBlockMap.MarkInterior<parallel>(candidate, this);
        return;
    }

    if (!HeapInfo::IsAlignedAddress(candidate))
    {
        RECYCLER_STATS_INTERLOCKED_INC(recycler, tryMarkUnalignedCount);
        return;
    }

    recycler->heapBlockMap.Mark<parallel, doSpecialMark>(candidate, this);

#ifdef RECYCLER_MARK_TRACK
    this->OnObjectMarked(candidate, parentReference);
#endif
}

inline
void MarkContext::MarkTrackedObject(FinalizableObject * trackedObject)
{
#if ENABLE_CONCURRENT_GC
    Assert(!recycler->queueTrackedObject);
    Assert(!recycler->IsConcurrentExecutingState());
#endif
#if ENABLE_PARTIAL_GC
    Assert(!recycler->inPartialCollectMode);
#endif
    Assert(!(recycler->collectionState == CollectionStateParallelMark));

    // Mark is not expected to throw.
    BEGIN_NO_EXCEPTION
    {
        trackedObject->Mark(recycler);
    }
    END_NO_EXCEPTION
}

template <bool parallel, bool interior>
inline
void MarkContext::ProcessMark()
{
#ifdef RECYCLER_STRESS
    if (recycler->GetRecyclerFlagsTable().RecyclerInduceFalsePositives)
    {
        // InduceFalsePositives logic doesn't support parallel marking
        if (!parallel)
        {
            recycler->heapBlockMap.InduceFalsePositives(recycler);
        }
    }
#endif

#if defined(_M_IX86) || defined(_M_X64)
    MarkCandidate current, next;

    while (markStack.Pop(&current))
    {
        // Process entries and prefetch as we go.
        while (markStack.Pop(&next))
        {
            // Prefetch the next entry so it's ready when we need it.
            _mm_prefetch((char *)next.obj, _MM_HINT_T0);

            // Process the previously retrieved entry.
            ScanObject<parallel, interior>(current.obj, current.byteCount);

            current = next;
        }

        // The stack is empty, but we still have a previously retrieved entry; process it now.
        ScanObject<parallel, interior>(current.obj, current.byteCount);

        // Processing that entry may have generated more entries in the mark stack, so continue the loop.
    }
#else
    // _mm_prefetch intrinsic is specific to Intel platforms.
    // CONSIDER: There does seem to be a compiler intrinsic for prefetch on ARM,
    // however, the information on this is scarce, so for now just don't do prefetch on ARM.
    MarkCandidate current;

    while (markStack.Pop(&current))
    {
        ScanObject<parallel, interior>(current.obj, current.byteCount);
    }
#endif

    Assert(markStack.IsEmpty());
}
