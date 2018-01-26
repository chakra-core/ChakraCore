//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
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

#if defined(_WIN32) && defined(_M_X64)
    // Enabling store forwards. The intrinsic generates stores matching the load in size.
    // This enables skipping caches and forwarding the store data to the following load.
    *(__m128i *)&markCandidate = _mm_set_epi64x(objectSize, (__int64)objectAddress);
#else
    markCandidate.obj = (void**)objectAddress;
    markCandidate.byteCount = objectSize;
#endif
    return markStack.Push(markCandidate);
}

#ifdef RECYCLER_VISITED_HOST
inline bool MarkContext::AddPreciselyTracedObject(IRecyclerVisitedObject* obj)
{
    FAULTINJECT_MEMORY_MARK_NOTHROW(_u("AddPreciselyTracedObject"), 0);

    return preciseStack.Push(obj);
}
#endif

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
void MarkContext::ScanMemory(void ** obj, size_t byteCount
        ADDRESS_SANITIZER_APPEND(void *asanFakeStack))
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

#if __has_feature(address_sanitizer)
    void *fakeFrameBegin = nullptr;
    void *fakeFrameEnd = nullptr;
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

#if __has_feature(address_sanitizer)
        bool isFakeStackAddr = false;
        if (asanFakeStack)
        {
            void *beg = nullptr;
            void *end = nullptr;
            isFakeStackAddr = __asan_addr_is_in_fake_stack(asanFakeStack, candidate, &beg, &end) != nullptr;
            if (isFakeStackAddr && (beg != fakeFrameBegin || end != fakeFrameEnd))
            {
                ScanMemory<parallel, interior, doSpecialMark>((void**)beg, (char*)end - (char*)beg);
                fakeFrameBegin = beg;
                fakeFrameEnd = end;
            }
        }

        if (!isFakeStackAddr)
        {
#endif
            Mark<parallel, interior, doSpecialMark>(candidate, parentObject);

#if __has_feature(address_sanitizer)
        }
#endif
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

#if defined(RECYCLER_STATS) || !defined(_M_X64)
    if ((size_t)candidate < 0x10000)
    {
        RECYCLER_STATS_INTERLOCKED_INC(recycler, tryMarkNullCount);
        return;
    }
#endif

    if (interior)
    {
        recycler->heapBlockMap.MarkInterior<parallel>(candidate, this);
        return;
    }

#if defined(RECYCLER_STATS) || !defined(_M_X64)
    if (!HeapInfo::IsAlignedAddress(candidate))
    {
        RECYCLER_STATS_INTERLOCKED_INC(recycler, tryMarkUnalignedCount);
        return;
    }
#endif

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
#if ENABLE_ALLOCATIONS_DURING_CONCURRENT_SWEEP
    Assert(!recycler->IsConcurrentExecutingState() && !recycler->IsConcurrentSweepState());
#else
    Assert(!recycler->IsConcurrentExecutingState());
#endif
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

#ifdef RECYCLER_VISITED_HOST
    // Flip between processing the generic mark stack (conservatively traced with ScanMemory) and
    // the precise stack (precisely traced via IRecyclerVisitedObject::Trace). Each of those
    // operations on an object has the potential to add new marked objects to either or both
    // stacks so we must loop until they are both empty.
    while (!markStack.IsEmpty() || !preciseStack.IsEmpty())
#endif
    {
        // It is possible that when the stacks were split, only one of them had any chunks to process.
        // If that is the case, one of the stacks might not be initialized, so we must check !IsEmpty before popping.
        if (!markStack.IsEmpty())
        {
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

                    _mm_prefetch((char *)*(next.obj), _MM_HINT_T0);

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
        }

        Assert(markStack.IsEmpty());

#ifdef RECYCLER_VISITED_HOST
        if (!preciseStack.IsEmpty())
        {
            MarkContextWrapper<parallel> markContextWrapper(this);
            IRecyclerVisitedObject* tracedObject;
            while (preciseStack.Pop(&tracedObject))
            {
                tracedObject->Trace(&markContextWrapper);
            }
        }

        Assert(preciseStack.IsEmpty());
#endif
    }
}
