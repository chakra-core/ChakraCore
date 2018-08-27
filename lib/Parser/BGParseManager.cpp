//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "ParserPch.h"

#include "Memory/AutoPtr.h"
#include "Common/Event.h"
#include "Base/ThreadContextTlsEntry.h"
#include "Base/ThreadBoundThreadContextManager.h"
#include "Base/Utf8SourceInfo.h"
#include "BGParseManager.h"
#include "Base/ScriptContext.h"
#include "ByteCodeSerializer.h"

#define BGPARSE_FLAGS (fscrGlobalCode | fscrWillDeferFncParse | fscrCanDeferFncParse | fscrCreateParserState)

// Global, process singleton
BGParseManager* BGParseManager::s_BGParseManager = nullptr;
DWORD           BGParseManager::s_lastCookie = 0;
DWORD           BGParseManager::s_completed = 0;
DWORD           BGParseManager::s_failed = 0;
CriticalSection BGParseManager::s_staticMemberLock;

// Static member management
BGParseManager* BGParseManager::GetBGParseManager()
{
    AutoCriticalSection lock(&s_staticMemberLock);
    if (s_BGParseManager == nullptr)
    {        
        AUTO_NESTED_HANDLED_EXCEPTION_TYPE(ExceptionType_DisableCheck);
        s_BGParseManager = HeapNew(BGParseManager);
        s_BGParseManager->Processor()->AddManager(s_BGParseManager);
    }
    return s_BGParseManager;
}

void BGParseManager::DeleteBGParseManager()
{
    AutoCriticalSection lock(&s_staticMemberLock);
    if (s_BGParseManager != nullptr)
    {
        BGParseManager* manager = s_BGParseManager;
        s_BGParseManager = nullptr;
        HeapDelete(manager);
    }
}

DWORD BGParseManager::GetNextCookie()
{
    AutoCriticalSection lock(&s_staticMemberLock);
    return ++s_lastCookie;
}

DWORD BGParseManager::IncCompleted()
{
    AutoCriticalSection lock(&s_staticMemberLock);
    return ++s_completed;
}
DWORD BGParseManager::IncFailed()
{
    AutoCriticalSection lock(&s_staticMemberLock);
    return ++s_failed;
}


// Note: runs on any thread
BGParseManager::BGParseManager()
    : JsUtil::WaitableJobManager(ThreadBoundThreadContextManager::GetSharedJobProcessor())
{
}

// Note: runs on any thread
BGParseManager::~BGParseManager()
{
    // First, remove the manager from the JobProcessor so that any remaining jobs
    // are moved back into this manager's processed workitem list
    Processor()->RemoveManager(this);

    Assert(this->workitemsProcessing.IsEmpty());

    // Now, free all remaining processed jobs
    BGParseWorkItem *item = (BGParseWorkItem*)this->workitemsProcessed.Head();
    while (item != nullptr)
    {
        // Get the next item first so that the current item
        // can be safely removed and freed
        BGParseWorkItem* temp = item;
        item = (BGParseWorkItem*)item->Next();

        this->workitemsProcessed.Unlink(temp);
        HeapDelete(temp);
    }
}

// Returns the BGParseWorkItem that matches the provided cookie. Parameters have the following impact:
// - waitForResults: creates an event on the returned WorkItem that the caller can wait for
// - removeJob: removes the job from the list that contained it, and marks it for discard if it is processing
// Note: runs on any thread
BGParseWorkItem* BGParseManager::FindJob(DWORD dwCookie, bool waitForResults, bool removeJob)
{
    Assert(dwCookie != 0);
    Assert(!waitForResults || !removeJob);

    AutoOptionalCriticalSection autoLock(Processor()->GetCriticalSection());
    BGParseWorkItem* matchedWorkitem = nullptr;

    // First, look among processed jobs
    for (BGParseWorkItem *item = this->workitemsProcessed.Head(); item != nullptr && matchedWorkitem == nullptr; item = (BGParseWorkItem*)item->Next())
    {
        if (item->GetCookie() == dwCookie)
        {
            matchedWorkitem = item;
            if (removeJob)
            {
                this->workitemsProcessed.Unlink(matchedWorkitem);
            }
        }
    }

    if (matchedWorkitem == nullptr)
    {
        // Then, look among processing jobs
        for (BGParseWorkItem *item = this->workitemsProcessing.Head(); item != nullptr && matchedWorkitem == nullptr; item = (BGParseWorkItem*)item->Next())
        {
            if (item->GetCookie() == dwCookie)
            {
                matchedWorkitem = item;
                if (removeJob)
                {
                    this->workitemsProcessing.Unlink(matchedWorkitem);
                    // Since the job is still processing, it cannot be freed immediately. Mark it as discarded so
                    // that it can be freed later.
                    matchedWorkitem->Discard();
                }
            }
        }

        // Lastly, look among queued jobs
        if (matchedWorkitem == nullptr)
        {
            Processor()->ForEachJob([&](JsUtil::Job * job) {
                if (job->Manager() == this)
                {
                    BGParseWorkItem* workitem = (BGParseWorkItem*)job;
                    if (workitem->GetCookie() == dwCookie)
                    {
                        matchedWorkitem = workitem;
                        return false;
                    }
                }
                return true;
            });

            if (removeJob && matchedWorkitem != nullptr)
            {
                Processor()->RemoveJob(matchedWorkitem);
            }
        }

        // Since this job isn't already processed and the caller needs the results, create an event
        // that the caller can wait on for results to complete
        if (waitForResults && matchedWorkitem != nullptr)
        {
            // TODO: Is it possible for one event to be shared to reduce the number of heap allocations?
            matchedWorkitem->CreateCompletionEvent();
        }
    }

    return matchedWorkitem;
}

// Creates a new job to parse the provided script on a background thread
// Note: runs on any thread
HRESULT BGParseManager::QueueBackgroundParse(LPCUTF8 pszSrc, size_t cbLength, char16 *fullPath, DWORD* dwBgParseCookie)
{
    HRESULT hr = S_OK;
    if (cbLength > 0)
    {
        BGParseWorkItem* workitem;
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE(ExceptionType_DisableCheck);
            workitem = HeapNew(BGParseWorkItem, this, (const byte *)pszSrc, cbLength, fullPath);
        }

        // Add the job to the processor
        {
            AutoOptionalCriticalSection autoLock(Processor()->GetCriticalSection());
            Processor()->AddJob(workitem, false /*prioritize*/);
        }

        (*dwBgParseCookie) = workitem->GetCookie();

        if (PHASE_TRACE1(Js::BgParsePhase))
        {
            Js::Tick now = Js::Tick::Now();
            Output::Print(
                _u("[BgParse: Start -- cookie: %04d on thread 0x%X at %.2f ms -- %s]\n"),
                workitem->GetCookie(),
                ::GetCurrentThreadId(),
                now.ToMilliseconds(),
                fullPath
            );
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

// Returns the data provided when the parse was queued
// Note: runs on any thread, but the buffer lifetimes are not guaranteed after parse results are returned
HRESULT BGParseManager::GetInputFromCookie(DWORD cookie, LPCUTF8* ppszSrc, size_t* pcbLength)
{
    HRESULT hr = E_FAIL;

    // Find the job associated with this cookie
    BGParseWorkItem* workitem = FindJob(cookie, false /*waitForResults*/, false /*removeJob*/);
    if (workitem != nullptr)
    {
        (*ppszSrc) = workitem->GetScriptSrc();
        (*pcbLength) = workitem->GetScriptLength();
        hr = S_OK;
    }

    return hr;
}

// Deserializes the background parse results into this thread
// Note: *must* run on a UI/Execution thread with an available ScriptContext
HRESULT BGParseManager::GetParseResults(
    Js::ScriptContext* scriptContextUI,
    DWORD cookie,
    LPCUTF8 pszSrc,
    SRCINFO const * pSrcInfo,
    Js::ParseableFunctionInfo** ppFunc,
    CompileScriptException* pse,
    size_t& srcLength,
    Js::Utf8SourceInfo* utf8SourceInfo,
    uint& sourceIndex)
{
    // TODO: Is there a way to cache the environment from which serialization begins to
    // determine whether or not deserialization will succeed? Specifically, being able
    // to assert/compare the flags used during background parse with the flags expected
    // from the UI thread?

    HRESULT hr = E_FAIL;

    // Find the job associated with this cookie
    BGParseWorkItem* workitem = FindJob(cookie, true /*waitForResults*/, false /*removeJob*/);
    if (workitem != nullptr)
    {
        // Synchronously wait for the job to complete
        workitem->WaitForCompletion();
        
        Js::FunctionBody* functionBody = nullptr;
        hr = workitem->DeserializeParseResults(scriptContextUI, pszSrc, pSrcInfo, utf8SourceInfo, &functionBody, srcLength, sourceIndex);
        (*ppFunc) = functionBody;
        workitem->TransferCSE(pse);

        if (hr == S_OK)
        {
            BGParseManager::IncCompleted();
        }
        else
        {
            BGParseManager::IncFailed();
        }
    }

    if (PHASE_TRACE1(Js::BgParsePhase))
    {
        Js::Tick now = Js::Tick::Now();
        Output::Print(
            _u("[BgParse: End   -- cookie: %04d on thread 0x%X at %.2f ms -- hr: 0x%X]\n"),
            workitem != nullptr ? workitem->GetCookie() : -1,
            ::GetCurrentThreadId(),
            now.ToMilliseconds(),
            hr
        );
    }

    return hr;
}

// Finds and removes the workitem associated with the provided cookie. If the workitem is processed
// or not yet processed, the workitem is simply removed and freed. If the workitem is being processed,
// it is removed from the list and will be freed after the job is processed (with its script source
// buffer).
// Returns true when the caller should free the script source buffer. Otherwise, when false is returned,
// the workitem is responsible for freeing the script source buffer.
bool BGParseManager::DiscardParseResults(DWORD cookie, void* buffer)
{
    BGParseWorkItem* workitem = FindJob(cookie, false /*waitForResults*/, true /*removeJob*/);
    bool callerOwnsSourceBuffer = true;
    if (workitem != nullptr)
    {
        Assert(buffer == workitem->GetScriptSrc());

        if (!workitem->IsDiscarded())
        {
            HeapDelete(workitem);
        }
        else
        {
            callerOwnsSourceBuffer = false;
        }
    }

    if (PHASE_TRACE1(Js::BgParsePhase))
    {
        Js::Tick now = Js::Tick::Now();
        Output::Print(
            _u("[BgParse: Discard -- cookie: %04d on thread 0x%X at %.2f ms, workitem: 0x%p, workitem owns buffer: %u]\n"),
            cookie,
            ::GetCurrentThreadId(),
            now.ToMilliseconds(),
            workitem,
            !callerOwnsSourceBuffer
        );
    }

    return callerOwnsSourceBuffer;
}

// Overloaded function called by JobProcessor to do work
// Note: runs on background thread
bool BGParseManager::Process(JsUtil::Job *const job, JsUtil::ParallelThreadData *threadData) 
{
#if ENABLE_BACKGROUND_JOB_PROCESSOR
    Assert(job->Manager() == this);
    
    // Create script context on this thread
    ThreadContext* threadContext = ThreadBoundThreadContextManager::EnsureContextForCurrentThread();

    // If there is no script context created for this thread yet, create it now
    if (threadData->scriptContextBG == nullptr)
    {
        threadData->scriptContextBG = Js::ScriptContext::New(threadContext);
        threadData->scriptContextBG->Initialize();
        threadData->canDecommit = true;
    }
    
    // Parse the workitem's data
    BGParseWorkItem* workItem = (BGParseWorkItem*)job;
    workItem->ParseUTF8Core(threadData->scriptContextBG);

    return true;
#else
    Assert(!"BGParseManager does not work without ThreadContext");
    return false;
#endif
}

// Callback before the provided job will be processed
// Note: runs on any thread
void BGParseManager::JobProcessing(JsUtil::Job* job)
{
    Assert(job->Manager() == this);
    Assert(Processor()->GetCriticalSection()->IsLocked());

    this->workitemsProcessing.LinkToEnd((BGParseWorkItem*)job);
}

// Callback after the provided job was processed. succeeded is true if the job
// was executed as well.
// Note: runs on any thread
void BGParseManager::JobProcessed(JsUtil::Job *const job, const bool succeeded)
{
    Assert(job->Manager() == this);
    Assert(Processor()->GetCriticalSection()->IsLocked());

    BGParseWorkItem* workItem = (BGParseWorkItem*)job;
    if (succeeded)
    {
        Assert(!this->workitemsProcessed.Contains(workItem));
        if (this->workitemsProcessing.Contains(workItem))
        {
            // Move this processed workitem from the processing list to
            // the processed list
            this->workitemsProcessing.Unlink(workItem);
            this->workitemsProcessed.LinkToEnd(workItem);
        }
        else
        {
            // If this workitem isn't in the processing queue, it should
            // already be discarded
            Assert(workItem->IsDiscarded());
        }
    }
    else
    {
        // When the manager is shutting down, workitems are processed through the JobProcessor
        // without executing (i.e., !succeeded). So, mark it for discard so that it can be freed.
        workItem->Discard();
    }

    workItem->JobProcessed(succeeded);
}

// Define needed for jobs.inl
// Note: Runs on any thread
BGParseWorkItem* BGParseManager::GetJob(BGParseWorkItem* workitem)
{
    Assert(!"BGParseManager::WasAddedToJobProcessor");
    return nullptr;
}

// Define needed for jobs.inl
// Note: Runs on any thread
bool BGParseManager::WasAddedToJobProcessor(JsUtil::Job *const job) const
{
    Assert(!"BGParseManager::WasAddedToJobProcessor");
    return true;
}


// Note: runs on any thread
BGParseWorkItem::BGParseWorkItem(
    BGParseManager* manager,
    const byte* pszScript,
    size_t cbScript,
    char16 *fullPath
    )
    : JsUtil::Job(manager),
    script(pszScript),
    cb(cbScript),
    path(nullptr),
    parseHR(S_OK),
    parseSourceLength(0),
    bufferReturn(nullptr),
    bufferReturnBytes(0),
    complete(nullptr),
    discarded(false)
{
    this->cookie = BGParseManager::GetNextCookie();

    Assert(fullPath != nullptr);
    this->path = SysAllocString(fullPath);
}

BGParseWorkItem::~BGParseWorkItem()
{
    SysFreeString(this->path);
    if (this->complete != nullptr)
    {
        HeapDelete(this->complete);
    }

    if (this->bufferReturn != nullptr)
    {
        ::CoTaskMemFree(this->bufferReturn);
    }

    if (this->discarded)
    {
        // When this workitem has been discarded, this is the last reference
        // to the script source, so free it now during destruction.
        ::HeapFree(GetProcessHeap(), 0, (void*)this->script);
    }
}

void BGParseWorkItem::TransferCSE(CompileScriptException* pse)
{
    this->cse.CopyInto(pse);
}

// This function parses the input data cached in BGParseWorkItem and caches the
// seralized bytecode
// Note: runs on BackgroundJobProcessor thread
// Note: All exceptions are caught by BackgroundJobProcessor
void BGParseWorkItem::ParseUTF8Core(Js::ScriptContext* scriptContext)
{
    if (PHASE_TRACE1(Js::BgParsePhase))
    {
        Js::Tick now = Js::Tick::Now();
        Output::Print(
            _u("[BgParse: Parse -- cookie: %04d on thread 0x%X at %.2f ms]\n"),
            GetCookie(),
            ::GetCurrentThreadId(),
            now.ToMilliseconds()
        );
    }

    Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
    SourceContextInfo* sourceContextInfo = scriptContext->GetSourceContextInfo(this->cookie, nullptr);
    if (sourceContextInfo == nullptr)
    {
        sourceContextInfo = scriptContext->CreateSourceContextInfo(this->cookie, this->path, wcslen(this->path), nullptr);
    }

    SRCINFO si = {
        sourceContextInfo,
        0, // dlnHost
        0, // ulColumnHost
        0, // lnMinHost
        0, // ichMinHost
        static_cast<ULONG>(cb / sizeof(utf8char_t)), // ichLimHost
        0, // ulCharOffset
        kmodGlobal, // mod
        0 // grfsi
    };

    ENTER_PINNED_SCOPE(Js::Utf8SourceInfo, sourceInfo);
    sourceInfo = Js::Utf8SourceInfo::NewWithNoCopy(scriptContext, (LPUTF8)this->script, (int32)this->cb, static_cast<int32>(this->cb), &si, false);    

    charcount_t cchLength = 0;
    uint sourceIndex = 0;
    Js::ParseableFunctionInfo * func = nullptr;

    Parser ps(scriptContext);
    this->parseHR = scriptContext->CompileUTF8Core(
        ps,
        sourceInfo,
        &si,
        true, // fOriginalUtf8Code
        this->script,
        this->cb,
        BGPARSE_FLAGS,
        &this->cse,
        cchLength,
        this->parseSourceLength,
        sourceIndex,
        &func,
        nullptr // pDataCache
    );


    if (this->parseHR == S_OK)
    {
        BEGIN_TEMP_ALLOCATOR(tempAllocator, scriptContext, _u("BGParseWorkItem"));
        Js::FunctionBody *functionBody = func->GetFunctionBody();
        this->parseHR = Js::ByteCodeSerializer::SerializeToBuffer(
            scriptContext,
            tempAllocator,
            (DWORD)this->cb,
            this->script,
            functionBody,
            functionBody->GetHostSrcInfo(),
            &this->bufferReturn,
            &this->bufferReturnBytes,
            GENERATE_BYTE_CODE_PARSER_STATE | GENERATE_BYTE_CODE_COTASKMEMALLOC
        );
        END_TEMP_ALLOCATOR(tempAllocator, scriptContext);
        Assert(this->parseHR == S_OK);
    }
    else
    {
        Assert(this->cse.ei.bstrSource != nullptr);
        Assert(func == nullptr);
    }

    LEAVE_PINNED_SCOPE();
}

// Deserializes the background parse results into this thread
// Note: *must* run on a UI/Execution thread with an available ScriptContext
HRESULT BGParseWorkItem::DeserializeParseResults(
    Js::ScriptContext* scriptContextUI,
    LPCUTF8 pszSrc,
    SRCINFO const * pSrcInfo,
    Js::Utf8SourceInfo* utf8SourceInfo,
    Js::FunctionBody** functionBodyReturn,
    size_t& srcLength,
    uint& sourceIndex
)
{
    HRESULT hr = this->parseHR;
    if (hr == S_OK)
    {
        srcLength = this->parseSourceLength;
        sourceIndex = scriptContextUI->SaveSourceNoCopy(utf8SourceInfo, (int)srcLength, false /*isCesu8*/);
        Assert(sourceIndex != Js::Constants::InvalidSourceIndex);

        Field(Js::FunctionBody*) functionBody = nullptr;
        hr = Js::ByteCodeSerializer::DeserializeFromBuffer(
            scriptContextUI,
            BGPARSE_FLAGS,
            (const byte *)pszSrc,
            pSrcInfo,
            this->bufferReturn,
            nullptr, // nativeModule
            &functionBody,
            sourceIndex
        );

        if (hr == S_OK)
        {
            // The buffer is now owned by the output of DeserializeFromBuffer
            (*functionBodyReturn) = functionBody;
            this->bufferReturn = nullptr;
            this->bufferReturnBytes = 0;
        }
    }

    return hr;
}

void BGParseWorkItem::CreateCompletionEvent()
{
    Assert(this->complete == nullptr);
    this->complete = HeapNew(Event, false);
}

// Upon notification of job processed, set the event for those waiting for this job to complete
void BGParseWorkItem::JobProcessed(const bool succeeded)
{
    Assert(Manager()->Processor()->GetCriticalSection()->IsLocked());

    if (IsDiscarded())
    {
        Js::Tick now = Js::Tick::Now();
        Output::Print(
            _u("[BgParse: Discard Before GetResults -- cookie: %04d on thread 0x%X at %.2f ms]\n"),
            GetCookie(),
            ::GetCurrentThreadId(),
            now.ToMilliseconds()
        );

        // When a workitem has been discarded while processing, there are now other
        // references to it, so free it now
        Assert((this->Next() == nullptr && this->Previous() == nullptr) || !succeeded);
        HeapDelete(this);
    }
    else if (this->complete != nullptr)
    {
        this->complete->Set();
    }
}

// Wait for this job to finish processing
void BGParseWorkItem::WaitForCompletion()
{
    if (this->complete != nullptr)
    {
        if (PHASE_TRACE1(Js::BgParsePhase))
        {
            Js::Tick now = Js::Tick::Now();
            Output::Print(
                _u("[BgParse: Wait -- cookie: %04d on thread 0x%X at %.2f ms]\n"),
                GetCookie(),
                ::GetCurrentThreadId(),
                now.ToMilliseconds()
            );
        }

        this->complete->Wait();
    }
}
