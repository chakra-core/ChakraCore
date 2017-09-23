//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
class ThreadContextManagerBase
{
protected:

    static void ShutdownThreadContext(
        ThreadContext* threadContext, bool deleteThreadContext = true);
};

class ThreadBoundThreadContextManager : public ThreadContextManagerBase
{
    friend class ThreadContext;

public:
    typedef DList<ThreadContextTLSEntry *, HeapAllocator> EntryList;

    static ThreadContext * EnsureContextForCurrentThread();
    static void DestroyContextAndEntryForCurrentThread();
    static void DestroyAllContexts();
    static void DestroyAllContextsAndEntries();
    static JsUtil::JobProcessor * GetSharedJobProcessor();
private:
    static EntryList entries;
#if ENABLE_BACKGROUND_JOB_PROCESSOR
    static JsUtil::BackgroundJobProcessor * s_sharedJobProcessor;
#endif
    static CriticalSection s_sharedJobProcessorCreationLock;
};

class RentalThreadContextManager : public ThreadContextManagerBase
{
public:
    static void DestroyThreadContext(ThreadContext* threadContext);

};
