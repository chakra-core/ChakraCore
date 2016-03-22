//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "jsrtHelper.h"
#include "Base/ThreadContextTlsEntry.h"

JsrtCallbackState::JsrtCallbackState(ThreadContext* currentThreadContext)
{
    if (currentThreadContext == nullptr)
    {
        originalThreadContext = ThreadContext::GetContextForCurrentThread();
    }
    else
    {
        originalThreadContext = currentThreadContext;
    }
    originalJsrtContext = JsrtContext::GetCurrent();
    Assert(originalJsrtContext == nullptr || originalThreadContext == originalJsrtContext->GetScriptContext()->GetThreadContext());
}

JsrtCallbackState::~JsrtCallbackState()
{
    if (originalJsrtContext != nullptr)
    {
        if (originalJsrtContext != JsrtContext::GetCurrent())
        {
            // This shouldn't fail as the context was previously set on the current thread.
            bool isSet = JsrtContext::TrySetCurrent(originalJsrtContext);
            if (!isSet)
            {
                Js::Throw::FatalInternalError();
            }
        }
    }
    else
    {
        bool isSet = ThreadContextTLSEntry::TrySetThreadContext(originalThreadContext);
        if (!isSet)
        {
            Js::Throw::FatalInternalError();
        }
    }
}

void JsrtCallbackState::ObjectBeforeCallectCallbackWrapper(JsObjectBeforeCollectCallback callback, void* object, void* callbackState, void* threadContext)
{
    JsrtCallbackState scope(reinterpret_cast<ThreadContext*>(threadContext));
    callback(object, callbackState);
}
