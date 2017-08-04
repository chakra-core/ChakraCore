//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
#include "../WasmReader/WasmReaderPch.h"
namespace Js
{

Var GetImportVariable(Wasm::WasmImport* wi, ScriptContext* ctx, Var ffi)
{
    PropertyRecord const * modPropertyRecord = nullptr;
    const char16* modName = wi->modName;
    uint32 modNameLen = wi->modNameLen;
    ctx->GetOrAddPropertyRecord(modName, modNameLen, &modPropertyRecord);
    Var modProp = JavascriptOperators::OP_GetProperty(ffi, modPropertyRecord->GetPropertyId(), ctx);
    if (!JavascriptOperators::IsObject(modProp))
    {
        JavascriptError::ThrowTypeErrorVar(ctx, WASMERR_InvalidImportModule, modName);
    }

    const char16* name = wi->importName;
    uint32 nameLen = wi->importNameLen;
    PropertyRecord const * propertyRecord = nullptr;
    ctx->GetOrAddPropertyRecord(name, nameLen, &propertyRecord);

    if (!JavascriptOperators::IsObject(modProp))
    {
        JavascriptError::ThrowTypeErrorVar(ctx, WASMERR_InvalidImport, modName, name, _u("Object"));
    }
    return JavascriptOperators::OP_GetProperty(modProp, propertyRecord->GetPropertyId(), ctx);
}

WebAssemblyInstance::WebAssemblyInstance(WebAssemblyModule * wasmModule, DynamicType * type) :
    DynamicObject(type),
    m_module(wasmModule),
    m_exports(nullptr)
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

Var
WebAssemblyInstance::GetterExports(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count == 0 || !WebAssemblyInstance::Is(args[0]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedInstanceObject);
    }

    WebAssemblyInstance* instance = WebAssemblyInstance::FromVar(args[0]);
    Js::Var exports = instance->m_exports;
    if (!exports || !DynamicObject::Is(exports))
    {
        Assert(UNREACHED);
        exports = scriptContext->GetLibrary()->GetUndefined();
    }
    return exports;
}

WebAssemblyInstance *
WebAssemblyInstance::CreateInstance(WebAssemblyModule * module, Var importObject)
{
    if (!JavascriptOperators::IsUndefined(importObject) && !JavascriptOperators::IsObject(importObject))
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
        ValidateTableAndMemory(module, scriptContext, &environment);
        try
        {
            LoadDataSegs(module, scriptContext, &environment);
            LoadIndirectFunctionTable(module, scriptContext, &environment);
        }
        catch (...)
        {
            AssertMsg(UNREACHED, "By spec, we should not have any exceptions possible here");
            throw;
        }
        newInstance->m_exports = BuildObject(module, scriptContext, &environment);
    }
    catch (Wasm::WasmCompilationException& e)
    {
        JavascriptError::ThrowWebAssemblyLinkErrorVar(scriptContext, WASMERR_WasmLinkError, e.GetTempErrorMessageRef());
    }

    uint32 startFuncIdx = module->GetStartFunction();
    if (startFuncIdx != Js::Constants::UninitializedValue)
    {
        AsmJsScriptFunction* start = environment.GetWasmFunction(startFuncIdx);
        Js::CallInfo info(Js::CallFlags_New, 1);
        Js::Arguments startArg(info, (Var*)&start);
        Js::JavascriptFunction::CallFunction<true>(start, start->GetEntryPoint(), startArg);
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
        Wasm::WasmFunctionInfo* wasmFuncInfo = wasmModule->GetWasmFunctionInfo(i);
        FunctionBody* body = wasmFuncInfo->GetBody();
        AsmJsScriptFunction * funcObj = ctx->GetLibrary()->CreateAsmJsScriptFunction(body);
        funcObj->SetModuleMemory((Field(Var)*)env->GetStartPtr());
        funcObj->SetSignature(body->GetAsmJsFunctionInfo()->GetWasmSignature());
        funcObj->SetEnvironment(frameDisplay);

        // Todo:: need to fix issue #2452 before we can do this,
        // otherwise we'll change the type of the functions and cause multiple instance to not share jitted code
        //Wasm::WasmSignature* sig = wasmFuncInfo->GetSignature();
        //funcObj->SetPropertyWithAttributes(PropertyIds::length, JavascriptNumber::ToVar(sig->GetParamCount(), ctx), PropertyNone, nullptr);
        //funcObj->SetPropertyWithAttributes(PropertyIds::name, JavascriptConversion::ToString(JavascriptNumber::ToVar(i, ctx), ctx), PropertyNone, nullptr);

        env->SetWasmFunction(i, funcObj);

        if (!PHASE_OFF(WasmDeferredPhase, body))
        {
            // if we still have WasmReaderInfo we haven't yet parsed
            if (body->GetAsmJsFunctionInfo()->GetWasmReaderInfo())
            {
                WasmLibrary::SetWasmEntryPointToInterpreter(funcObj, true);
            }
        }
        else
        {
            AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();
            if (info->GetWasmReaderInfo())
            {
                WasmLibrary::SetWasmEntryPointToInterpreter(funcObj, false);
                WAsmJs::JitFunctionIfReady(funcObj);
                info->SetWasmReaderInfo(nullptr);
            }
        }
    }
}

void WebAssemblyInstance::LoadDataSegs(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env)
{
    WebAssemblyMemory* mem = env->GetMemory(0);
    Assert(mem);
    ArrayBuffer* buffer = mem->GetBuffer();

    for (uint32 iSeg = 0; iSeg < wasmModule->GetDataSegCount(); ++iSeg)
    {
        Wasm::WasmDataSegment* segment = wasmModule->GetDataSeg(iSeg);
        Assert(segment != nullptr);
        const uint32 offset = env->GetDataSegmentOffset(iSeg);
        const uint32 size = segment->GetSourceSize();

        if (size > 0)
        {
            js_memcpy_s(buffer->GetBuffer() + offset, (uint32)buffer->GetByteLength() - offset, segment->GetData(), size);
        }
    }
}

Var WebAssemblyInstance::BuildObject(WebAssemblyModule * wasmModule, ScriptContext* scriptContext, WebAssemblyEnvironment* env)
{
    Js::Var exportsNamespace = scriptContext->GetLibrary()->CreateObject(scriptContext->GetLibrary()->GetNull());
    for (uint32 iExport = 0; iExport < wasmModule->GetExportCount(); ++iExport)
    {
        Wasm::WasmExport* wasmExport = wasmModule->GetExport(iExport);
        Assert(wasmExport);
        if (wasmExport)
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
    DynamicObject::FromVar(exportsNamespace)->PreventExtensions();
    return exportsNamespace;
}

void WebAssemblyInstance::LoadImports(
    WebAssemblyModule * wasmModule,
    ScriptContext* ctx,
    Var ffi,
    WebAssemblyEnvironment* env)
{
    const uint32 importCount = wasmModule->GetImportCount();
    if (importCount > 0 && (!ffi || !JavascriptOperators::IsObject(ffi)))
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
                JavascriptError::ThrowWebAssemblyLinkErrorVar(ctx, WASMERR_InvalidImport, import->modName, import->importName, _u("Function"));
            }
            Assert(counter < wasmModule->GetImportedFunctionCount());
            Assert(wasmModule->GetFunctionIndexType(counter) == Wasm::FunctionIndexTypes::ImportThunk);

            env->SetImportedFunction(counter, prop);
            if (AsmJsScriptFunction::IsWasmScriptFunction(prop))
            {
                Assert(env->GetWasmFunction(counter) == nullptr);
                AsmJsScriptFunction* func = AsmJsScriptFunction::FromVar(prop);
                if (!wasmModule->GetWasmFunctionInfo(counter)->GetSignature()->IsEquivalent(func->GetSignature()))
                {
                    char16 temp[2048] = { 0 };
                    char16 importargs[512] = { 0 };
                    wasmModule->GetWasmFunctionInfo(counter)->GetSignature()->WriteSignatureToString(importargs, 512);
                    char16 exportargs[512] = { 0 };
                    func->GetSignature()->WriteSignatureToString(exportargs, 512);
                    _snwprintf_s(temp, 2048, _TRUNCATE, _u("%ls%ls to %ls%ls"), func->GetDisplayName()->GetString(), exportargs, import->importName, importargs);
                    // this makes a copy of the error message buffer, so it's fine to not worry about clean-up
                    JavascriptError::ThrowWebAssemblyLinkError(ctx, WASMERR_LinkSignatureMismatch, temp);
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
                    JavascriptError::ThrowWebAssemblyLinkErrorVar(ctx, WASMERR_InvalidImport, import->modName, import->importName, _u("WebAssembly.Memory"));
                }
                WebAssemblyMemory * mem = WebAssemblyMemory::FromVar(prop);

                if (mem->GetInitialLength() < wasmModule->GetMemoryInitSize())
                {
                    JavascriptError::ThrowWebAssemblyLinkErrorVar(ctx, WASMERR_InvalidInitialSize, _u("WebAssembly.Memory"), mem->GetInitialLength(), wasmModule->GetMemoryInitSize());
                }
                if (mem->GetMaximumLength() > wasmModule->GetMemoryMaxSize())
                {
                    JavascriptError::ThrowWebAssemblyLinkErrorVar(ctx, WASMERR_InvalidMaximumSize, _u("WebAssembly.Memory"), mem->GetMaximumLength(), wasmModule->GetMemoryMaxSize());
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
                    JavascriptError::ThrowWebAssemblyLinkErrorVar(ctx, WASMERR_InvalidImport, import->modName, import->importName, _u("WebAssembly.Table"));
                }
                WebAssemblyTable * table = WebAssemblyTable::FromVar(prop);

                if (table->GetInitialLength() < wasmModule->GetTableInitSize())
                {
                    JavascriptError::ThrowWebAssemblyLinkErrorVar(ctx, WASMERR_InvalidInitialSize, _u("WebAssembly.Table"), table->GetInitialLength(), wasmModule->GetTableInitSize());
                }
                if (table->GetMaximumLength() > wasmModule->GetTableMaxSize())
                {
                    JavascriptError::ThrowWebAssemblyLinkErrorVar(ctx, WASMERR_InvalidMaximumSize, _u("WebAssembly.Table"), table->GetMaximumLength(), wasmModule->GetTableMaxSize());
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
                JavascriptError::ThrowWebAssemblyLinkErrorVar(ctx, WASMERR_InvalidImport, import->modName, import->importName, _u("Number"));
            }

            Assert(global->GetReferenceType() == Wasm::GlobalReferenceTypes::ImportedReference);
            Wasm::WasmConstLitNode cnst = {0};
            switch (global->GetType())
            {
            case Wasm::WasmTypes::I32: cnst.i32 = JavascriptConversion::ToInt32(prop, ctx); break;
            case Wasm::WasmTypes::F32: cnst.f32 = (float)JavascriptConversion::ToNumber(prop, ctx); break;
            case Wasm::WasmTypes::F64: cnst.f64 = JavascriptConversion::ToNumber(prop, ctx); break;
            case Wasm::WasmTypes::I64: Js::JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidTypeConversion);
            default:
                Js::Throw::InternalError();
            }
            env->SetGlobalValue(global, cnst);
            break;
        }
        default:
            Js::Throw::InternalError();
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

        if (global->GetReferenceType() == Wasm::GlobalReferenceTypes::ImportedReference)
        {
            // the value should already be resolved
            continue;
        }

        if (global->GetReferenceType() == Wasm::GlobalReferenceTypes::LocalReference)
        {
            Wasm::WasmGlobal* sourceGlobal = wasmModule->GetGlobal(global->GetGlobalIndexInit());
            if (sourceGlobal->GetReferenceType() != Wasm::GlobalReferenceTypes::Const &&
                sourceGlobal->GetReferenceType() != Wasm::GlobalReferenceTypes::ImportedReference)
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
    Assert(table != nullptr);

    for (uint elementsIndex = 0; elementsIndex < wasmModule->GetElementSegCount(); ++elementsIndex)
    {
        Wasm::WasmElementSegment* eSeg = wasmModule->GetElementSeg(elementsIndex);

        if (eSeg->GetNumElements() > 0)
        {
            uint offset = env->GetElementSegmentOffset(elementsIndex);
            for (uint segIndex = 0; segIndex < eSeg->GetNumElements(); ++segIndex)
            {
                uint funcIndex = eSeg->GetElement(segIndex);
                Var funcObj = env->GetWasmFunction(funcIndex);
                table->DirectSetValue(segIndex + offset, funcObj);
            }
        }
    }
}


void WebAssemblyInstance::ValidateTableAndMemory(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment* env)
{
    WebAssemblyTable* table = env->GetTable(0);
    if (wasmModule->HasTableImport())
    {
        if (table == nullptr)
        {
            JavascriptError::ThrowWebAssemblyLinkError(ctx, WASMERR_NeedTableObject);
        }
    }
    else
    {
        table = wasmModule->CreateTable();
        env->SetTable(0, table);
    }

    WebAssemblyMemory* mem = env->GetMemory(0);
    if (wasmModule->HasMemoryImport())
    {
        if (mem == nullptr)
        {
            JavascriptError::ThrowWebAssemblyLinkError(ctx, WASMERR_NeedMemoryObject);
        }
    }
    else
    {
        mem = wasmModule->CreateMemory();
        if (mem == nullptr)
        {
            JavascriptError::ThrowWebAssemblyLinkError(ctx, WASMERR_MemoryCreateFailed);
        }
        env->SetMemory(0, mem);
    }
    ArrayBuffer * buffer = mem->GetBuffer();
    if (buffer->IsDetached())
    {
        JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), JSERR_DetachedTypedArray);
    }

    env->CalculateOffsets(table, mem);
}

} // namespace Js
#endif // ENABLE_WASM
