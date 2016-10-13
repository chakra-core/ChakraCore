//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM

#include "../WasmReader/WasmReaderPch.h"

namespace Js
{
char16* WebAssemblyModule::lastWasmExceptionMessage = nullptr;

WebAssemblyModule::WebAssemblyModule(Wasm::WasmModule * wasmModule, DynamicType * type) :
    DynamicObject(type),
    module(wasmModule)
{
}

/* static */
bool
WebAssemblyModule::Is(Var value)
{
    return JavascriptOperators::GetTypeId(value) == TypeIds_WebAssemblyMemory;
}

/* static */
WebAssemblyModule *
WebAssemblyModule::FromVar(Var value)
{
    Assert(WebAssemblyModule::Is(value));
    return static_cast<WebAssemblyModule*>(value);
}

/* static */
Var
WebAssemblyModule::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

    Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
    bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
    Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr);

    if (!(callInfo.Flags & CallFlags_New) || (newTarget && JavascriptOperators::IsUndefinedObject(newTarget)))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("WebAssembly.Module"));
    }

    if (args.Info.Count < 2)
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource, _u("WebAssembly.Module"));
    }

    const BOOL isTypedArray = Js::TypedArrayBase::Is(args[1]);
    const BOOL isArrayBuffer = Js::ArrayBuffer::Is(args[1]);

    if (!isTypedArray && !isArrayBuffer)
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource, _u("WebAssembly.Module"));
    }

    BYTE* buffer;
    uint byteLength;
    Var bufferSrc = args[1];
    if (isTypedArray)
    {
        Js::TypedArrayBase* array = Js::TypedArrayBase::FromVar(bufferSrc);
        buffer = array->GetByteBuffer();
        byteLength = array->GetByteLength();
    }
    else
    {
        Js::ArrayBuffer* arrayBuffer = Js::ArrayBuffer::FromVar(bufferSrc);
        buffer = arrayBuffer->GetBuffer();
        byteLength = arrayBuffer->GetByteLength();
    }

    CompileScriptException se;
    Js::Utf8SourceInfo* utf8SourceInfo;
    Wasm::WasmModule * wasmModule = CompileModule(scriptContext, (const char16*)buffer,
        nullptr, // source info
        &se,
        &utf8SourceInfo,
        byteLength, false, bufferSrc);

    return RecyclerNew(scriptContext->GetRecycler(), WebAssemblyModule, wasmModule, scriptContext->GetLibrary()->GetWebAssemblyModuleType());
}

/* static */
Wasm::WasmModule *
WebAssemblyModule::CompileModule(
    ScriptContext* scriptContext,
    const char16* script,
    SRCINFO const * pSrcInfo,
    CompileScriptException * pse,
    Utf8SourceInfo** ppSourceInfo,
    const uint lengthBytes,
    bool validateOnly,
    Var bufferSrc)
{
    Wasm::WasmModule * wasmModule = nullptr;
    if (pSrcInfo == nullptr)
    {
        pSrcInfo = scriptContext->cache->noContextGlobalSourceInfo;
    }

    AutoProfilingPhase wasmPhase(scriptContext, Js::WasmPhase);
    Unused(wasmPhase);

    BEGIN_LEAVE_SCRIPT_INTERNAL(scriptContext)
    {
        Assert(pse != nullptr);
        Wasm::WasmModuleGenerator *bytecodeGen = nullptr;
        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
            Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
            *ppSourceInfo = nullptr;

            *ppSourceInfo = Utf8SourceInfo::New(scriptContext, (LPCUTF8)script, lengthBytes / sizeof(char16), lengthBytes, pSrcInfo, false);
            bytecodeGen = HeapNew(Wasm::WasmModuleGenerator, scriptContext, *ppSourceInfo, (byte*)script, lengthBytes, bufferSrc);
            wasmModule = bytecodeGen->GenerateModule();
        }
        catch (Js::OutOfMemoryException)
        {
            pse->ProcessError(nullptr, E_OUTOFMEMORY, nullptr);
        }
        catch (Js::StackOverflowException)
        {
            pse->ProcessError(nullptr, VBSERR_OutOfStack, nullptr);
        }
        catch (Wasm::WasmCompilationException& ex)
        {
            lastWasmExceptionMessage = ex.ReleaseErrorMessage();

            pse->ProcessError(nullptr, JSERR_WasmCompileError, nullptr);
            pse->ei.pfnDeferredFillIn = [](tagEXCEPINFO *pei) -> HRESULT {
                pei->bstrDescription = lastWasmExceptionMessage;
                lastWasmExceptionMessage = nullptr;
                return S_OK;
            };
        }
        if (bytecodeGen)
        {
            HeapDelete(bytecodeGen);
        }
    }
    END_LEAVE_SCRIPT_INTERNAL(scriptContext);

    return wasmModule;
}

} // namespace Js

#endif