//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"
#include "Base/ThreadContextTlsEntry.h"

static THREAD_LOCAL ThreadContextTLSEntry* s_tlvSlot = nullptr;

void ThreadContextTLSEntry::CleanupThread()
{
    ThreadContextTLSEntry * entry = s_tlvSlot;

    if (entry != nullptr)
    {
        HeapDelete(entry);
        s_tlvSlot = nullptr;
    }
}

bool ThreadContextTLSEntry::TrySetThreadContext(ThreadContext * threadContext)
{
    Assert(threadContext != NULL);

    DWORD threadContextThreadId = threadContext->GetCurrentThreadId();

    // If a thread context is current on another thread, then you cannot set it to current on this one.
    if (threadContextThreadId != ThreadContext::NoThread && threadContextThreadId != ::GetCurrentThreadId())
    {
        // the thread doesn't support rental thread and try to set on a different thread???
        Assert(!threadContext->IsThreadBound());
        return false;
    }

    ThreadContextTLSEntry * entry = s_tlvSlot;

    if (entry == nullptr)
    {
        Assert(!threadContext->IsThreadBound());
        entry = CreateEntryForCurrentThread();
        s_tlvSlot = entry;
    }
    else if (entry->threadContext != NULL && entry->threadContext != threadContext)
    {
        // If the thread has an active thread context and either that thread context is thread
        // bound (in which case it cannot be moved off this thread), or if the thread context
        // is running script, you cannot move it off this thread.
        if (entry->threadContext->IsThreadBound() || entry->threadContext->IsInScript())
        {
            return false;
        }

        ClearThreadContext(entry, true);
    }

    SetThreadContext(entry, threadContext);

    return true;
}

void ThreadContextTLSEntry::SetThreadContext(ThreadContextTLSEntry * entry, ThreadContext * threadContext)
{
    entry->threadContext = threadContext;
    threadContext->SetStackProber(&entry->prober);
    threadContext->SetCurrentThreadId(::GetCurrentThreadId());
}

bool ThreadContextTLSEntry::ClearThreadContext(bool isValid)
{
    return ClearThreadContext(s_tlvSlot, isValid, false);
}

bool ThreadContextTLSEntry::ClearThreadContext(ThreadContextTLSEntry * entry, bool isThreadContextValid, bool force)
{
    if (entry != nullptr)
    {
        if (entry->threadContext != NULL && isThreadContextValid)
        {
            // If the thread has an active thread context and either that thread context is thread
            // bound (in which case it cannot be moved off this thread), or if the thread context
            // is running script, you cannot move it off this thread.
            if (!force && (entry->threadContext->IsThreadBound() || entry->threadContext->IsInScript()))
            {
                return false;
            }
            entry->threadContext->SetCurrentThreadId(ThreadContext::NoThread);
            entry->threadContext->SetStackProber(NULL);
        }

        entry->threadContext = NULL;
    }

    return true;
}

void ThreadContextTLSEntry::Delete(ThreadContextTLSEntry * entry)
{
    HeapDelete(entry);
}

ThreadContextTLSEntry * ThreadContextTLSEntry::GetEntryForCurrentThread()
{
    return s_tlvSlot;
}

ThreadContextTLSEntry * ThreadContextTLSEntry::CreateEntryForCurrentThread()
{
    ThreadContextTLSEntry * entry = HeapNewStructZ(ThreadContextTLSEntry);
#pragma prefast(suppress:6001, "Memory from HeapNewStructZ are zero initialized")
    entry->prober.Initialize();
    s_tlvSlot = entry;

    return entry;
}

ThreadContext * ThreadContextTLSEntry::GetThreadContext()
{
    return this->threadContext;
}

ThreadContextId ThreadContextTLSEntry::GetCurrentThreadContextId()
{
    ThreadContextTLSEntry * entry = s_tlvSlot;
    if (entry != nullptr && entry->GetThreadContext() != NULL)
    {
        return (ThreadContextId)entry->GetThreadContext();
    }

    return NoThreadContextId;
}

ThreadContextId ThreadContextTLSEntry::GetThreadContextId(ThreadContext * threadContext)
{
    return threadContext;
}
