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
    const unsigned int WasmLibrary::experimentalVersion = Wasm::experimentalVersion;

    char16* WasmLibrary::lastWasmExceptionMessage = nullptr;

    Var WasmLibrary::instantiateModule(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray, _u("[Wasm].instantiateModule(typedArray,)"));
        }
        if (args.Info.Count < 3)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("[Wasm].instantiateModule(,ffi)"));
        }

        const BOOL isTypedArray = Js::TypedArrayBase::Is(args[1]);
        const BOOL isArrayBuffer = Js::ArrayBuffer::Is(args[1]);

        if (!isTypedArray && !isArrayBuffer)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray, _u("[Wasm].instantiateModule(typedArray,)"));
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

        if (!Js::JavascriptObject::Is(args[2]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("[Wasm].instantiateModule(,ffi)"));
        }
        Js::Var ffi = args[2];

        CompileScriptException se;
        Js::Var exportObject;
        Js::Var start = nullptr;
        Js::Utf8SourceInfo* utf8SourceInfo;
        BEGIN_LEAVE_SCRIPT_INTERNAL(scriptContext)
            exportObject = WasmLibrary::LoadWasmScript(
                scriptContext,
                (const char16*)buffer,
                args[1],
                nullptr, // source info
                &se,
                &utf8SourceInfo,
                byteLength,
                Js::Constants::GlobalCode,
                ffi,
                &start
            );
        END_LEAVE_SCRIPT_INTERNAL(scriptContext);

        HRESULT hr = se.ei.scode;
        if (FAILED(hr))
        {
            if (hr == E_OUTOFMEMORY || hr == VBSERR_OutOfMemory || hr == VBSERR_OutOfStack || hr == ERRnoMemory)
            {
                Js::Throw::OutOfMemory();
            }
            JavascriptError::ThrowParserError(scriptContext, hr, &se);
        }

        if (exportObject && start)
        {
            Js::ScriptFunction* f = Js::AsmJsScriptFunction::FromVar(start);
            Js::CallInfo info(Js::CallFlags_New, 1);
            Js::Arguments startArg(info, &start);
            Js::JavascriptFunction::CallFunction<true>(f, f->GetEntryPoint(), startArg);
        }
        return exportObject;
    }

    Var WasmLibrary::WasmLazyTrapCallback(RecyclableObject *callee, CallInfo, ...)
    {
        AsmJsScriptFunction* asmFunction = static_cast<AsmJsScriptFunction*>(callee);
        Assert(asmFunction);
        ScriptContext * scriptContext = asmFunction->GetScriptContext();
        Assert(scriptContext);
        JavascriptExceptionOperators::Throw(asmFunction->GetLazyError(), scriptContext);
    }

    void WasmLibrary::WasmLoadDataSegs(Wasm::WasmModule * wasmModule, Var* heap, ScriptContext* ctx)
    {
        if (wasmModule->GetMemory()->minSize != 0)
        {
            const uint64 maxSize = wasmModule->GetMemory()->maxSize;
            if (maxSize > ArrayBuffer::MaxArrayBufferLength)
            {
                Js::Throw::OutOfMemory();
            }
            // TODO: create new type array buffer that is non detachable
            *heap = JavascriptArrayBuffer::Create((uint32)maxSize, ctx->GetLibrary()->GetArrayBufferType());
            BYTE* buffer = ((JavascriptArrayBuffer*)*heap)->GetBuffer();
            for (uint32 iSeg = 0; iSeg < wasmModule->GetDataSegCount(); ++iSeg)
            {
                Wasm::WasmDataSegment* segment = wasmModule->GetDataSeg(iSeg);
                Assert(segment != nullptr);
                const uint32 offset = segment->getDestAddr();
                const uint32 size = segment->getSourceSize();
                if (offset > maxSize || UInt32Math::Add(offset, size) > maxSize)
                {
                    throw Wasm::WasmCompilationException(_u("Data segment #%u is out of bound"), iSeg);
                }

                if (size > 0)
                {
                    js_memcpy_s(buffer + offset, (uint32)maxSize - offset, segment->getData(), size);
                }
            }

        }
        else
        {
            *heap = nullptr;
        }
    }

    void WasmLibrary::WasmFunctionGenerateBytecode(AsmJsScriptFunction* func, bool propagateError)
    {
        FunctionBody* body = func->GetFunctionBody();
        AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();
        ScriptContext* scriptContext = func->GetScriptContext();

        Js::FunctionEntryPointInfo * entypointInfo = (Js::FunctionEntryPointInfo*)func->GetEntryPointInfo();
        Wasm::WasmReaderInfo* readerInfo = info->GetWasmReaderInfo();
        try
        {
            Wasm::WasmBytecodeGenerator::GenerateFunctionBytecode(scriptContext, readerInfo);
            info->SetWasmReaderInfo(nullptr);
            func->GetDynamicType()->SetEntryPoint(Js::AsmJsExternalEntryPoint);
            entypointInfo->jsMethod = AsmJsDefaultEntryThunk;
            // Do MTJRC/MAIC:0 check
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (CONFIG_FLAG(ForceNative) || CONFIG_FLAG(MaxAsmJsInterpreterRunCount) == 0)
            {
                GenerateFunction(scriptContext->GetNativeCodeGenerator(), func->GetFunctionBody(), func);
            }
#endif
        }
        catch (Wasm::WasmCompilationException& ex)
        {
            char16* originalMessage = ex.ReleaseErrorMessage();
            Wasm::WasmCompilationException newEx = Wasm::WasmCompilationException(_u("function %s: %s"), body->GetDisplayName(), originalMessage);
            SysFreeString(originalMessage);
            if (propagateError)
            {
                throw newEx;
            }
            info->SetWasmReaderInfo(nullptr);
            Js::JavascriptLibrary *library = scriptContext->GetLibrary();
            Js::JavascriptError *pError = library->CreateError();
            Js::JavascriptError::SetErrorMessage(pError, JSERR_WasmCompileError, newEx.ReleaseErrorMessage(), scriptContext);

            func->GetDynamicType()->SetEntryPoint(WasmLazyTrapCallback);
            entypointInfo->jsMethod = WasmLazyTrapCallback;
            func->SetLazyError(pError);
        }
    }

    void WasmLibrary::WasmLoadFunctions(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var* moduleMemoryPtr, Var* exportObj, Var* localModuleFunctions, bool* hasAnyLazyTraps)
    {
        FrameDisplay * frameDisplay = RecyclerNewPlus(ctx->GetRecycler(), sizeof(void*), FrameDisplay, 1);
        frameDisplay->SetItem(0, moduleMemoryPtr);

        for (uint i = 0; i < wasmModule->GetFunctionCount(); ++i)
        {
            AsmJsScriptFunction * funcObj = ctx->GetLibrary()->CreateAsmJsScriptFunction(wasmModule->GetFunctionInfo(i)->GetBody());
            funcObj->SetModuleMemory(moduleMemoryPtr);
            FunctionEntryPointInfo * entypointInfo = (FunctionEntryPointInfo*)funcObj->GetEntryPointInfo();
            entypointInfo->SetIsAsmJSFunction(true);
            entypointInfo->SetModuleAddress((uintptr_t)moduleMemoryPtr);
            funcObj->SetEnvironment(frameDisplay);
            localModuleFunctions[i] = funcObj;

            if (PHASE_ON(WasmDeferredPhase, funcObj->GetFunctionBody()))
            {
                funcObj->GetDynamicType()->SetEntryPoint(WasmLibrary::WasmDeferredParseExternalThunk);
                entypointInfo->jsMethod = WasmLibrary::WasmDeferredParseInternalThunk;
            }
            else
            {
                WasmFunctionGenerateBytecode(funcObj, !PHASE_ON(WasmLazyTrapPhase, funcObj->GetFunctionBody()));
            }
        }
    }

    Var WasmLibrary::WasmLoadExports(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var* localModuleFunctions)
    {
        Js::Var exportsNamespace = nullptr;

        // Check for Default export
        for (uint32 iExport = 0; iExport < wasmModule->GetExportCount(); ++iExport)
        {
            Wasm::WasmExport* funcExport = wasmModule->GetFunctionExport(iExport);
            if (funcExport && funcExport->nameLength == 0)
            {
                const uint32 funcIndex = funcExport->funcIndex;
                if (funcIndex < wasmModule->GetFunctionCount())
                {
                    exportsNamespace = localModuleFunctions[funcIndex];
                    break;
                }
            }
        }
        // If no default export is present, create an empty object
        if (exportsNamespace == nullptr)
        {
            exportsNamespace = JavascriptOperators::NewJavascriptObjectNoArg(ctx);
        }

        return exportsNamespace;
    }

    void WasmLibrary::WasmBuildObject(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var exportsNamespace, Var* heap, Var* exportObj, bool* hasAnyLazyTraps, Var* localModuleFunctions)
    {
        if (wasmModule->GetMemory()->minSize != 0 && wasmModule->GetMemory()->exported)
        {
            PropertyRecord const * propertyRecord = nullptr;
            ctx->GetOrAddPropertyRecord(_u("memory"), lstrlen(_u("memory")), &propertyRecord);
            JavascriptOperators::OP_SetProperty(exportsNamespace, propertyRecord->GetPropertyId(), *heap, ctx);
        }

        PropertyRecord const * exportsPropertyRecord = nullptr;
        ctx->GetOrAddPropertyRecord(_u("exports"), lstrlen(_u("exports")), &exportsPropertyRecord);
        JavascriptOperators::OP_SetProperty(*exportObj, exportsPropertyRecord->GetPropertyId(), exportsNamespace, ctx);
        if (*hasAnyLazyTraps)
        {
            PropertyRecord const * hasErrorsPropertyRecord = nullptr;
            ctx->GetOrAddPropertyRecord(_u("hasErrors"), lstrlen(_u("hasErrors")), &hasErrorsPropertyRecord);
            JavascriptOperators::OP_SetProperty(exportsNamespace, hasErrorsPropertyRecord->GetPropertyId(), JavascriptBoolean::OP_LdTrue(ctx), ctx);
        }

        for (uint32 iExport = 0; iExport < wasmModule->GetExportCount(); ++iExport)
        {
            Wasm::WasmExport* funcExport = wasmModule->GetFunctionExport(iExport);
            if (funcExport && funcExport->nameLength > 0)
            {
                PropertyRecord const * propertyRecord = nullptr;
                ctx->GetOrAddPropertyRecord(funcExport->name, funcExport->nameLength, &propertyRecord);
                Var funcObj;
                // todo:: This should not happen, we need to add validation that the `function_bodies` section is present
                if (funcExport->funcIndex < wasmModule->GetFunctionCount())
                {
                    funcObj = localModuleFunctions[funcExport->funcIndex];
                }
                else
                {
                    Assert(UNREACHED);
                    funcObj = ctx->GetLibrary()->GetUndefined();
                }
                JavascriptOperators::OP_SetProperty(exportsNamespace, propertyRecord->GetPropertyId(), funcObj, ctx);
            }
        }
    }

    void WasmLibrary::WasmLoadImports(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var* importFunctions, Var ffi)
    {
        const uint32 importCount = wasmModule->GetImportCount();
        if (importCount > 0 && (!ffi || !JavascriptObject::Is(ffi)))
        {
            throw Wasm::WasmCompilationException(_u("Import object is invalid"));
        }
        for (uint32 i = 0; i < importCount; ++i)
        {
            PropertyRecord const * modPropertyRecord = nullptr;
            PropertyRecord const * propertyRecord = nullptr;

            char16* modName = wasmModule->GetFunctionImport(i)->modName;
            uint32 modNameLen = wasmModule->GetFunctionImport(i)->modNameLen;
            ctx->GetOrAddPropertyRecord(modName, modNameLen, &modPropertyRecord);
            Var modProp = JavascriptOperators::OP_GetProperty(ffi, modPropertyRecord->GetPropertyId(), ctx);

            char16* name = wasmModule->GetFunctionImport(i)->fnName;
            uint32 nameLen = wasmModule->GetFunctionImport(i)->fnNameLen;
            Var prop = nullptr;
            if (nameLen > 0)
            {
                ctx->GetOrAddPropertyRecord(name, nameLen, &propertyRecord);

                if (!JavascriptObject::Is(modProp))
                {
                    throw Wasm::WasmCompilationException(_u("Import module %s is invalid"), modName);
                }
                prop = JavascriptOperators::OP_GetProperty(modProp, propertyRecord->GetPropertyId(), ctx);
            }
            else
            {
                // Use only first level if name is missing
                prop = modProp;
            }
            if (!JavascriptFunction::Is(prop))
            {
                throw Wasm::WasmCompilationException(_u("Import function %s.%s is invalid"), modName, name);
            }
            importFunctions[i] = prop;
        }
    }

    void WasmLibrary::WasmLoadIndirectFunctionTables(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var** indirectFunctionTables, Var* localModuleFunctions)
    {
        for (uint i = 0; i < wasmModule->GetIndirectFunctionCount(); ++i)
        {
            uint funcIndex = wasmModule->GetIndirectFunctionIndex(i);
            if (funcIndex >= wasmModule->GetFunctionCount())
            {
                throw Wasm::WasmCompilationException(_u("Invalid function index %U for indirect function table"), funcIndex);
            }
            Wasm::WasmFunctionInfo * indirFunc = wasmModule->GetFunctionInfo(funcIndex);
            uint sigId = indirFunc->GetSignature()->GetSignatureId();
            if (!indirectFunctionTables[sigId])
            {
                // TODO: initialize all indexes to "Js::Throw::RuntimeError" or similar type thing
                // now, indirect func call to invalid type will give nullptr deref
                indirectFunctionTables[sigId] = RecyclerNewArrayZ(ctx->GetRecycler(), Js::Var, wasmModule->GetIndirectFunctionCount());
            }
            indirectFunctionTables[sigId][i] = localModuleFunctions[funcIndex];
        }
    }

    Var WasmLibrary::LoadWasmScript(
        ScriptContext* scriptContext,
        const char16* buffer,
        Js::Var bufferSrc,
        SRCINFO const * pSrcInfo,
        CompileScriptException * pse,
        Utf8SourceInfo** ppSourceInfo,
        const uint lengthBytes,
        const char16 *rootDisplayName,
        Js::Var ffi,
        Js::Var* start)
    {
        if (pSrcInfo == nullptr)
        {
            pSrcInfo = scriptContext->cache->noContextGlobalSourceInfo;
        }

        AutoProfilingPhase wasmPhase(scriptContext, Js::WasmPhase);
        Unused(wasmPhase);

        Assert(!scriptContext->GetThreadContext()->IsScriptActive());
        Assert(pse != nullptr);
        Wasm::WasmModuleGenerator *bytecodeGen = nullptr;
        Js::Var exportObj = nullptr;
        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
            Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
            *ppSourceInfo = nullptr;
            Wasm::WasmModule * wasmModule = nullptr;

            *ppSourceInfo = Utf8SourceInfo::New(scriptContext, (LPCUTF8)buffer, lengthBytes / sizeof(char16), lengthBytes, pSrcInfo, false);
            bytecodeGen = HeapNew(Wasm::WasmModuleGenerator, scriptContext, *ppSourceInfo, (byte*)buffer, lengthBytes, bufferSrc);
            wasmModule = bytecodeGen->GenerateModule();

            Var* moduleEnvironmentPtr = RecyclerNewArrayZ(scriptContext->GetRecycler(), Var, wasmModule->GetModuleEnvironmentSize());
            Var* heap = moduleEnvironmentPtr + wasmModule->GetHeapOffset();
            exportObj = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
            Var* localModuleFunctions = moduleEnvironmentPtr + wasmModule->GetFuncOffset();

            WasmLoadDataSegs(wasmModule, heap, scriptContext);

            bool hasAnyLazyTraps = false;
            WasmLoadFunctions(wasmModule, scriptContext, moduleEnvironmentPtr, &exportObj, localModuleFunctions, &hasAnyLazyTraps);
            Js::Var exportsNamespace = WasmLoadExports(wasmModule, scriptContext, localModuleFunctions);
            WasmBuildObject(wasmModule, scriptContext, exportsNamespace, heap, &exportObj, &hasAnyLazyTraps, localModuleFunctions);

            Var* importFunctions = moduleEnvironmentPtr + wasmModule->GetImportFuncOffset();
            WasmLoadImports(wasmModule, scriptContext, importFunctions, ffi);

            Var** indirectFunctionTables = (Var**)(moduleEnvironmentPtr + wasmModule->GetIndirFuncTableOffset());
            WasmLoadIndirectFunctionTables(wasmModule, scriptContext, indirectFunctionTables, localModuleFunctions);
            uint32 startFuncIdx = wasmModule->GetStartFunction();
            if (start)
            {
                if (startFuncIdx != Js::Constants::UninitializedValue)
                {
                    *start = localModuleFunctions[startFuncIdx];
                }
                else
                {
                    *start = nullptr;
                }
            }
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
        return exportObj;
    }

#if _M_IX86
    __declspec(naked)
        Var WasmLibrary::WasmDeferredParseExternalThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        // Register functions
        __asm
        {
            push ebp
            mov ebp, esp
            lea eax, [esp + 8]
            push 0
            push eax
            call WasmLibrary::WasmDeferredParseEntryPoint
#ifdef _CONTROL_FLOW_GUARD
            // verify that the call target is valid
            mov  ecx, eax
            call[__guard_check_icall_fptr]
            mov eax, ecx
#endif
            pop ebp
            // Although we don't restore ESP here on WinCE, this is fine because script profiler is not shipped for WinCE.
            jmp eax
        }
    }

    __declspec(naked)
        Var WasmLibrary::WasmDeferredParseInternalThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        // Register functions
        __asm
        {
            push ebp
            mov ebp, esp
            lea eax, [esp + 8]
            push 1
            push eax
            call WasmLibrary::WasmDeferredParseEntryPoint
#ifdef _CONTROL_FLOW_GUARD
            // verify that the call target is valid
            mov  ecx, eax
            call[__guard_check_icall_fptr]
            mov eax, ecx
#endif
            pop ebp
            // Although we don't restore ESP here on WinCE, this is fine because script profiler is not shipped for WinCE.
            jmp eax
        }
    }
#elif defined(_M_X64)
    // Do nothing: the implementation of WasmLibrary::WasmDeferredParseExternalThunk is declared (appropriately decorated) in
    // Language\amd64\amd64_Thunks.asm.
#endif // _M_IX86

}

#endif // ENABLE_WASM

Js::JavascriptMethod Js::WasmLibrary::WasmDeferredParseEntryPoint(Js::AsmJsScriptFunction** funcPtr, int internalCall)
{
#ifdef ENABLE_WASM
    AsmJsScriptFunction* func = *funcPtr;

    WasmLibrary::WasmFunctionGenerateBytecode(func, false);
    Js::FunctionEntryPointInfo * entypointInfo = (Js::FunctionEntryPointInfo*)func->GetEntryPointInfo();
    if (internalCall)
    {
        return entypointInfo->jsMethod;
    }
    return func->GetDynamicType()->GetEntryPoint();
#else
    Js::Throw::InternalError();
#endif
}