//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
#include "../WasmReader/WasmReaderPch.h"
// Included for AsmJsDefaultEntryThunk
#include "Language/InterpreterStackFrame.h"

namespace Js
{
Var WebAssembly::EntryCompile(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    WebAssemblyModule * module = nullptr;
    try
    {
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource, _u("WebAssembly.compile"));
        }

        const BOOL isTypedArray = Js::TypedArrayBase::Is(args[1]);
        const BOOL isArrayBuffer = Js::ArrayBuffer::Is(args[1]);

        if (!isTypedArray && !isArrayBuffer)
        {
            JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource, _u("WebAssembly.compile"));
        }

        BYTE* buffer;
        uint byteLength;
        if (isTypedArray)
        {
            Js::TypedArrayBase* array = Js::TypedArrayBase::FromVar(args[1]);
            buffer = array->GetByteBuffer();
            byteLength = array->GetByteLength();
        }
        else
        {
            Js::ArrayBuffer* arrayBuffer = Js::ArrayBuffer::FromVar(args[1]);
            buffer = arrayBuffer->GetBuffer();
            byteLength = arrayBuffer->GetByteLength();
        }

        module = WebAssemblyModule::CreateModule(scriptContext, buffer, byteLength);
    }
    catch (JavascriptError & e)
    {
        return JavascriptPromise::CreateRejectedPromise(&e, scriptContext);
    }

    Assert(module);

    return JavascriptPromise::CreateResolvedPromise(module, scriptContext);
}

Var WebAssembly::EntryValidate(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count < 2)
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource, _u("WebAssembly.validate"));
    }

    const BOOL isTypedArray = Js::TypedArrayBase::Is(args[1]);
    const BOOL isArrayBuffer = Js::ArrayBuffer::Is(args[1]);

    if (!isTypedArray && !isArrayBuffer)
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource, _u("WebAssembly.validate"));
    }

    BYTE* buffer;
    uint byteLength;
    if (isTypedArray)
    {
        Js::TypedArrayBase* array = Js::TypedArrayBase::FromVar(args[1]);
        buffer = array->GetByteBuffer();
        byteLength = array->GetByteLength();
    }
    else
    {
        Js::ArrayBuffer* arrayBuffer = Js::ArrayBuffer::FromVar(args[1]);
        buffer = arrayBuffer->GetBuffer();
        byteLength = arrayBuffer->GetByteLength();
    }

    if (WebAssemblyModule::ValidateModule(scriptContext, buffer, byteLength))
    {
        return scriptContext->GetLibrary()->GetTrue();
    }
    else
    {
        return scriptContext->GetLibrary()->GetFalse();
    }
}

uint32
WebAssembly::ToNonWrappingUint32(Var val, ScriptContext * ctx)
{
    double i = JavascriptConversion::ToInteger(val, ctx);
    if (i < 0 || i > (double)UINT32_MAX)
    {
        JavascriptError::ThrowRangeError(ctx, JSERR_ArgumentOutOfRange);
    }
    return (uint32)i;
}

}
#endif // ENABLE_WASM