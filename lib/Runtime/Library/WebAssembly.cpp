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

        BYTE* buffer;
        uint byteLength;
        WebAssembly::ReadBufferSource(args[1], scriptContext, &buffer, &byteLength);

        module = WebAssemblyModule::CreateModule(scriptContext, buffer, byteLength);
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
            BYTE* buffer;
            uint byteLength;
            WebAssembly::ReadBufferSource(args[1], scriptContext, &buffer, &byteLength);

            WebAssemblyModule * module = WebAssemblyModule::CreateModule(scriptContext, buffer, byteLength);

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

    BYTE* buffer;
    uint byteLength;
    WebAssembly::ReadBufferSource(args[1], scriptContext, &buffer, &byteLength);

    if (WebAssemblyModule::ValidateModule(scriptContext, buffer, byteLength))
    {
        return scriptContext->GetLibrary()->GetTrue();
    }
    else
    {
        return scriptContext->GetLibrary()->GetFalse();
    }
}

Var WebAssembly::EntryNativeTypeCallTest(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));
    if (args.Info.Count < 2 || !AsmJsScriptFunction::Is(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
    }
    AsmJsScriptFunction* asmFunc = (AsmJsScriptFunction*)args[1];
    AsmJsFunctionInfo* asmFuncInfo = asmFunc->GetFunctionBody()->GetAsmJsFunctionInfo();

    const uint32 argSize = asmFuncInfo->GetArgByteSize() + sizeof(Js::Var);
    char* argsBytes = HeapNewArrayZ(char, argSize);
    AutoArrayPtr<char> heapAutoClean(argsBytes, argSize);
    Unused(heapAutoClean);

    Var* argsVar = (Var*)argsBytes;
    CallInfo newInfo = callInfo;
    newInfo.Count--; // Remove the asm function
    JavascriptMethod asmJSEntryPoint = (JavascriptMethod)UnboxAsmJsArguments(asmFunc, args.Values + 2 /*skip [this, asmFunc]*/, argsBytes, newInfo, true);

    Var returnValue = scriptContext->GetLibrary()->GetUndefined();
    switch (asmFuncInfo->GetReturnType().which())
    {
    case AsmJsRetType::Void:
    case AsmJsRetType::Signed:
    {
        int intRetVal = JavascriptFunction::CallAsmJsFunction<int>(asmFunc, asmJSEntryPoint, asmFuncInfo->GetArgCount(), argsVar);
        if (asmFuncInfo->GetReturnType().which() == AsmJsRetType::Signed)
        {
            returnValue = JavascriptNumber::ToVar(intRetVal, scriptContext);
        }
        break;
    }
    case AsmJsRetType::Int64:
    {
        int64 val = JavascriptFunction::CallAsmJsFunction<int64>(asmFunc, asmJSEntryPoint, asmFuncInfo->GetArgCount(), argsVar);
        char16 buf[24];
        _i64tow_s(val, buf, 24, 10);
        returnValue = JavascriptString::NewCopySz(buf, scriptContext);
        break;
    }
    case AsmJsRetType::Double:
    {
        double val = JavascriptFunction::CallAsmJsFunction<double>(asmFunc, asmJSEntryPoint, asmFuncInfo->GetArgCount(), argsVar);
        returnValue = JavascriptNumber::NewWithCheck(val, scriptContext);
        break;
    }
    case AsmJsRetType::Float:
    {
        float val = JavascriptFunction::CallAsmJsFunction<float>(asmFunc, asmJSEntryPoint, asmFuncInfo->GetArgCount(), argsVar);
        returnValue = JavascriptNumber::NewWithCheck(val, scriptContext);
        break;
    }
    default:
        Assume(UNREACHED);
    }
    return returnValue;
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
WebAssembly::ReadBufferSource(Var val, ScriptContext * ctx, _Out_ BYTE** buffer, _Out_ uint *byteLength)
{
    const BOOL isTypedArray = Js::TypedArrayBase::Is(val);
    const BOOL isArrayBuffer = Js::ArrayBuffer::Is(val);

    if (!isTypedArray && !isArrayBuffer)
    {
        *buffer = nullptr;
        *byteLength = 0;
        JavascriptError::ThrowTypeError(ctx, WASMERR_NeedBufferSource);
    }

    if (isTypedArray)
    {
        Js::TypedArrayBase* array = Js::TypedArrayBase::FromVar(val);
        *buffer = array->GetByteBuffer();
        *byteLength = array->GetByteLength();
    }
    else
    {
        Js::ArrayBuffer* arrayBuffer = Js::ArrayBuffer::FromVar(val);
        *buffer = arrayBuffer->GetBuffer();
        *byteLength = arrayBuffer->GetByteLength();
    }
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