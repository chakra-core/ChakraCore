//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"
#include "WebAssemblySource.h"

#ifdef ENABLE_WASM

namespace Js
{

WebAssemblySource::WebAssemblySource(Var source, bool createNewContext, ScriptContext * scriptContext) :
    buffer(nullptr), bufferLength(0), sourceInfo(nullptr)
{
    ReadBufferSource(source, scriptContext);
    CreateSourceInfo(createNewContext, scriptContext);
}

WebAssemblySource::WebAssemblySource(byte* source, uint bufferLength, bool createNewContext, ScriptContext* scriptContext):
    buffer(source), bufferLength(bufferLength)
{
    CreateSourceInfo(createNewContext, scriptContext);
}

void WebAssemblySource::ReadBufferSource(Var val, ScriptContext * scriptContext)
{
    BYTE* srcBuffer;
    if (Js::TypedArrayBase::Is(val))
    {
        Js::TypedArrayBase* array = Js::TypedArrayBase::FromVar(val);
        srcBuffer = array->GetByteBuffer();
        bufferLength = array->GetByteLength();
    }
    else if (Js::ArrayBuffer::Is(val))
    {
        Js::ArrayBuffer* arrayBuffer = Js::ArrayBuffer::FromVar(val);
        srcBuffer = arrayBuffer->GetBuffer();
        bufferLength = arrayBuffer->GetByteLength();
    }
    else
    {
        // The buffer was not a TypedArray nor an ArrayBuffer
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource);
    }
    Assert(srcBuffer || bufferLength == 0);
    if (bufferLength > 0)
    {
        // copy buffer so external changes to it don't cause issues when defer parsing
        buffer = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), byte, bufferLength);
        js_memcpy_s(buffer, bufferLength, srcBuffer, bufferLength);
    }
}

void WebAssemblySource::CreateSourceInfo(bool createNewContext, ScriptContext* scriptContext)
{
    SRCINFO si = {
        /* sourceContextInfo   */ nullptr,
        /* dlnHost             */ 0,
        /* ulColumnHost        */ 0,
        /* lnMinHost           */ 0,
        /* ichMinHost          */ 0,
        /* ichLimHost          */ 0,
        /* ulCharOffset        */ 0,
        /* mod                 */ 0,
        /* grfsi               */ 0
    };
    SRCINFO const * srcInfo = scriptContext->Cache()->noContextGlobalSourceInfo;
    if (createNewContext)
    {
        si.sourceContextInfo = scriptContext->CreateSourceContextInfo(scriptContext->GetNextSourceContextId(), nullptr, 0, nullptr);
        srcInfo = &si;
    }

    // Note: We don't have real "source info" for Wasm. Following are just placeholders.
    // Hack: Wasm handles debugging differently. Fake this as "LibraryCode" so that
    // normal script debugging code ignores this source info and its functions.
    const int32 cchLength = static_cast<int32>(bufferLength / sizeof(char16));
    sourceInfo = Utf8SourceInfo::NewWithNoCopy(
        scriptContext, (LPCUTF8)buffer, cchLength, bufferLength, srcInfo, /*isLibraryCode*/true);
    scriptContext->SaveSourceNoCopy(sourceInfo, cchLength, /*isCesu8*/false);
}

}
#endif