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
                const uint32 offset = segment->getDestAddr(wasmModule);
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
            const bool noJit = PHASE_OFF(BackEndPhase, body) || PHASE_OFF(FullJitPhase, body) || scriptContext->GetConfig()->IsNoNative();
            if (!noJit && (CONFIG_FLAG(ForceNative) || CONFIG_FLAG(MaxAsmJsInterpreterRunCount) == 0))
            {
                GenerateFunction(scriptContext->GetNativeCodeGenerator(), body, func);
            }
#endif
        }
        catch (Wasm::WasmCompilationException& ex)
        {
            char16* originalMessage = ex.ReleaseErrorMessage();
            intptr_t offset = readerInfo->m_module->GetReader()->GetCurrentOffset();
            intptr_t start = readerInfo->m_funcInfo->m_readerInfo.startOffset;
            uint32 size = readerInfo->m_funcInfo->m_readerInfo.size;

            Wasm::WasmCompilationException newEx = Wasm::WasmCompilationException(
                _u("function %s at offset %d/%d: %s"),
                body->GetDisplayName(),
                offset - start,
                size,
                originalMessage
            );
            SysFreeString(originalMessage);
            if (propagateError)
            {
                throw newEx;
            }
            info->SetWasmReaderInfo(nullptr);
            char16* msg = newEx.ReleaseErrorMessage();
            //Output::Print(_u("%s\n"), msg);
            Js::JavascriptLibrary *library = scriptContext->GetLibrary();
            Js::JavascriptError *pError = library->CreateError();
            Js::JavascriptError::SetErrorMessage(pError, JSERR_WasmCompileError, msg, scriptContext);

            func->GetDynamicType()->SetEntryPoint(WasmLazyTrapCallback);
            entypointInfo->jsMethod = WasmLazyTrapCallback;
            func->SetLazyError(pError);
        }
    }

    void WasmLibrary::WasmLoadFunctions(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var* moduleMemoryPtr, Var* exportObj, Var* localModuleFunctions, bool* hasAnyLazyTraps)
    {
        FrameDisplay * frameDisplay = RecyclerNewPlus(ctx->GetRecycler(), sizeof(void*), FrameDisplay, 1);
        frameDisplay->SetItem(0, moduleMemoryPtr);

        for (uint i = 0; i < wasmModule->GetWasmFunctionCount(); ++i)
        {
            AsmJsScriptFunction * funcObj = ctx->GetLibrary()->CreateAsmJsScriptFunction(wasmModule->GetWasmFunctionInfo(i)->GetBody());
            funcObj->SetModuleMemory(moduleMemoryPtr);
            FunctionEntryPointInfo * entypointInfo = (FunctionEntryPointInfo*)funcObj->GetEntryPointInfo();
            entypointInfo->SetIsAsmJSFunction(true);
            entypointInfo->SetModuleAddress((uintptr_t)moduleMemoryPtr);
            funcObj->SetEnvironment(frameDisplay);
            localModuleFunctions[i] = funcObj;

            if (!PHASE_OFF(WasmDeferredPhase, funcObj->GetFunctionBody()))
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

    void WasmLibrary::WasmBuildObject(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var exportsNamespace, Var* heap, Var* exportObj, bool* hasAnyLazyTraps, Var* localModuleFunctions, Var* importFunctions)
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

                Var obj = ctx->GetLibrary()->GetUndefined();
                switch (funcExport->kind)
                {
                case Wasm::ExternalKinds::Function:

                    obj = GetFunctionObjFromFunctionIndex(wasmModule, ctx, funcExport->funcIndex, localModuleFunctions, importFunctions);
                    break;
                case Wasm::ExternalKinds::Global:
                    Wasm::WasmGlobal* global = wasmModule->globals.Item(funcExport->funcIndex);
                    Assert(global->GetReferenceType() == Wasm::WasmGlobal::Const); //every global has to be resolved by this point

                    if (global->GetMutability())
                    {
                        throw Wasm::WasmCompilationException(_u("global %d is mutable. Exporting mutable globals isn't supported"), iExport);
                    }

                    switch (global->GetType())
                    {
                    case Wasm::WasmTypes::I32:
                        obj = JavascriptNumber::ToVar(global->cnst.i32, ctx);
                        break;
                    case Wasm::WasmTypes::F32:
                        obj = JavascriptNumber::New((double)global->cnst.f32, ctx);
                        break;
                    case Wasm::WasmTypes::F64:
                        obj = JavascriptNumber::New(global->cnst.f64, ctx);
                        break;
                    case Wasm::WasmTypes::I64:
                    default:
                        Assert(UNREACHED);
                        break;
                    }
                }
                JavascriptOperators::OP_SetProperty(exportsNamespace, propertyRecord->GetPropertyId(), obj, ctx);
            }
        }
    }

    static Var GetImportVariable(Wasm::WasmImport* wi, ScriptContext* ctx, Var ffi)
    {
        PropertyRecord const * modPropertyRecord = nullptr;
        char16* modName = wi->modName;
        uint32 modNameLen = wi->modNameLen;
        ctx->GetOrAddPropertyRecord(modName, modNameLen, &modPropertyRecord);
        Var modProp = JavascriptOperators::OP_GetProperty(ffi, modPropertyRecord->GetPropertyId(), ctx);



        char16* name = wi->fnName;
        uint32 nameLen = wi->fnNameLen;
        Var prop = nullptr;
        if (nameLen > 0)
        {
            PropertyRecord const * propertyRecord = nullptr;
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

        return prop;
    }

    template <typename T>
    void static SetGlobalValue(Var moduleEnv, uint offset, T val)
    {
        T* slot = (T*)moduleEnv + offset;
        *slot = val;
    }

    void WasmLibrary::WasmLoadImports(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var* importFunctions, Var moduleEnv, Var ffi)
    {
        const uint32 importCount = wasmModule->GetImportCount();
        if (importCount > 0 && (!ffi || !JavascriptObject::Is(ffi)))
        {
            throw Wasm::WasmCompilationException(_u("Import object is invalid"));
        }
        for (uint32 i = 0; i < importCount; ++i)
        {
            Var prop = GetImportVariable(wasmModule->GetFunctionImport(i), ctx, ffi);
            if (!JavascriptFunction::Is(prop))
            {
                throw Wasm::WasmCompilationException(_u("Import function %s.%s is invalid"), wasmModule->GetFunctionImport(i)->modName, wasmModule->GetFunctionImport(i)->fnName);
            }
            importFunctions[i] = prop;
        }
    }

    void WasmLibrary::WasmLoadGlobals(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var moduleEnv, Var ffi)
    {
        uint i = 0;
        uint count = (uint) wasmModule->globals.Count();
        while (i < count && wasmModule->globals.Item(i)->GetReferenceType() == Wasm::WasmGlobal::ImportedReference)
        {
            Wasm::WasmGlobal* global = wasmModule->globals.Item(i);
            Var prop = GetImportVariable(global->importVar, ctx, ffi);

            uint offset = wasmModule->GetOffsetForGlobal(global);
            global->SetReferenceType(Wasm::WasmGlobal::Const);

            if (!JavascriptNumber::Is(prop) && !TaggedInt::Is(prop))
            {
                throw Wasm::WasmCompilationException(_u("Import global %s.%s (%d) isn't a valid javascript number"), global->importVar->modName, global->importVar->fnName, i);
            }

            switch (global->GetType())
            {
            case Wasm::WasmTypes::I32:
            {

                int val = JavascriptConversion::ToInt32(prop, ctx);
                global->cnst.i32 = val; //resolve global to const
                SetGlobalValue(moduleEnv, offset, val);
                break;
            }
            case Wasm::WasmTypes::F32:
            {
                float val = (float) JavascriptConversion::ToNumber(prop, ctx);
                global->cnst.f32 = val;
                SetGlobalValue(moduleEnv, offset, val);
            }
            case Wasm::WasmTypes::F64:
            {
                double val = JavascriptConversion::ToNumber(prop, ctx);
                global->cnst.f64 = val;
                SetGlobalValue(moduleEnv, offset, val);
                break;
            }
            case Wasm::WasmTypes::I64:
                Js::JavascriptError::ThrowTypeError(ctx, VBSERR_TypeMismatch);
            default:
                Assert(UNREACHED);
                break;

            }
            i++;
        }


        for (; i < count; i++)
        {
            Wasm::WasmGlobal* global = wasmModule->globals.Item(i);

            uint offset = wasmModule->GetOffsetForGlobal(global);
            Wasm::WasmGlobal* sourceGlobal = nullptr;
            if (global->GetReferenceType() == Wasm::WasmGlobal::Const)
            {
                sourceGlobal = global;
            }
            else
            {
                sourceGlobal = wasmModule->globals.Item(global->var.num);
                Assert(sourceGlobal->GetReferenceType() != Wasm::WasmGlobal::ImportedReference); //no imported globals at this point
                if (sourceGlobal->GetReferenceType() != Wasm::WasmGlobal::Const)
                {
                    throw Wasm::WasmCompilationException(_u("Global %d is initialized with global %d ")
                        _u("which is not a const. Forward references aren't supported!"), i, global->var.num);
                }
                global->SetReferenceType(Wasm::WasmGlobal::Const); //resolve global to const
                global->cnst = sourceGlobal->cnst;

                Assert(sourceGlobal->GetReferenceType() == Wasm::WasmGlobal::Const);
                if (sourceGlobal->GetType() != global->GetType())
                {
                    throw Wasm::WasmCompilationException(_u("Type mismatch between %d and %d"), i, global->var.num);
                }
            }

            switch (sourceGlobal->GetType())
            {
            case Wasm::WasmTypes::I32:
                SetGlobalValue(moduleEnv, offset, sourceGlobal->cnst.i32);
                break;
            case Wasm::WasmTypes::I64:
                SetGlobalValue(moduleEnv, offset, sourceGlobal->cnst.i64);
                break;
            case Wasm::WasmTypes::F32:
                SetGlobalValue(moduleEnv, offset, sourceGlobal->cnst.f32);
                break;
            case Wasm::WasmTypes::F64:
                SetGlobalValue(moduleEnv, offset, sourceGlobal->cnst.f64);
                break;
            default:
                break;
            }

        }
    }

    void WasmLibrary::WasmLoadIndirectFunctionTables(Wasm::WasmModule * wasmModule, ScriptContext* ctx, Var** indirectFunctionTables, Var* localModuleFunctions, Var* importFunctions)
    {
        //  Globals can be imported, thus the offset must be resolved at at the last possible moment before instantiating the table.
        wasmModule->ResolveTableElementOffsets();

        for (uint i = 0; i < wasmModule->GetTableSize(); ++i)
        {
            uint funcIndex = wasmModule->GetTableValue(i);
            if (funcIndex == Js::Constants::UninitializedValue)
            {
                // todo:: are we suppose to invalidate here, or do a runtime error if this is accessed?
                continue;
            }

            uint32 sigId = wasmModule->GetFunctionSignature(funcIndex)->GetSignatureId();
            sigId = wasmModule->GetEquivalentSignatureId(sigId);
            if (!indirectFunctionTables[sigId])
            {
                // TODO: initialize all indexes to "Js::Throw::RuntimeError" or similar type thing
                // now, indirect func call to invalid type will give nullptr deref
                indirectFunctionTables[sigId] = RecyclerNewArrayZ(ctx->GetRecycler(), Js::Var, wasmModule->GetTableSize());
            }
            Var funcObj = GetFunctionObjFromFunctionIndex(wasmModule, ctx, funcIndex, localModuleFunctions, importFunctions);
            if (funcObj)
            {
                indirectFunctionTables[sigId][i] = funcObj;
            }
        }
    }

    Var WasmLibrary::GetFunctionObjFromFunctionIndex(Wasm::WasmModule * wasmModule, ScriptContext* ctx, uint32 funcIndex, Var* localModuleFunctions, Var* importFunctions)
    {
        Wasm::FunctionIndexTypes::Type funcType = wasmModule->GetFunctionIndexType(funcIndex);
        uint32 normIndex = wasmModule->NormalizeFunctionIndex(funcIndex);
        switch (funcType)
        {
        case Wasm::FunctionIndexTypes::Function:
            return localModuleFunctions[normIndex];
            break;
        case Wasm::FunctionIndexTypes::Import:
            return importFunctions[normIndex];
            break;
        default:
            Assert(UNREACHED);
            return nullptr;
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

            WasmLoadGlobals(wasmModule, scriptContext, moduleEnvironmentPtr, ffi);
            WasmLoadDataSegs(wasmModule, heap, scriptContext);

            bool hasAnyLazyTraps = false;
            WasmLoadFunctions(wasmModule, scriptContext, moduleEnvironmentPtr, &exportObj, localModuleFunctions, &hasAnyLazyTraps);

            Var* importFunctions = moduleEnvironmentPtr + wasmModule->GetImportFuncOffset();
            WasmLoadImports(wasmModule, scriptContext, importFunctions, moduleEnvironmentPtr, ffi);


            Js::Var exportsNamespace = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
            WasmBuildObject(wasmModule, scriptContext, exportsNamespace, heap, &exportObj, &hasAnyLazyTraps, localModuleFunctions, importFunctions);

            Var** indirectFunctionTables = (Var**)(moduleEnvironmentPtr + wasmModule->GetTableEnvironmentOffset());
            WasmLoadIndirectFunctionTables(wasmModule, scriptContext, indirectFunctionTables, localModuleFunctions, importFunctions);
            uint32 startFuncIdx = wasmModule->GetStartFunction();
            if (start)
            {
                if (startFuncIdx != Js::Constants::UninitializedValue)
                {
                    *start = GetFunctionObjFromFunctionIndex(wasmModule, scriptContext, startFuncIdx, localModuleFunctions, importFunctions);
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