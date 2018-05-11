//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"
#include "Base/ThreadContextTlsEntry.h"
#include "Base/ThreadBoundThreadContextManager.h"

ThreadBoundThreadContextManager::EntryList ThreadBoundThreadContextManager::entries(&HeapAllocator::Instance);
#if ENABLE_BACKGROUND_JOB_PROCESSOR
JsUtil::BackgroundJobProcessor * ThreadBoundThreadContextManager::s_sharedJobProcessor = NULL;
#endif
CriticalSection ThreadBoundThreadContextManager::s_sharedJobProcessorCreationLock;

ThreadContext * ThreadBoundThreadContextManager::EnsureContextForCurrentThread()
{
    AutoCriticalSection lock(ThreadContext::GetCriticalSection());

    ThreadContextTLSEntry * entry = ThreadContextTLSEntry::GetEntryForCurrentThread();

    if (entry == NULL)
    {
        ThreadContextTLSEntry::CreateEntryForCurrentThread();
        entry = ThreadContextTLSEntry::GetEntryForCurrentThread();
        entries.Prepend(entry);
    }

    ThreadContext * threadContext = entry->GetThreadContext();

    // An existing TLS entry may have a null ThreadContext
    // DllCanUnload may have cleaned out all the TLS entry when the module lock count is 0,
    // but the library didn't get unloaded because someone is holding onto ref count via LoadLibrary.
    // Just reinitialize the thread context.
    if (threadContext == nullptr)
    {
        bool requireConcurrencySupport = true;
        AllocationPolicyManager * policyManager = HeapNew(AllocationPolicyManager, requireConcurrencySupport);
        threadContext = HeapNew(ThreadContext, policyManager);

        threadContext->SetIsThreadBound();
        if (!ThreadContextTLSEntry::TrySetThreadContext(threadContext))
        {
            HeapDelete(threadContext);
            HeapDelete(policyManager);
            return NULL;
        }
    }

    Assert(threadContext != NULL);

    return threadContext;
}

void ThreadBoundThreadContextManager::DestroyContextAndEntryForCurrentThread()
{
    AutoCriticalSection lock(ThreadContext::GetCriticalSection());

    ThreadContextTLSEntry * entry = ThreadContextTLSEntry::GetEntryForCurrentThread();

    if (entry == NULL)
    {
        return;
    }

    ThreadContext * threadContext = static_cast<ThreadContext *>(entry->GetThreadContext());
    entries.Remove(entry);

    if (threadContext != NULL && threadContext->IsThreadBound())
    {
        ShutdownThreadContext(threadContext);
    }

    ThreadContextTLSEntry::CleanupThread();
}

void ThreadBoundThreadContextManager::DestroyAllContexts()
{
#if ENABLE_BACKGROUND_JOB_PROCESSOR
    JsUtil::BackgroundJobProcessor * jobProcessor = NULL;
#endif
    
    // Since BGParseManager has a dependency on threadcontexts, make sure it shuts down first
    BGParseManager::DeleteBGParseManager();

    {
        AutoCriticalSection lock(ThreadContext::GetCriticalSection());

        ThreadContextTLSEntry * currentEntry = ThreadContextTLSEntry::GetEntryForCurrentThread();

        if (currentEntry == NULL)
        {
            // We need a current thread entry so that we can use it to release any thread contexts
            // we find below.
            try
            {
                AUTO_NESTED_HANDLED_EXCEPTION_TYPE(ExceptionType_OutOfMemory);
                currentEntry = ThreadContextTLSEntry::CreateEntryForCurrentThread();
                entries.Prepend(currentEntry);
            }
            catch (Js::OutOfMemoryException)
            {
                return;
            }
        }
        else
        {
            // We need to clear out the current thread entry so that we can use it to release any
            // thread contexts we find below.
            ThreadContext * threadContext = static_cast<ThreadContext *>(currentEntry->GetThreadContext());

            if (threadContext != NULL)
            {
                if (threadContext->IsThreadBound())
                {
                    ShutdownThreadContext(threadContext);
                    ThreadContextTLSEntry::ClearThreadContext(currentEntry, false);
                }
                else
                {
                    ThreadContextTLSEntry::ClearThreadContext(currentEntry, true);
                }
            }
        }

        EntryList::Iterator iter(&entries);

        while (iter.Next())
        {
            ThreadContextTLSEntry * entry = iter.Data();
            ThreadContext * threadContext =  static_cast<ThreadContext *>(entry->GetThreadContext());

            if (threadContext != nullptr)
            {
                // Found a thread context. Remove it from the containing entry.
                ThreadContextTLSEntry::ClearThreadContext(entry, true);
                // Now set it to our thread's entry.
                ThreadContextTLSEntry::SetThreadContext(currentEntry, threadContext);
                // Clear it out.
                ShutdownThreadContext(threadContext);
                // Now clear it out of our entry.
                ThreadContextTLSEntry::ClearThreadContext(currentEntry, false);
            }
        }

        // We can only clean up our own TLS entry, so we're going to go ahead and do that here.
        entries.Remove(currentEntry);
        ThreadContextTLSEntry::CleanupThread();

#if ENABLE_BACKGROUND_JOB_PROCESSOR
        if (s_sharedJobProcessor != NULL)
        {
            jobProcessor = s_sharedJobProcessor;
            s_sharedJobProcessor = NULL;

            jobProcessor->Close();
        }
#endif
    }

#if ENABLE_BACKGROUND_JOB_PROCESSOR
    if (jobProcessor != NULL)
    {
        HeapDelete(jobProcessor);
    }
#endif
}

void ThreadBoundThreadContextManager::DestroyAllContextsAndEntries(bool shouldDeleteCurrentTlsEntry)
{
    // Since BGParseManager has a dependency on threadcontexts, make sure it shuts down first
    BGParseManager::DeleteBGParseManager();

    AutoCriticalSection lock(ThreadContext::GetCriticalSection());

    // When shouldDeleteCurrentTlsEntry is true, the comparison in the while loop will always be true, so
    // every entry in the list will be deleted.
    ThreadContextTLSEntry* currentThreadEntry = shouldDeleteCurrentTlsEntry ? nullptr : ThreadContextTLSEntry::GetEntryForCurrentThread();

    while (!entries.Empty())
    {
        ThreadContextTLSEntry * entry = entries.Head();
        ThreadContext * threadContext =  static_cast<ThreadContext *>(entry->GetThreadContext());

        entries.RemoveHead();

        if (threadContext != nullptr)
        {
            ShutdownThreadContext(threadContext);
        }

        if (currentThreadEntry != entry)
        {
            // Note: This deletes the ThreadContextTLSEntry but does not remove its pointer
            // from the thread's TLS
            ThreadContextTLSEntry::Delete(entry);
        }
    }

#if ENABLE_BACKGROUND_JOB_PROCESSOR
    if (s_sharedJobProcessor != NULL)
    {
        s_sharedJobProcessor->Close();

        HeapDelete(s_sharedJobProcessor);
        s_sharedJobProcessor = NULL;
    }
#endif
}

JsUtil::JobProcessor * ThreadBoundThreadContextManager::GetSharedJobProcessor()
{
#if ENABLE_BACKGROUND_JOB_PROCESSOR
    if (s_sharedJobProcessor == NULL)
    {
        // Don't use ThreadContext::GetCriticalSection() because it's also locked during thread detach while the loader lock is
        // held, and that may prevent the background job processor's thread from being started due to contention on the loader
        // lock, leading to a deadlock
        AutoCriticalSection lock(&s_sharedJobProcessorCreationLock);

        if (s_sharedJobProcessor == NULL)
        {
            // We don't need to have allocation policy manager for web worker.
            s_sharedJobProcessor = HeapNew(JsUtil::BackgroundJobProcessor, NULL, NULL, false /*disableParallelThreads*/);
        }
    }

    return s_sharedJobProcessor;
#else
    return nullptr;
#endif
}

void RentalThreadContextManager::DestroyThreadContext(ThreadContext* threadContext)
{
    bool deleteThreadContext = true;

#ifdef CHAKRA_STATIC_LIBRARY
    // xplat-todo: Cleanup staticlib shutdown. Deleting contexts / finalizers having
    // trouble with current runtime/context.
    deleteThreadContext = false;
#endif

    ShutdownThreadContext(threadContext, deleteThreadContext);
}

void ThreadContextManagerBase::ShutdownThreadContext(
    ThreadContext* threadContext, bool deleteThreadContext /*= true*/)
{

#if DBG
    PageAllocator* pageAllocator = threadContext->GetPageAllocator();
    if (pageAllocator)
    {
        pageAllocator->SetConcurrentThreadId(::GetCurrentThreadId());
    }
#endif
    threadContext->ShutdownThreads();

    if (deleteThreadContext)
    {
        AllocationPolicyManager * policyManager = threadContext->IsThreadBound() ? threadContext->GetAllocationPolicyManager() : nullptr;
        HeapDelete(threadContext);

        if (policyManager)
        {
            HeapDelete(policyManager);
        }
    }
}
