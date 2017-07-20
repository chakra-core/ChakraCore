//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "JsrtRuntime.h"
#include "Base/ThreadContextTlsEntry.h"

static THREAD_LOCAL JsrtContext* s_tlvSlot = nullptr;

JsrtContext::JsrtContext(JsrtRuntime * runtime) :
    runtime(runtime), javascriptLibrary(nullptr)
{
}

void JsrtContext::SetJavascriptLibrary(Js::JavascriptLibrary * library)
{
    this->javascriptLibrary = library;
    if (this->javascriptLibrary)
    {
        this->javascriptLibrary->SetJsrtContext(this);
    }
}

void JsrtContext::Link()
{
    // Link this new JsrtContext up in the JsrtRuntime's context list
    this->next = runtime->contextList;
    this->previous = nullptr;

    if (runtime->contextList != nullptr)
    {
        Assert(runtime->contextList->previous == nullptr);
        runtime->contextList->previous = this;
    }

    runtime->contextList = this;
}

void JsrtContext::Unlink()
{
    // Unlink from JsrtRuntime JsrtContext list
    if (this->previous == nullptr)
    {
        // Have to check this because if we failed while creating, it might
        // never have gotten linked in to the runtime at all.
        if (this->runtime->contextList == this)
        {
            this->runtime->contextList = this->next;
        }
    }
    else
    {
        Assert(this->previous->next == this);
        this->previous->next = this->next;
    }

    if (this->next != nullptr)
    {
        Assert(this->next->previous == this);
        this->next->previous = this->previous;
    }
}

/* static */
JsrtContext * JsrtContext::GetCurrent()
{
    return s_tlvSlot;
}

/* static */
bool JsrtContext::TrySetCurrent(JsrtContext * context)
{
    ThreadContext * threadContext;

    //We are not pinning the context after SetCurrentContext, so if the context is not pinned
    //it might be reclaimed half way during execution. In jsrtshell the runtime was optimized out
    //at time of JsrtContext::Run by the compiler.
    //The change is to pin the context at setconcurrentcontext, and unpin the previous one. In
    //JsDisposeRuntime we'll reject if current context is active, so that will make sure all
    //contexts are unpinned at time of JsDisposeRuntime.
    if (context != nullptr)
    {
        threadContext = context->GetScriptContext()->GetThreadContext();

        if (!ThreadContextTLSEntry::TrySetThreadContext(threadContext))
        {
            return false;
        }
        // no need to rootAddRef and Release for the same context
        if (s_tlvSlot == context) return true;

        threadContext->GetRecycler()->RootAddRef((LPVOID)context);
    }
    else
    {
        if (!ThreadContextTLSEntry::ClearThreadContext(true))
        {
            return false;
        }
    }

    JsrtContext* originalContext = s_tlvSlot;
    if (originalContext != nullptr)
    {
        originalContext->GetScriptContext()->GetRecycler()->RootRelease((LPVOID) originalContext);
    }

    s_tlvSlot = context;
    return true;
}

void JsrtContext::Mark(Recycler * recycler)
{
    AssertMsg(false, "Mark called on object that isn't TrackableObject");
}
