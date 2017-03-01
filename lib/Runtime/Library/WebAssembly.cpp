//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
#include "../WasmReader/WasmReaderPch.h"
// Included for AsmJsDefaultEntryThunk
#include "Language/InterpreterStackFrame.h"
#include "Language/AsmJsUtils.h"
#include "Language/WebAssemblySource.h"

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
            JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource);
        }

        WebAssemblySource src(args[1], true, scriptContext);
        module = WebAssemblyModule::CreateModule(scriptContext, &src);
    }
    catch (JavascriptError & e)
    {
        return JavascriptPromise::CreateRejectedPromise(&e, scriptContext);
    }

    Assert(module);

    return JavascriptPromise::CreateResolvedPromise(module, scriptContext);
}

Var WebAssembly::EntryInstantiate(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    Var resultObject = nullptr;
    try
    {
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, WASMERR_InvalidInstantiateArgument);
        }

        Var importObject = scriptContext->GetLibrary()->GetUndefined();
        if (args.Info.Count >= 3)
        {
            importObject = args[2];
        }

        if (WebAssemblyModule::Is(args[1]))
        {
            resultObject = WebAssemblyInstance::CreateInstance(WebAssemblyModule::FromVar(args[1]), importObject);
        }
        else
        {
            WebAssemblySource src(args[1], true, scriptContext);
            WebAssemblyModule * module = WebAssemblyModule::CreateModule(scriptContext, &src);

            WebAssemblyInstance * instance = WebAssemblyInstance::CreateInstance(module, importObject);

            resultObject = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);

            JavascriptOperators::OP_SetProperty(resultObject, PropertyIds::module, module, scriptContext);

            JavascriptOperators::OP_SetProperty(resultObject, PropertyIds::instance, instance, scriptContext);
        }
    }
    catch (JavascriptError & e)
    {
        return JavascriptPromise::CreateRejectedPromise(&e, scriptContext);
    }

    Assert(resultObject != nullptr);

    return JavascriptPromise::CreateResolvedPromise(resultObject, scriptContext);
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
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource);
    }

    WebAssemblySource src(args[1], false, scriptContext);
    if (WebAssemblyModule::ValidateModule(scriptContext, &src))
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

void
WebAssembly::CheckSignature(ScriptContext * scriptContext, Wasm::WasmSignature * sig1, Wasm::WasmSignature * sig2)
{
    if (!sig1->IsEquivalent(sig2))
    {
        JavascriptError::ThrowWebAssemblyRuntimeError(scriptContext, WASMERR_SignatureMismatch);
    }
}

uint
WebAssembly::GetSignatureSize()
{
    return sizeof(Wasm::WasmSignature);
}

}
#endif // ENABLE_WASM
