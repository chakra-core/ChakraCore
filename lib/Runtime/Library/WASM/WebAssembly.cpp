//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
#include "../WasmReader/WasmReaderPch.h"
#include "Language/WebAssemblySource.h"

using namespace Js;

Var WebAssembly::EntryCompile(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));
    try
    {
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource);
        }

        WebAssemblySource src(args[1], true, scriptContext);
        WebAssemblyModule* wasmModule = WebAssemblyModule::CreateModule(scriptContext, &src);
        return JavascriptPromise::CreateResolvedPromise(wasmModule, scriptContext);
    }
    catch (JavascriptException & e)
    {
        return JavascriptPromise::CreateRejectedPromise(e.GetAndClear()->GetThrownObject(scriptContext), scriptContext);
    }
}

Var WebAssembly::EntryCompileStreaming(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();
    JavascriptLibrary* library = scriptContext->GetLibrary();

    Assert(!(callInfo.Flags & CallFlags_New));
    try
    {
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedResponse);
        }

        // Check to see if it was a response object
        Var responsePromise = TryResolveResponse(function, args[0], args[1]);
        if (responsePromise)
        {
            // Once we've resolved everything, create the module
            return JavascriptPromise::CreateThenPromise((JavascriptPromise*)responsePromise, library->GetWebAssemblyCompileFunction(), library->GetThrowerFunction(), scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedResponse);
    }
    catch (JavascriptException & e)
    {
        return JavascriptPromise::CreateRejectedPromise(e.GetAndClear()->GetThrownObject(scriptContext), scriptContext);
    }
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

        if (VarIs<WebAssemblyModule>(args[1]))
        {
            resultObject = WebAssemblyInstance::CreateInstance(VarTo<WebAssemblyModule>(args[1]), importObject);
        }
        else
        {
            WebAssemblySource src(args[1], true, scriptContext);
            WebAssemblyModule* wasmModule = WebAssemblyModule::CreateModule(scriptContext, &src);

            WebAssemblyInstance* instance = WebAssemblyInstance::CreateInstance(wasmModule, importObject);

            resultObject = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);

            JavascriptOperators::OP_SetProperty(resultObject, PropertyIds::module, wasmModule, scriptContext);
            JavascriptOperators::OP_SetProperty(resultObject, PropertyIds::instance, instance, scriptContext);
        }
        return JavascriptPromise::CreateResolvedPromise(resultObject, scriptContext);
    }
    catch (JavascriptException & e)
    {
        return JavascriptPromise::CreateRejectedPromise(e.GetAndClear()->GetThrownObject(scriptContext), scriptContext);
    }
}

Var WebAssembly::EntryInstantiateStreaming(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();
    JavascriptLibrary* library = scriptContext->GetLibrary();

    Assert(!(callInfo.Flags & CallFlags_New));

    try
    {
        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedResponse);
        }

        // Check to see if it was a response object
        Var responsePromise = TryResolveResponse(function, args[0], args[1]);
        if (responsePromise)
        {
            Var importObject = scriptContext->GetLibrary()->GetUndefined();
            if (args.Info.Count >= 3)
            {
                importObject = args[2];
            }
            // Since instantiate takes extra arguments, we have to create a bound function to carry the importsObject until the response is resolved
            // Because function::bind() binds arguments from the left first, we have to calback a different function to reverse the order of the arguments
            Var boundArgs[] = { library->GetWebAssemblyInstantiateBoundFunction(), args[0], importObject };
            CallInfo boundCallInfo(CallFlags_Value, 3);
            ArgumentReader myargs(&boundCallInfo, boundArgs);
            RecyclableObject* boundFunction = BoundFunction::New(scriptContext, myargs);
            return JavascriptPromise::CreateThenPromise((JavascriptPromise*)responsePromise, boundFunction, library->GetThrowerFunction(), scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedResponse);
    }
    catch (JavascriptException & e)
    {
        return JavascriptPromise::CreateRejectedPromise(e.GetAndClear()->GetThrownObject(scriptContext), scriptContext);
    }
}

Var WebAssembly::EntryInstantiateBound(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    Assert(!(callInfo.Flags & CallFlags_New));

    Var thisVar = args[0];
    Var importObj = callInfo.Count > 1 ? args[1] : function->GetScriptContext()->GetLibrary()->GetUndefined();
    Var bufferSrc = callInfo.Count > 2 ? args[2] : function->GetScriptContext()->GetLibrary()->GetUndefined();

    return CALL_ENTRYPOINT_NOASSERT(EntryInstantiate, function, CallInfo(CallFlags_Value, 3), thisVar, bufferSrc, importObj);
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

Var WebAssembly::EntryQueryResponse(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    // Make sure this is a Response object
    if (args.Info.Count < 2 || !IsResponseObject(args[1], scriptContext))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedResponse);
    }
    Var responseObject = args[1];

    // Get the arrayBuffer method from the object
    PropertyString* propStr = scriptContext->GetPropertyString(PropertyIds::arrayBuffer);
    Var arrayBufferProp = JavascriptOperators::OP_GetElementI(responseObject, propStr, scriptContext);

    // Call res.arrayBuffer()
    if (!JavascriptConversion::IsCallable(arrayBufferProp))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedResponse);
    }

    RecyclableObject* arrayBufferFunc = VarTo<RecyclableObject>(arrayBufferProp);
    Var arrayBufferRes = nullptr;
    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
    {
        arrayBufferRes = CALL_FUNCTION(scriptContext->GetThreadContext(), arrayBufferFunc, Js::CallInfo(CallFlags_Value, 1), responseObject);
    }
    END_SAFE_REENTRANT_CALL

    // Make sure res.arrayBuffer() is a Promise
    if (!VarIs<JavascriptPromise>(arrayBufferRes))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedResponse);
    }
    return arrayBufferRes;
}

bool WebAssembly::IsResponseObject(Var responseObject, ScriptContext* scriptContext)
{
    if (!VarIs<RecyclableObject>(responseObject))
    {
        return false;
    }
    TypeId typeId = VarTo<RecyclableObject>(responseObject)->GetTypeId();
    if (!CONFIG_FLAG(WasmIgnoreResponse))
    {
        return scriptContext->IsWellKnownHostType<WellKnownHostType_Response>(typeId) && typeId != TypeIds_Undefined;
    }
    // Consider all object as Response objects under -wasmIgnoreResponse
    return typeId == TypeIds_Object;
}

Var WebAssembly::TryResolveResponse(RecyclableObject* function, Var thisArg, Var responseArg)
{
    ScriptContext* scriptContext = function->GetScriptContext();
    JavascriptLibrary* library = scriptContext->GetLibrary();
    Var responsePromise = nullptr;
    bool isResponse = IsResponseObject(responseArg, scriptContext);
    if (isResponse)
    {
        CallInfo newCallInfo;
        newCallInfo.Count = 2;
        // We already have a response object, query it now
        responsePromise = CALL_ENTRYPOINT_NOASSERT(EntryQueryResponse, function, Js::CallInfo(CallFlags_Value, 2), thisArg, responseArg);
    }
    else if (VarIs<JavascriptPromise>(responseArg))
    {
        JavascriptPromise* promise = (JavascriptPromise*)responseArg;
        // Wait until this promise resolves and then try to query the response object (if it's a response object)
        responsePromise = JavascriptPromise::CreateThenPromise(promise, library->GetWebAssemblyQueryResponseFunction(), library->GetThrowerFunction(), scriptContext);
    }
    if (responsePromise && !VarIs<JavascriptPromise>(responsePromise))
    {
        AssertMsg(UNREACHED, "How did we end up with something other than a promise here ?");
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedResponse);
    }
    return responsePromise;
}

uint32
WebAssembly::ToNonWrappingUint32(Var val, ScriptContext * ctx)
{
    double i = JavascriptConversion::ToNumber(val, ctx);
    if (
        JavascriptNumber::IsNan(i) ||
        JavascriptNumber::IsPosInf(i) ||
        JavascriptNumber::IsNegInf(i) ||
        i < 0 ||
        i > (double)UINT32_MAX
    )
    {
        JavascriptError::ThrowTypeError(ctx, JSERR_NeedNumber);
    }
    return (uint32)JavascriptConversion::ToInteger(i);
}

void
WebAssembly::CheckSignature(ScriptContext * scriptContext, Wasm::WasmSignature * sig1, Wasm::WasmSignature * sig2)
{
    JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_CheckWasmSignature);
    if (!sig1->IsEquivalent(sig2))
    {
        JavascriptError::ThrowWebAssemblyRuntimeError(scriptContext, WASMERR_SignatureMismatch);
    }
    JIT_HELPER_END(Op_CheckWasmSignature);
}

uint
WebAssembly::GetSignatureSize()
{
    return sizeof(Wasm::WasmSignature);
}

#endif // ENABLE_WASM
