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

WebAssemblyInstance::WebAssemblyInstance(WebAssemblyModule * wasmModule, DynamicType * type) :
    DynamicObject(type),
    m_module(wasmModule)
{
}

/* static */
bool
WebAssemblyInstance::Is(Var value)
{
    return JavascriptOperators::GetTypeId(value) == TypeIds_WebAssemblyInstance;
}

/* static */
WebAssemblyInstance *
WebAssemblyInstance::FromVar(Var value)
{
    Assert(WebAssemblyInstance::Is(value));
    return static_cast<WebAssemblyInstance*>(value);
}

// Implements "new WebAssembly.Instance(moduleObject [, importObject])" as described here:
// https://github.com/WebAssembly/design/blob/master/JS.md#webassemblyinstance-constructor
Var
WebAssemblyInstance::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
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
        JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("WebAssembly.Instance"));
    }

    if (args.Info.Count < 2 || !WebAssemblyModule::Is(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedModule, _u("WebAssembly.Instance"));
    }
    WebAssemblyModule * module = WebAssemblyModule::FromVar(args[1]);

    Var importObject;
    if (args.Info.Count < 3)
    {
        importObject = scriptContext->GetLibrary()->GetUndefined();
    }
    else
    {
        if (!JavascriptOperators::IsUndefined(args[2]) && !JavascriptOperators::IsObject(args[2]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("WebAssembly.Instance"));
        }
        importObject = args[2];
    }

    if (module->GetImportCount() > 0 && !JavascriptOperators::IsObject(importObject))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("WebAssembly.Instance"));
    }

    return CreateInstance(module, importObject);
}

WebAssemblyInstance *
WebAssemblyInstance::CreateInstance(WebAssemblyModule * module, Var importObject)
{
    ScriptContext * scriptContext = module->GetScriptContext();
    Var* moduleEnvironmentPtr = RecyclerNewArrayZ(scriptContext->GetRecycler(), Var, module->GetModuleEnvironmentSize());
    Var* heap = moduleEnvironmentPtr + module->GetHeapOffset();
    WebAssemblyInstance * newInstance = RecyclerNewZ(scriptContext->GetRecycler(), WebAssemblyInstance, module, scriptContext->GetLibrary()->GetWebAssemblyInstanceType());
    Var* localModuleFunctions = moduleEnvironmentPtr + module->GetFuncOffset();

    LoadGlobals(module, scriptContext, moduleEnvironmentPtr, importObject);
    LoadDataSegs(module, heap, scriptContext);

    LoadFunctions(module, scriptContext, moduleEnvironmentPtr, localModuleFunctions);

    Var* importFunctions = moduleEnvironmentPtr + module->GetImportFuncOffset();
    LoadImports(module, scriptContext, importFunctions, moduleEnvironmentPtr, importObject);

    Js::Var exportsNamespace = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
    BuildObject(module, scriptContext, exportsNamespace, heap, newInstance, localModuleFunctions, importFunctions);

    Var** indirectFunctionTables = (Var**)(moduleEnvironmentPtr + module->GetTableEnvironmentOffset());
    LoadIndirectFunctionTables(module, scriptContext, indirectFunctionTables, localModuleFunctions, importFunctions);
    uint32 startFuncIdx = module->GetStartFunction();
    if (startFuncIdx != Js::Constants::UninitializedValue)
    {
        Var start = GetFunctionObjFromFunctionIndex(module, scriptContext, startFuncIdx, localModuleFunctions, importFunctions);
        Js::ScriptFunction* f = Js::AsmJsScriptFunction::FromVar(start);
        Js::CallInfo info(Js::CallFlags_New, 1);
        Js::Arguments startArg(info, &start);
        Js::JavascriptFunction::CallFunction<true>(f, f->GetEntryPoint(), startArg);
    }

    return newInstance;
}

void WebAssemblyInstance::LoadFunctions(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var* moduleMemoryPtr, Var* localModuleFunctions)
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

        FunctionBody* body = funcObj->GetFunctionBody();
        if (!PHASE_OFF(WasmDeferredPhase, body))
        {
            funcObj->GetDynamicType()->SetEntryPoint(WasmLibrary::WasmDeferredParseExternalThunk);
            entypointInfo->jsMethod = WasmLibrary::WasmDeferredParseInternalThunk;
        }
        else
        {
            AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();

            info->SetWasmReaderInfo(nullptr);
            funcObj->GetDynamicType()->SetEntryPoint(Js::AsmJsExternalEntryPoint);
            entypointInfo->jsMethod = AsmJsDefaultEntryThunk;
#if ENABLE_DEBUG_CONFIG_OPTIONS
            // Do MTJRC/MAIC:0 check
            const bool noJit = PHASE_OFF(BackEndPhase, body) || PHASE_OFF(FullJitPhase, body) || ctx->GetConfig()->IsNoNative();
            if (!noJit && (CONFIG_FLAG(ForceNative) || CONFIG_FLAG(MaxAsmJsInterpreterRunCount) == 0))
            {
                GenerateFunction(ctx->GetNativeCodeGenerator(), body, funcObj);
            }
#endif
        }
    }
}

void WebAssemblyInstance::LoadDataSegs(WebAssemblyModule * wasmModule, Var* heap, ScriptContext* ctx)
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
                JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), WASMERR_DataSegOutOfRange);
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

void WebAssemblyInstance::BuildObject(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var exportsNamespace, Var* heap, Var exportObj, Var* localModuleFunctions, Var* importFunctions)
{
    if (wasmModule->GetMemory()->minSize != 0 && wasmModule->GetMemory()->exported)
    {
        PropertyRecord const * propertyRecord = nullptr;
        ctx->GetOrAddPropertyRecord(_u("memory"), lstrlen(_u("memory")), &propertyRecord);
        JavascriptOperators::OP_SetProperty(exportsNamespace, propertyRecord->GetPropertyId(), *heap, ctx);
    }

    PropertyRecord const * exportsPropertyRecord = nullptr;
    ctx->GetOrAddPropertyRecord(_u("exports"), lstrlen(_u("exports")), &exportsPropertyRecord);
    JavascriptOperators::OP_SetProperty(exportObj, exportsPropertyRecord->GetPropertyId(), exportsNamespace, ctx);

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
                Wasm::WasmGlobal* global = wasmModule->globals->Item(funcExport->funcIndex);
                Assert(global->GetReferenceType() == Wasm::WasmGlobal::Const); //every global has to be resolved by this point

                if (global->GetMutability())
                {
                    JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), WASMERR_MutableGlobal);
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
    const char16* modName = wi->modName;
    uint32 modNameLen = wi->modNameLen;
    ctx->GetOrAddPropertyRecord(modName, modNameLen, &modPropertyRecord);
    Var modProp = JavascriptOperators::OP_GetProperty(ffi, modPropertyRecord->GetPropertyId(), ctx);



    const char16* name = wi->fnName;
    uint32 nameLen = wi->fnNameLen;
    Var prop = nullptr;
    if (nameLen > 0)
    {
        PropertyRecord const * propertyRecord = nullptr;
        ctx->GetOrAddPropertyRecord(name, nameLen, &propertyRecord);

        if (!JavascriptObject::Is(modProp))
        {
            JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidImport);
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

void WebAssemblyInstance::LoadImports(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var* importFunctions, Var moduleEnv, Var ffi)
{
    const uint32 importCount = wasmModule->GetImportCount();
    if (importCount > 0 && (!ffi || !JavascriptObject::Is(ffi)))
    {
        JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidImport);
    }
    for (uint32 i = 0; i < importCount; ++i)
    {
        Var prop = GetImportVariable(wasmModule->GetFunctionImport(i), ctx, ffi);
        if (!JavascriptFunction::Is(prop))
        {
            JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidImport);
        }
        importFunctions[i] = prop;
    }
}

void WebAssemblyInstance::LoadGlobals(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var moduleEnv, Var ffi)
{
    uint i = 0;
    uint count = (uint)wasmModule->globals->Count();
    while (i < count && wasmModule->globals->Item(i)->GetReferenceType() == Wasm::WasmGlobal::ImportedReference)
    {
        Wasm::WasmGlobal* global = wasmModule->globals->Item(i);
        Var prop = GetImportVariable(global->importVar, ctx, ffi);

        uint offset = wasmModule->GetOffsetForGlobal(global);
        global->SetReferenceType(Wasm::WasmGlobal::Const);

        if (!JavascriptNumber::Is(prop) && !TaggedInt::Is(prop))
        {
            JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidImport);
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
            float val = (float)JavascriptConversion::ToNumber(prop, ctx);
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
        Wasm::WasmGlobal* global = wasmModule->globals->Item(i);

        uint offset = wasmModule->GetOffsetForGlobal(global);
        Wasm::WasmGlobal* sourceGlobal = nullptr;
        if (global->GetReferenceType() == Wasm::WasmGlobal::Const)
        {
            sourceGlobal = global;
        }
        else
        {
            sourceGlobal = wasmModule->globals->Item(global->var.num);
            Assert(sourceGlobal->GetReferenceType() != Wasm::WasmGlobal::ImportedReference); //no imported globals at this point
            if (sourceGlobal->GetReferenceType() != Wasm::WasmGlobal::Const)
            {
                JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidGlobalRef);
            }
            global->SetReferenceType(Wasm::WasmGlobal::Const); //resolve global to const
            global->cnst = sourceGlobal->cnst;

            Assert(sourceGlobal->GetReferenceType() == Wasm::WasmGlobal::Const);
            if (sourceGlobal->GetType() != global->GetType())
            {
                JavascriptError::ThrowTypeError(ctx, VBSERR_TypeMismatch);
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
            Assert(UNREACHED);
            break;
        }

    }
}

void WebAssemblyInstance::LoadIndirectFunctionTables(WebAssemblyModule * wasmModule, ScriptContext* ctx, Var** indirectFunctionTables, Var* localModuleFunctions, Var* importFunctions)
{
    for (uint i = 0; i < wasmModule->GetTableSize(); ++i)
    {
        uint funcIndex = wasmModule->GetTableValue(i);
        if (funcIndex == Js::Constants::UninitializedValue)
        {
            // todo:: are we suppose to invalidate here, or do a runtime error if this is accessed?
            continue;
        }

        uint sigId = wasmModule->GetFunctionSignature(funcIndex)->GetSignatureId();
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

Var WebAssemblyInstance::GetFunctionObjFromFunctionIndex(WebAssemblyModule * wasmModule, ScriptContext* ctx, uint32 funcIndex, Var* localModuleFunctions, Var* importFunctions)
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

} // namespace Js
#endif // ENABLE_WASM
