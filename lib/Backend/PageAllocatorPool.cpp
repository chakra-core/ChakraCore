//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#if ENABLE_OOP_NATIVE_CODEGEN
#include "JITServer/JITServer.h"
#include "PageAllocatorPool.h"

CriticalSection PageAllocatorPool::cs;
PageAllocatorPool* PageAllocatorPool::Instance = nullptr;

PageAllocatorPool::PageAllocatorPool()
    :pageAllocators(&NoThrowHeapAllocator::Instance),
    activePageAllocatorCount(0)
{
    idleCleanupTimer = CreateWaitableTimerEx(NULL, L"JITServerIdle", 0/*auto reset*/, TIMER_ALL_ACCESS);
}

PageAllocatorPool::~PageAllocatorPool()
{
    Shutdown();
}

void PageAllocatorPool::Initialize()
{
    Instance = HeapNewNoThrow(PageAllocatorPool);
    if (Instance == nullptr)
    {
        Js::Throw::FatalInternalError();
    }
}
void PageAllocatorPool::Shutdown()
{
    AutoCriticalSection autoCS(&cs);
    if (Instance)
    {
        CloseHandle(Instance->idleCleanupTimer);
        Instance->RemoveAll();
        HeapDelete(Instance);
        Instance = nullptr;
    }
}
void PageAllocatorPool::RemoveAll()
{
    Assert(cs.IsLocked());
    while (!pageAllocators.Empty())
    {
        HeapDelete(pageAllocators.Pop());
    }
}

unsigned int PageAllocatorPool::GetInactivePageAllocatorCount()
{
    AutoCriticalSection autoCS(&cs);
    return pageAllocators.Count();
}

PageAllocator* PageAllocatorPool::GetPageAllocator()
{
    AutoCriticalSection autoCS(&cs);
    PageAllocator* pageAllocator = nullptr;
    if (pageAllocators.Count() > 0)
    {
        // TODO: OOP JIT, select the page allocator with right count of free pages
        // base on some heuristic
        pageAllocator = this->pageAllocators.Pop();
    }
    else
    {
        pageAllocator = HeapNew(PageAllocator, nullptr, Js::Configuration::Global.flags, PageAllocatorType_BGJIT,
            AutoSystemInfo::Data.IsLowMemoryProcess() ? PageAllocator::DefaultLowMaxFreePageCount : PageAllocator::DefaultMaxFreePageCount);
    }

    activePageAllocatorCount++;
    return pageAllocator;

}
void PageAllocatorPool::ReturnPageAllocator(PageAllocator* pageAllocator)
{
    AutoCriticalSection autoCS(&cs);
    if (!this->pageAllocators.PrependNoThrow(&HeapAllocator::Instance, pageAllocator))
    {
        HeapDelete(pageAllocator);
    }

    activePageAllocatorCount--;
    if (activePageAllocatorCount == 0 || GetInactivePageAllocatorCount() > (uint)Js::Configuration::Global.flags.JITServerMaxInactivePageAllocatorCount)
    {
        PageAllocatorPool::IdleCleanup();
    }
}

void PageAllocatorPool::IdleCleanup()
{
    AutoCriticalSection autoCS(&cs);
    if (Instance)
    {
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = Js::Configuration::Global.flags.JITServerIdleTimeout * -10000000LL; // wait for 10 seconds to do the cleanup

        // If the timer is already active when you call SetWaitableTimer, the timer is stopped, then it is reactivated.
        if (!SetWaitableTimer(Instance->idleCleanupTimer, &liDueTime, 0, IdleCleanupRoutine, NULL, 0))
        {
            Instance->RemoveAll();
        }
    }
}

VOID CALLBACK PageAllocatorPool::IdleCleanupRoutine(
    _In_opt_ LPVOID lpArgToCompletionRoutine,
    _In_     DWORD  dwTimerLowValue,
    _In_     DWORD  dwTimerHighValue)
{
    AutoCriticalSection autoCS(&cs);
    if (Instance)
    {
        // TODO: OOP JIT, use better stragtegy to do the cleanup, like do not remove all,
        // instead keep couple inactivate page allocator for next calls
        Instance->RemoveAll();
    }
}
#endif
