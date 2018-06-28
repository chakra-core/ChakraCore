//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

#if defined(_M_IX86) || defined(_M_X64)
// For prefetch
#include <mmintrin.h>
#endif


MarkContext::MarkContext(Recycler * recycler, PagePool * pagePool) :
    recycler(recycler),
    pagePool(pagePool),
    markStack(pagePool),
#ifdef RECYCLER_VISITED_HOST
    preciseStack(pagePool),
#endif
    trackStack(pagePool)
{
}


MarkContext::~MarkContext()
{
#ifdef RECYCLER_MARK_TRACK
    this->markMap = nullptr;
#endif
}

#ifdef RECYCLER_MARK_TRACK
void MarkContext::OnObjectMarked(void* object, void* parent)
{
    if (!this->markMap->ContainsKey(object))
    {
        this->markMap->AddNew(object, parent);
    }
}
#endif

void MarkContext::Init(uint reservedPageCount)
{
    markStack.Init(reservedPageCount);
#ifdef RECYCLER_VISITED_HOST
    preciseStack.Init();
#endif
    trackStack.Init();
}

void MarkContext::Clear()
{
    markStack.Clear();
#ifdef RECYCLER_VISITED_HOST
    preciseStack.Clear();
#endif
    trackStack.Clear();
}

void MarkContext::Abort()
{
    markStack.Abort();
#ifdef RECYCLER_VISITED_HOST
    preciseStack.Abort();
#endif
    trackStack.Abort();

    pagePool->ReleaseFreePages();
}


void MarkContext::Release()
{
    markStack.Release();
#ifdef RECYCLER_VISITED_HOST
    preciseStack.Release();
#endif
    trackStack.Release();

    pagePool->ReleaseFreePages();
}


uint MarkContext::Split(uint targetCount, __in_ecount(targetCount) MarkContext ** targetContexts)
{
#pragma prefast(suppress:__WARNING_REDUNDANTTEST, "Due to implementation of the PageStack template this test may end up being redundant")
    Assert(targetCount > 0 && targetCount <= PageStack<MarkCandidate>::MaxSplitTargets && targetCount <= PageStack<IRecyclerVisitedObject*>::MaxSplitTargets);
    __analysis_assume(targetCount <= PageStack<MarkCandidate>::MaxSplitTargets);
    __analysis_assume(targetCount <= PageStack<IRecyclerVisitedObject*>::MaxSplitTargets);

    PageStack<MarkCandidate> * targetMarkStacks[PageStack<MarkCandidate>::MaxSplitTargets];
#ifdef RECYCLER_VISITED_HOST
    PageStack<IRecyclerVisitedObject*> * targetPreciseStacks[PageStack<IRecyclerVisitedObject*>::MaxSplitTargets];
#endif

    for (uint i = 0; i < targetCount; i++)
    {
        targetMarkStacks[i] = &targetContexts[i]->markStack;
#ifdef RECYCLER_VISITED_HOST
        targetPreciseStacks[i] = &targetContexts[i]->preciseStack;
#endif
    }

    // Return the max count of the two splits - since the stacks have more or less unrelated sizes, they
    // could yield different number of splits, but the caller wants to know the max parallelism it
    // should use on the results of the split.
    const uint markStackSplitCount = this->markStack.Split(targetCount, targetMarkStacks);
#ifdef RECYCLER_VISITED_HOST
    const uint preciseStackSplitCount = this->preciseStack.Split(targetCount, targetPreciseStacks);
    return max(markStackSplitCount, preciseStackSplitCount);
#else
    return markStackSplitCount;
#endif
}


void MarkContext::ProcessTracked()
{
    if (trackStack.IsEmpty())
    {
        return;
    }

    FinalizableObject * trackedObject;
    while (trackStack.Pop(&trackedObject))
    {
        MarkTrackedObject(trackedObject);
    }

    Assert(trackStack.IsEmpty());

    trackStack.Release();
}



