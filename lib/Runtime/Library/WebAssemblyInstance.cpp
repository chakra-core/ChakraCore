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

Var GetImportVariable(Wasm::WasmImport* wi, ScriptContext* ctx, Var ffi)
{
    PropertyRecord const * modPropertyRecord = nullptr;
    const char16* modName = wi->modName;
    uint32 modNameLen = wi->modNameLen;
    ctx->GetOrAddPropertyRecord(modName, modNameLen, &modPropertyRecord);
    Var modProp = JavascriptOperators::OP_GetProperty(ffi, modPropertyRecord->GetPropertyId(), ctx);

    const char16* name = wi->importName;
    uint32 nameLen = wi->importNameLen;
    Var prop = nullptr;
    if (nameLen > 0)
    {
        PropertyRecord const * propertyRecord = nullptr;
        ctx->GetOrAddPropertyRecord(name, nameLen, &propertyRecord);

        if (!RecyclableObject::Is(modProp))
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
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedModule);
    }
    WebAssemblyModule * module = WebAssemblyModule::FromVar(args[1]);

    Var importObject = scriptContext->GetLibrary()->GetUndefined();
    if (args.Info.Count >= 3)
    {
        importObject = args[2];
    }

    return CreateInstance(module, importObject);
}

WebAssemblyInstance *
WebAssemblyInstance::CreateInstance(WebAssemblyModule * module, Var importObject)
{
    if (!JavascriptOperators::IsUndefined(importObject) && !JavascriptOperators::IsObject(importObject))
    {
        JavascriptError::ThrowTypeError(module->GetScriptContext(), JSERR_NeedObject);
    }

    if (module->GetImportCount() > 0 && !JavascriptOperators::IsObject(importObject))
    {
        JavascriptError::ThrowTypeError(module->GetScriptContext(), JSERR_NeedObject);
    }

    ScriptContext * scriptContext = module->GetScriptContext();
    WebAssemblyEnvironment environment(module);
    WebAssemblyInstance * newInstance = RecyclerNewZ(scriptContext->GetRecycler(), WebAssemblyInstance, module, scriptContext->GetLibrary()->GetWebAssemblyInstanceType());
    try
    {
        LoadImports(module, scriptContext, importObject, &environment);
        LoadGlobals(module, scriptContext, &environment);
        LoadFunctions(module, scriptContext, &environment);
        LoadDataSegs(module, scriptContext, &environment);
        LoadIndirectFunctionTable(module, scriptContext, &environment);
        Js::Var exportsNamespace = BuildObject(module, scriptContext, &environment);
        JavascriptOperators::OP_SetProperty(newInstance, PropertyIds::exports, exportsNamespace, scriptContext);

        uint32 startFuncIdx = module->GetStartFunction();
        if (startFuncIdx != Js::Constants::UninitializedValue)
        {
            AsmJsScriptFunction* start = environment.GetWasmFunction(startFuncIdx);
            Js::CallInfo info(Js::CallFlags_New, 1);
            Js::Arguments startArg(info, (Var*)&start);
            Js::JavascriptFunction::CallFunction<true>(start, start->GetEntryPoint(), startArg);
        }
    }
    catch (Wasm::WasmCompilationException e)
    {
        // Todo:: report the right message
        Unused(e);
        JavascriptError::ThrowTypeError(module->GetScriptContext(), VBSERR_InternalError);
    }

    return newInstance;
}

void WebAssemblyInstance::LoadFunctions(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env)
{
    FrameDisplay * frameDisplay = RecyclerNewPlus(ctx->GetRecycler(), sizeof(void*), FrameDisplay, 1);
    frameDisplay->SetItem(0, env->GetStartPtr());

    for (uint i = 0; i < wasmModule->GetWasmFunctionCount(); ++i)
    {
        if (i < wasmModule->GetImportedFunctionCount() && env->GetWasmFunction(i) != nullptr)
        {
            continue;
        }
        AsmJsScriptFunction * funcObj = ctx->GetLibrary()->CreateAsmJsScriptFunction(wasmModule->GetWasmFunctionInfo(i)->GetBody());
        FunctionBody* body = funcObj->GetFunctionBody();
        funcObj->SetModuleMemory(env->GetStartPtr());
        funcObj->SetSignature(body->GetAsmJsFunctionInfo()->GetWasmSignature());
        FunctionEntryPointInfo * entypointInfo = (FunctionEntryPointInfo*)funcObj->GetEntryPointInfo();
        entypointInfo->SetIsAsmJSFunction(true);
        entypointInfo->SetModuleAddress((uintptr_t)env->GetStartPtr());
        funcObj->SetEnvironment(frameDisplay);
        env->SetWasmFunction(i, funcObj);

        if (!PHASE_OFF(WasmDeferredPhase, body))
        {
            // if we still have WasmReaderInfo we haven't yet parsed
            if (body->GetAsmJsFunctionInfo()->GetWasmReaderInfo())
            {
                funcObj->GetDynamicType()->SetEntryPoint(WasmLibrary::WasmDeferredParseExternalThunk);
                entypointInfo->jsMethod = WasmLibrary::WasmDeferredParseInternalThunk;
            }
        }
        else
        {
            AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();
            if (info->GetWasmReaderInfo())
            {
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
                info->SetWasmReaderInfo(nullptr);
            }
        }
    }
}

void WebAssemblyInstance::LoadDataSegs(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env)
{
    WebAssemblyMemory* mem = env->GetMemory(0);
    if (wasmModule->HasMemoryImport())
    {
        if (mem == nullptr)
        {
            JavascriptError::ThrowTypeError(ctx, WASMERR_NeedMemoryObject);
        }
    }
    else
    {
        mem = wasmModule->CreateMemory();
        env->SetMemory(0, mem);
    }

    ArrayBuffer * buffer = mem->GetBuffer();
    if (buffer->IsDetached())
    {
        JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), JSERR_DetachedTypedArray);
    }
    for (uint32 iSeg = 0; iSeg < wasmModule->GetDataSegCount(); ++iSeg)
    {
        Wasm::WasmDataSegment* segment = wasmModule->GetDataSeg(iSeg);
        Assert(segment != nullptr);
        const uint32 offset = wasmModule->GetOffsetFromInit(segment->GetOffsetExpr(), env);
        const uint32 size = segment->GetSourceSize();

        if (size > 0)
        {
            if (UInt32Math::Add(offset, size) > buffer->GetByteLength())
            {
                JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), WASMERR_DataSegOutOfRange);
            }

            js_memcpy_s(buffer->GetBuffer() + offset, (uint32)buffer->GetByteLength() - offset, segment->GetData(), size);
        }
    }
}

Var WebAssemblyInstance::BuildObject(WebAssemblyModule * wasmModule, ScriptContext* scriptContext, WebAssemblyEnvironment* env)
{
    Js::Var exportsNamespace = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
    for (uint32 iExport = 0; iExport < wasmModule->GetExportCount(); ++iExport)
    {
        Wasm::WasmExport* wasmExport = wasmModule->GetExport(iExport);
        if (wasmExport  && wasmExport->nameLength > 0)
        {
            PropertyRecord const * propertyRecord = nullptr;
            scriptContext->GetOrAddPropertyRecord(wasmExport->name, wasmExport->nameLength, &propertyRecord);

            Var obj = scriptContext->GetLibrary()->GetUndefined();
            switch (wasmExport->kind)
            {
            case Wasm::ExternalKinds::Table:
                obj = env->GetTable(wasmExport->index);
                break;
            case Wasm::ExternalKinds::Memory:
                obj = env->GetMemory(wasmExport->index);
                break;
            case Wasm::ExternalKinds::Function:
                obj = env->GetWasmFunction(wasmExport->index);
                break;
            case Wasm::ExternalKinds::Global:
                Wasm::WasmGlobal* global = wasmModule->GetGlobal(wasmExport->index);
                if (global->IsMutable())
                {
                    JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), WASMERR_MutableGlobal);
                }
                Wasm::WasmConstLitNode cnst = env->GetGlobalValue(global);
                switch (global->GetType())
                {
                case Wasm::WasmTypes::I32:
                    obj = JavascriptNumber::ToVar(cnst.i32, scriptContext);
                    break;
                case Wasm::WasmTypes::F32:
                    obj = JavascriptNumber::New(cnst.f32, scriptContext);
                    break;
                case Wasm::WasmTypes::F64:
                    obj = JavascriptNumber::New(cnst.f64, scriptContext);
                    break;
                case Wasm::WasmTypes::I64:
                    JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), WASMERR_InvalidTypeConversion);
                default:
                    Assert(UNREACHED);
                    break;
                }
            }
            JavascriptOperators::OP_SetProperty(exportsNamespace, propertyRecord->GetPropertyId(), obj, scriptContext);
        }
    }
    return exportsNamespace;
}

void WebAssemblyInstance::LoadImports(
    WebAssemblyModule * wasmModule,
    ScriptContext* ctx,
    Var ffi,
    WebAssemblyEnvironment* env)
{
    const uint32 importCount = wasmModule->GetImportCount();
    if (importCount > 0 && (!ffi || !RecyclableObject::Is(ffi)))
    {
        JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidImport);
    }

    uint32 counters[Wasm::ExternalKinds::Limit];
    memset(counters, 0, sizeof(counters));
    for (uint32 i = 0; i < importCount; ++i)
    {
        Wasm::WasmImport* import = wasmModule->GetImport(i);
        Var prop = GetImportVariable(import, ctx, ffi);
        uint32& counter = counters[import->kind];
        switch (import->kind)
        {
        case Wasm::ExternalKinds::Function:
        {
            if (!JavascriptFunction::Is(prop))
            {
                JavascriptError::ThrowTypeError(ctx, JSERR_Property_NeedFunction);
            }
            Assert(counter < wasmModule->GetImportedFunctionCount());
            Assert(wasmModule->GetFunctionIndexType(counter) == Wasm::FunctionIndexTypes::ImportThunk);

            env->SetImportedFunction(counter, prop);
            if (AsmJsScriptFunction::IsWasmScriptFunction(prop))
            {
                Assert(env->GetWasmFunction(counter) == nullptr);
                AsmJsScriptFunction* func = AsmJsScriptFunction::FromVar(prop);
                if (!wasmModule->GetSignature(counter)->IsEquivalent(func->GetSignature()))
                {
                    JavascriptError::ThrowTypeError(ctx, WASMERR_SignatureMismatch);
                }
                // Imported Wasm functions can be called directly
                env->SetWasmFunction(counter, func);
            }
            break;
        }
        case Wasm::ExternalKinds::Memory:
        {
            Assert(wasmModule->HasMemoryImport());
            if (wasmModule->HasMemoryImport())
            {
                if (!WebAssemblyMemory::Is(prop))
                {
                    JavascriptError::ThrowTypeError(ctx, WASMERR_NeedMemoryObject);
                }
                WebAssemblyMemory * mem = WebAssemblyMemory::FromVar(prop);
                if (!wasmModule->IsValidMemoryImport(mem))
                {
                    JavascriptError::ThrowTypeError(ctx, WASMERR_NeedMemoryObject);
                }
                env->SetMemory(counter, mem);
            }
            break;
        }
        case Wasm::ExternalKinds::Table:
        {
            Assert(wasmModule->HasTableImport());
            if (wasmModule->HasTableImport())
            {
                if (!WebAssemblyTable::Is(prop))
                {
                    JavascriptError::ThrowTypeError(ctx, WASMERR_NeedTableObject);
                }
                WebAssemblyTable * table = WebAssemblyTable::FromVar(prop);

                if (!wasmModule->IsValidTableImport(table))
                {
                    JavascriptError::ThrowTypeError(ctx, WASMERR_NeedTableObject);
                }
                env->SetTable(counter, table);
            }
            break;
        }
        case Wasm::ExternalKinds::Global:
        {
            Wasm::WasmGlobal* global = wasmModule->GetGlobal(counter);
            if (global->IsMutable() || (!JavascriptNumber::Is(prop) && !TaggedInt::Is(prop)))
            {
                JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidImport);
            }

            Assert(global->GetReferenceType() == Wasm::ReferenceTypes::ImportedReference);
            Wasm::WasmConstLitNode cnst = {0};
            switch (global->GetType())
            {
            case Wasm::WasmTypes::I32: cnst.i32 = JavascriptConversion::ToInt32(prop, ctx); break;
            case Wasm::WasmTypes::F32: cnst.f32 = (float)JavascriptConversion::ToNumber(prop, ctx); break;
            case Wasm::WasmTypes::F64: cnst.f64 = JavascriptConversion::ToNumber(prop, ctx); break;
            case Wasm::WasmTypes::I64: Js::JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidTypeConversion);
            default:
                Assert(UNREACHED);
                JavascriptError::ThrowTypeError(ctx, VBSERR_InternalError);
            }
            env->SetGlobalValue(global, cnst);
            break;
        }
        default:
            Assert(UNREACHED);
            JavascriptError::ThrowTypeError(ctx, VBSERR_InternalError);
        }
        ++counter;
    }
}

void WebAssemblyInstance::LoadGlobals(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env)
{
    uint count = wasmModule->GetGlobalCount();
    for (uint i = 0; i < count; i++)
    {
        Wasm::WasmGlobal* global = wasmModule->GetGlobal(i);
        Wasm::WasmConstLitNode cnst = {};

        if (global->GetReferenceType() == Wasm::ReferenceTypes::ImportedReference)
        {
            // the value should already be resolved
            continue;
        }

        if (global->GetReferenceType() == Wasm::ReferenceTypes::LocalReference)
        {
            Wasm::WasmGlobal* sourceGlobal = wasmModule->GetGlobal(global->GetGlobalIndexInit());
            if (sourceGlobal->GetReferenceType() != Wasm::ReferenceTypes::Const &&
                sourceGlobal->GetReferenceType() != Wasm::ReferenceTypes::ImportedReference)
            {
                JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidGlobalRef);
            }
            
            if (sourceGlobal->GetType() != global->GetType())
            {
                JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidTypeConversion);
            }
            cnst = env->GetGlobalValue(sourceGlobal);
        }
        else
        {
            cnst = global->GetConstInit();
        }

        env->SetGlobalValue(global, cnst);
    }
}

void WebAssemblyInstance::LoadIndirectFunctionTable(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env)
{
    WebAssemblyTable* table = env->GetTable(0);
    if (wasmModule->HasTableImport())
    {
        if (table == nullptr)
        {
            JavascriptError::ThrowTypeError(ctx, WASMERR_NeedTableObject);
        }
    }
    else
    {
        table = wasmModule->CreateTable();
        env->SetTable(0, table);
    }

    for (uint elementsIndex = 0; elementsIndex < wasmModule->GetElementSegCount(); ++elementsIndex)
    {
        Wasm::WasmElementSegment* eSeg = wasmModule->GetElementSeg(elementsIndex);

        if (eSeg->GetNumElements() > 0)
        {
            uint offset = wasmModule->GetOffsetFromInit(eSeg->GetOffsetExpr(), env);
            if (UInt32Math::Add(offset, eSeg->GetNumElements()) > table->GetCurrentLength())
            {
                JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), WASMERR_ElementSegOutOfRange);
            }
            for (uint segIndex = 0; segIndex < eSeg->GetNumElements(); ++segIndex)
            {
                uint funcIndex = eSeg->GetElement(segIndex);
                Var funcObj = env->GetWasmFunction(funcIndex);
                table->DirectSetValue(segIndex + offset, funcObj);
            }
        }
    }
}

} // namespace Js
#endif // ENABLE_WASM
