//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
interface IRecyclerVisitedObject;

namespace Memory
{
class Recycler;

typedef JsUtil::SynchronizedDictionary<void *, void *, NoCheckHeapAllocator, PrimeSizePolicy, RecyclerPointerComparer, JsUtil::SimpleDictionaryEntry, Js::DefaultContainerLockPolicy, CriticalSection> MarkMap;

#if __has_feature(address_sanitizer)
enum class RecyclerScanMemoryType { General, Stack };
#endif

class MarkContext
{
private:
    struct MarkCandidate
    {
        void ** obj;
        size_t byteCount;
    };

public:
    static const int MarkCandidateSize = sizeof(MarkCandidate);

    MarkContext(Recycler * recycler, PagePool * pagePool);
    ~MarkContext();

    void Init(uint reservedPageCount);
    void Clear();

    Recycler * GetRecycler() { return this->recycler; }

    bool AddMarkedObject(void * obj, size_t byteCount);
#ifdef RECYCLER_VISITED_HOST
    bool AddPreciselyTracedObject(IRecyclerVisitedObject *obj);
#endif
#if ENABLE_CONCURRENT_GC
    bool AddTrackedObject(FinalizableObject * obj);
#endif

    template <bool parallel, bool interior, bool doSpecialMark>
    void Mark(void * candidate, void * parentReference);
    template <bool parallel>
    void MarkInterior(void * candidate);
    template <bool parallel, bool interior>
    void ScanObject(void ** obj, size_t byteCount);

    template <bool parallel, bool interior, bool doSpecialMark>
    void ScanMemory(void ** obj, size_t byteCount
            ADDRESS_SANITIZER_APPEND(void *asanFakeStack = nullptr));

    template <bool parallel, bool interior>
    void ProcessMark();

    void MarkTrackedObject(FinalizableObject * obj);
    void ProcessTracked();

    uint Split(uint targetCount, __in_ecount(targetCount) MarkContext ** targetContexts);

    void Abort();
    void Release();

    bool HasPendingMarkObjects() const { return !markStack.IsEmpty(); }
#ifdef RECYCLER_VISITED_HOST
    bool HasPendingPreciselyTracedObjects() const { return !preciseStack.IsEmpty(); }
#endif
    bool HasPendingTrackObjects() const { return !trackStack.IsEmpty(); }
    bool HasPendingObjects() const {
        return HasPendingMarkObjects()
#ifdef RECYCLER_VISITED_HOST
            || HasPendingPreciselyTracedObjects()
#endif
            || HasPendingTrackObjects();
    }

    PageAllocator * GetPageAllocator() { return this->pagePool->GetPageAllocator(); }

    bool IsEmpty()
    {
        if (HasPendingObjects())
        {
            return false;
        }

        Assert(pagePool->IsEmpty());
        Assert(!GetPageAllocator()->DisableAllocationOutOfMemory());
        return true;
    }

#if DBG
    void VerifyPostMarkState()
    {
        Assert(this->markStack.HasChunk());
    }
#endif

    void Cleanup()
    {
        Assert(!HasPendingObjects());
        Assert(!GetPageAllocator()->DisableAllocationOutOfMemory());
        this->pagePool->ReleaseFreePages();
    }

    void DecommitPages() { this->pagePool->Decommit(); }


#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    void SetMaxPageCount(size_t maxPageCount) {
        markStack.SetMaxPageCount(maxPageCount);
#ifdef RECYCLER_VISITED_HOST
        preciseStack.SetMaxPageCount(maxPageCount);
#endif
        trackStack.SetMaxPageCount(maxPageCount);
    }
#endif

#ifdef RECYCLER_MARK_TRACK
    void SetMarkMap(MarkMap* markMap)
    {
        this->markMap = markMap;
    }
#endif

private:
    Recycler * recycler;
    PagePool * pagePool;
    PageStack<MarkCandidate> markStack;
#ifdef RECYCLER_VISITED_HOST
    PageStack<IRecyclerVisitedObject*> preciseStack;
#endif
    PageStack<FinalizableObject *> trackStack;

#ifdef RECYCLER_MARK_TRACK
    MarkMap* markMap;

    void OnObjectMarked(void* object, void* parent);
#endif

#if DBG
public:
    void* parentRef;
#endif
};


}
