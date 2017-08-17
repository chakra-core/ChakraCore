//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include <JsrtPch.h>
#include "JsrtRuntime.h"
#include "jsrtHelper.h"
#include "Base/ThreadContextTlsEntry.h"
#include "Base/ThreadBoundThreadContextManager.h"
JsrtRuntime::JsrtRuntime(ThreadContext * threadContext, bool useIdle, bool dispatchExceptions)
{
    Assert(threadContext != NULL);
    this->threadContext = threadContext;
    this->contextList = NULL;
    this->collectCallback = NULL;
    this->beforeCollectCallback = NULL;
    this->callbackContext = NULL;
    this->allocationPolicyManager = threadContext->GetAllocationPolicyManager();
    this->useIdle = useIdle;
    this->dispatchExceptions = dispatchExceptions;
    if (useIdle)
    {
        this->threadService.Initialize(threadContext);
    }
    threadContext->SetJSRTRuntime(this);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    serializeByteCodeForLibrary = false;
#endif
#ifdef ENABLE_SCRIPT_DEBUGGING
    this->jsrtDebugManager = nullptr;
#endif
}

JsrtRuntime::~JsrtRuntime()
{
    HeapDelete(allocationPolicyManager);
#ifdef ENABLE_SCRIPT_DEBUGGING
    if (this->jsrtDebugManager != nullptr)
    {
        HeapDelete(this->jsrtDebugManager);
        this->jsrtDebugManager = nullptr;
    }
#endif
}

// This is called at process detach.
// threadcontext created from runtime should not be destroyed in ThreadBoundThreadContext
// we should clean them up at process detach only as runtime can be used in other threads
// even after the current physical thread was destroyed.
// This is called after ThreadBoundThreadContext are cleaned up, so the remaining items
// in the globalthreadContext linklist should be for jsrt only.
void JsrtRuntime::Uninitialize()
{
    ThreadContext* currentThreadContext = ThreadContext::GetThreadContextList();
    ThreadContext* tmpThreadContext;
    while (currentThreadContext)
    {
        Assert(!currentThreadContext->IsScriptActive());
        JsrtRuntime* currentRuntime = static_cast<JsrtRuntime*>(currentThreadContext->GetJSRTRuntime());
        tmpThreadContext = currentThreadContext;
        currentThreadContext = currentThreadContext->Next();

#ifdef CHAKRA_STATIC_LIBRARY
        // xplat-todo: Cleanup staticlib shutdown. This only shuts down threads.
        // Other closing contexts / finalizers having trouble with current
        // runtime/context.
        RentalThreadContextManager::DestroyThreadContext(tmpThreadContext);
#else
        currentRuntime->CloseContexts();
        RentalThreadContextManager::DestroyThreadContext(tmpThreadContext);
        HeapDelete(currentRuntime);
#endif
    }
}

void JsrtRuntime::CloseContexts()
{
    while (this->contextList != NULL)
    {
        this->contextList->Dispose(false);
        // This will remove it from the list
    }
}

void JsrtRuntime::SetBeforeCollectCallback(JsBeforeCollectCallback beforeCollectCallback, void * callbackContext)
{
    if (beforeCollectCallback != NULL)
    {
        if (this->collectCallback == NULL)
        {
            this->collectCallback = this->threadContext->AddRecyclerCollectCallBack(RecyclerCollectCallbackStatic, this);
        }

        this->beforeCollectCallback = beforeCollectCallback;
        this->callbackContext = callbackContext;
    }
    else
    {
        if (this->collectCallback != NULL)
        {
            this->threadContext->RemoveRecyclerCollectCallBack(this->collectCallback);
            this->collectCallback = NULL;
        }

        this->beforeCollectCallback = NULL;
        this->callbackContext = NULL;
    }
}

void JsrtRuntime::RecyclerCollectCallbackStatic(void * context, RecyclerCollectCallBackFlags flags)
{
    if (flags & Collect_Begin)
    {
        JsrtRuntime * _this = reinterpret_cast<JsrtRuntime *>(context);
        try
        {
            JsrtCallbackState scope(reinterpret_cast<ThreadContext*>(_this->GetThreadContext()));
            _this->beforeCollectCallback(_this->callbackContext);
        }
        catch (...)
        {
            AssertMsg(false, "Unexpected non-engine exception.");
        }
    }
}

unsigned int JsrtRuntime::Idle()
{
    return this->threadService.Idle();
}

#ifdef ENABLE_SCRIPT_DEBUGGING
void JsrtRuntime::EnsureJsrtDebugManager()
{
    if (this->jsrtDebugManager == nullptr)
    {
        this->jsrtDebugManager = HeapNew(JsrtDebugManager, this->threadContext);
    }
    Assert(this->jsrtDebugManager != nullptr);
}

void JsrtRuntime::DeleteJsrtDebugManager()
{
    if (this->jsrtDebugManager != nullptr)
    {
        HeapDelete(this->jsrtDebugManager);
        this->jsrtDebugManager = nullptr;
    }
}

JsrtDebugManager * JsrtRuntime::GetJsrtDebugManager()
{
    return this->jsrtDebugManager;
}

#if ENABLE_TTD
uint32 JsrtRuntime::BPRegister_TTD(int64 bpID, Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* utf8SourceInfo, uint32 line, uint32 column, BOOL* isNewBP)
{
    TTDAssert(this->jsrtDebugManager != nullptr, "This needs to be setup before registering any breakpoints.");

    Js::BreakpointProbe* probe = this->jsrtDebugManager->SetBreakpointHelper_TTD(bpID, scriptContext, utf8SourceInfo, line, column, isNewBP);
    return probe->GetId();
}

void JsrtRuntime::BPDelete_TTD(uint32 bpID)
{
    TTDAssert(this->jsrtDebugManager != nullptr, "This needs to be setup before deleting any breakpoints.");

    this->jsrtDebugManager->GetDebugDocumentManager()->RemoveBreakpoint(bpID);
}

void JsrtRuntime::BPClearDocument_TTD()
{
    TTDAssert(this->jsrtDebugManager != nullptr, "This needs to be setup before deleting any breakpoints.");

    this->jsrtDebugManager->ClearBreakpointDebugDocumentDictionary();
}
#endif
#endif
