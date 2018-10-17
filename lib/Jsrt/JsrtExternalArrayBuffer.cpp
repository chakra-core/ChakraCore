//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "JsrtExternalArrayBuffer.h"

namespace Js
{
    JsrtExternalArrayBuffer::JsrtExternalArrayBuffer(byte *buffer, uint32 length, JsFinalizeCallback finalizeCallback, void *callbackState, DynamicType *type)
        : ExternalArrayBuffer(buffer, length, type), finalizeCallback(finalizeCallback), callbackState(callbackState)
    {
    }

    JsrtExternalArrayBuffer* JsrtExternalArrayBuffer::New(byte *buffer, uint32 length, JsFinalizeCallback finalizeCallback, void *callbackState, DynamicType *type)
    {
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        return RecyclerNewFinalized(recycler, JsrtExternalArrayBuffer, buffer, length, finalizeCallback, callbackState, type);
    }

    void JsrtExternalArrayBuffer::Finalize(bool isShutdown)
    {
        if (finalizeCallback != nullptr && !isDetached)
        {
            finalizeCallback(callbackState);
        }
    }

    ArrayBufferDetachedStateBase* JsrtExternalArrayBuffer::CreateDetachedState(BYTE* buffer, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength)
    {
        return HeapNew(JsrtExternalArrayBufferDetachedState, buffer, bufferLength, finalizeCallback, callbackState);
    };

    JsrtExternalArrayBuffer::JsrtExternalArrayBufferDetachedState::JsrtExternalArrayBufferDetachedState(
        BYTE* buffer, uint32 bufferLength, JsFinalizeCallback finalizeCallback, void *callbackState)
        : ExternalArrayBufferDetachedState(buffer, bufferLength), finalizeCallback(finalizeCallback), callbackState(callbackState)
    {}

    void JsrtExternalArrayBuffer::JsrtExternalArrayBufferDetachedState::ClearSelfOnly()
    {
        HeapDelete(this);
    }

    void JsrtExternalArrayBuffer::JsrtExternalArrayBufferDetachedState::DiscardState()
    {
        if (finalizeCallback != nullptr)
        {
            finalizeCallback(callbackState);
        }
        finalizeCallback = nullptr;
    }

    ArrayBuffer* JsrtExternalArrayBuffer::JsrtExternalArrayBufferDetachedState::Create(JavascriptLibrary* library)
    {
        ArrayBuffer* arr = JsrtExternalArrayBuffer::New(buffer, bufferLength, finalizeCallback, callbackState, library->GetArrayBufferType());
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(arr));
        return arr;
    }
}
