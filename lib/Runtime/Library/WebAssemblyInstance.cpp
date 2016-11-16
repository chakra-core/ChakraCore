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

template <typename T>
void static SetGlobalValue(Var moduleEnv, uint offset, T val)
{
    T* slot = (T*)moduleEnv + offset;
    *slot = val;
}

template <typename T>
T static GetGlobalValue(Var moduleEnv, uint offset)
{
    T* slot = (T*)moduleEnv + offset;
    return *slot;
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
        LoadImports(module, scriptContext, importObject, environment);
        LoadGlobals(module, scriptContext, environment);
        LoadFunctions(module, scriptContext, environment);
        LoadDataSegs(module, scriptContext, environment);
        LoadIndirectFunctionTable(module, scriptContext, environment);
        Js::Var exportsNamespace = BuildObject(module, scriptContext, environment);
        JavascriptOperators::OP_SetProperty(newInstance, PropertyIds::exports, exportsNamespace, scriptContext);

        uint32 startFuncIdx = module->GetStartFunction();
        if (startFuncIdx != Js::Constants::UninitializedValue)
        {
            Var start = GetFunctionObjFromFunctionIndex(module, scriptContext, startFuncIdx, environment);
            Js::ScriptFunction* f = Js::AsmJsScriptFunction::FromVar(start);
            Js::CallInfo info(Js::CallFlags_New, 1);
            Js::Arguments startArg(info, &start);
            Js::JavascriptFunction::CallFunction<true>(f, f->GetEntryPoint(), startArg);
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

void WebAssemblyInstance::LoadFunctions(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment env)
{
    FrameDisplay * frameDisplay = RecyclerNewPlus(ctx->GetRecycler(), sizeof(void*), FrameDisplay, 1);
    frameDisplay->SetItem(0, env.ptr);

    for (uint i = 0; i < wasmModule->GetWasmFunctionCount(); ++i)
    {
        if (i < wasmModule->GetImportedFunctionCount() && env.functions[i] != nullptr)
        {
            if (!AsmJsScriptFunction::IsWasmScriptFunction(env.functions[i]))
            {
                JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), WASMERR_InvalidImport);
            }
            continue;
        }
        AsmJsScriptFunction * funcObj = ctx->GetLibrary()->CreateAsmJsScriptFunction(wasmModule->GetWasmFunctionInfo(i)->GetBody());
        FunctionBody* body = funcObj->GetFunctionBody();
        funcObj->SetModuleMemory(env.memory);
        funcObj->SetSignature(body->GetAsmJsFunctionInfo()->GetWasmSignature());
        FunctionEntryPointInfo * entypointInfo = (FunctionEntryPointInfo*)funcObj->GetEntryPointInfo();
        entypointInfo->SetIsAsmJSFunction(true);
        entypointInfo->SetModuleAddress((uintptr_t)env.ptr);
        funcObj->SetEnvironment(frameDisplay);
        env.functions[i] = funcObj;

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

void WebAssemblyInstance::LoadDataSegs(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment env)
{
    WebAssemblyMemory** memoryObject = (WebAssemblyMemory**)env.memory;
    if (wasmModule->HasMemoryImport())
    {
        if (*memoryObject == nullptr)
        {
            JavascriptError::ThrowTypeError(ctx, WASMERR_NeedMemoryObject);
        }
    }
    else
    {
        *memoryObject = wasmModule->CreateMemory();
    }

    WebAssemblyMemory * mem = WebAssemblyMemory::FromVar(*memoryObject);

    ArrayBuffer * buffer = mem->GetBuffer();
    if (buffer->IsDetached())
    {
        JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), JSERR_DetachedTypedArray);
    }
    for (uint32 iSeg = 0; iSeg < wasmModule->GetDataSegCount(); ++iSeg)
    {
        Wasm::WasmDataSegment* segment = wasmModule->GetDataSeg(iSeg);
        Assert(segment != nullptr);
        const uint32 offset = wasmModule->GetOffsetFromInit(segment->GetOffsetExpr(), env.ptr);
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

Var WebAssemblyInstance::BuildObject(WebAssemblyModule * wasmModule, ScriptContext* scriptContext, WebAssemblyEnvironment env)
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
                obj = *env.table;
                break;
            case Wasm::ExternalKinds::Memory:
                obj = *env.memory;
                break;
            case Wasm::ExternalKinds::Function:
                obj = GetFunctionObjFromFunctionIndex(wasmModule, scriptContext, wasmExport->index, env);
                break;
            case Wasm::ExternalKinds::Global:
                Wasm::WasmGlobal* global = wasmModule->GetGlobal(wasmExport->index);
                if (global->IsMutable())
                {
                    JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), WASMERR_MutableGlobal);
                }
                uint32 offset = wasmModule->GetOffsetForGlobal(global);
                switch (global->GetType())
                {
                case Wasm::WasmTypes::I32:
                    obj = JavascriptNumber::ToVar(GetGlobalValue<int>(env.ptr, offset), scriptContext);
                    break;
                case Wasm::WasmTypes::F32:
                    obj = JavascriptNumber::New(GetGlobalValue<float>(env.ptr, offset), scriptContext);
                    break;
                case Wasm::WasmTypes::F64:
                    obj = JavascriptNumber::New(GetGlobalValue<double>(env.ptr, offset), scriptContext);
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
    WebAssemblyEnvironment env)
{
    const uint32 importCount = wasmModule->GetImportCount();
    if (importCount > 0 && (!ffi || !RecyclableObject::Is(ffi)))
    {
        JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidImport);
    }

    uint32 iImportFunction = 0;
    uint32 iGlobal = 0;
    for (uint32 i = 0; i < importCount; ++i)
    {
        Wasm::WasmImport* import = wasmModule->GetImport(i);
        Var prop = GetImportVariable(import, ctx, ffi);
        switch (import->kind)
        {
        case Wasm::ExternalKinds::Function:
        {
            if (!JavascriptFunction::Is(prop))
            {
                JavascriptError::ThrowTypeError(ctx, JSERR_Property_NeedFunction);
            }
            Assert(iImportFunction < wasmModule->GetImportedFunctionCount());
            Assert(wasmModule->GetFunctionIndexType(iImportFunction) == Wasm::FunctionIndexTypes::ImportThunk);

            env.imports[iImportFunction] = prop;
            if (AsmJsScriptFunction::IsWasmScriptFunction(prop))
            {
                Assert(env.functions[iImportFunction] == nullptr);
                // Imported Wasm functions can be called directly
                env.functions[iImportFunction] = prop;
            }
            iImportFunction++;
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
                *env.memory = mem;
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
                *env.table = table;
            }
            break;
        }
        case Wasm::ExternalKinds::Global:
        {
            Wasm::WasmGlobal* global = wasmModule->GetGlobal(iGlobal++);
            if (global->IsMutable() || (!JavascriptNumber::Is(prop) && !TaggedInt::Is(prop)))
            {
                JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidImport);
            }

            Assert(global->GetReferenceType() == Wasm::ReferenceTypes::ImportedReference);
            uint32 offset = wasmModule->GetOffsetForGlobal(global);
            switch (global->GetType())
            {
            case Wasm::WasmTypes::I32:
            {
                int val = JavascriptConversion::ToInt32(prop, ctx);
                SetGlobalValue(env.ptr, offset, val);
                break;
            }
            case Wasm::WasmTypes::F32:
            {
                float val = (float)JavascriptConversion::ToNumber(prop, ctx);
                SetGlobalValue(env.ptr, offset, val);
                break;
            }
            case Wasm::WasmTypes::F64:
            {
                double val = JavascriptConversion::ToNumber(prop, ctx);
                SetGlobalValue(env.ptr, offset, val);
                break;
            }
            case Wasm::WasmTypes::I64:
                Js::JavascriptError::ThrowTypeError(ctx, WASMERR_InvalidTypeConversion);
            default:
                Assert(UNREACHED);
                break;

            }
            break;
        }
        default:
            Assert(UNREACHED);
            break;
        }
        
    }
}

void WebAssemblyInstance::LoadGlobals(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment env)
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
            uint32 sourceOffset = wasmModule->GetOffsetForGlobal(sourceGlobal);
            switch (sourceGlobal->GetType())
            {
            case Wasm::WasmTypes::I32:
                cnst.i32 = GetGlobalValue<int>(env.ptr, sourceOffset);
                break;
            case Wasm::WasmTypes::I64:
                // We shouldn't reference int64 global since we can't import them
                Assert(UNREACHED);
                cnst.i64 = GetGlobalValue<int64>(env.ptr, sourceOffset);
                break;
            case Wasm::WasmTypes::F32:
                cnst.f32 = GetGlobalValue<float>(env.ptr, sourceOffset);
                break;
            case Wasm::WasmTypes::F64:
                cnst.f64 = GetGlobalValue<double>(env.ptr, sourceOffset);
                break;
            default:
                Assert(UNREACHED);
                break;
            }
        }
        else
        {
            cnst = global->GetConstInit();
        }

        uint offset = wasmModule->GetOffsetForGlobal(global);
        switch (global->GetType())
        {
        case Wasm::WasmTypes::I32:
            SetGlobalValue(env.ptr, offset, cnst.i32);
            break;
        case Wasm::WasmTypes::I64:
            SetGlobalValue(env.ptr, offset, cnst.i64);
            break;
        case Wasm::WasmTypes::F32:
            SetGlobalValue(env.ptr, offset, cnst.f32);
            break;
        case Wasm::WasmTypes::F64:
            SetGlobalValue(env.ptr, offset, cnst.f64);
            break;
        default:
            Assert(UNREACHED);
            break;
        }

    }
}

void WebAssemblyInstance::LoadIndirectFunctionTable(WebAssemblyModule * wasmModule, ScriptContext* ctx, WebAssemblyEnvironment env)
{
    WebAssemblyTable** tableObject = (WebAssemblyTable**)env.table;
    if (wasmModule->HasTableImport())
    {
        if (*tableObject == nullptr)
        {
            JavascriptError::ThrowTypeError(ctx, WASMERR_NeedTableObject);
        }
    }
    else
    {
        *tableObject = wasmModule->CreateTable();
    }

    WebAssemblyTable * table = *tableObject;

    for (uint elementsIndex = 0; elementsIndex < wasmModule->GetElementSegCount(); ++elementsIndex)
    {
        Wasm::WasmElementSegment* eSeg = wasmModule->GetElementSeg(elementsIndex);

        if (eSeg->GetNumElements() > 0)
        {
            uint offset = wasmModule->GetOffsetFromInit(eSeg->GetOffsetExpr(), env.ptr);
            if (UInt32Math::Add(offset, eSeg->GetNumElements()) > table->GetCurrentLength())
            {
                JavascriptError::ThrowTypeError(wasmModule->GetScriptContext(), WASMERR_ElementSegOutOfRange);
            }
            for (uint segIndex = 0; segIndex < eSeg->GetNumElements(); ++segIndex)
            {
                uint funcIndex = eSeg->GetElement(segIndex);
                Var funcObj = GetFunctionObjFromFunctionIndex(wasmModule, ctx, funcIndex, env);
                table->DirectSetValue(segIndex + offset, funcObj);
            }
        }
    }
}

Var WebAssemblyInstance::GetFunctionObjFromFunctionIndex(WebAssemblyModule * wasmModule, ScriptContext* ctx, uint32 funcIndex, WebAssemblyEnvironment env)
{
    Wasm::FunctionIndexTypes::Type funcType = wasmModule->GetFunctionIndexType(funcIndex);
    switch (funcType)
    {
    case Wasm::FunctionIndexTypes::ImportThunk:
    case Wasm::FunctionIndexTypes::Function:
        return env.functions[funcIndex];
        break;
    default:
        Assert(UNREACHED);
        throw Wasm::WasmCompilationException(_u("Unexpected function type for function index %u"), funcIndex);
    }
}

WebAssemblyInstance::WebAssemblyEnvironment::WebAssemblyEnvironment(WebAssemblyModule* module)
{
    ScriptContext* scriptContext = module->GetScriptContext();
    uint32 envSize = module->GetModuleEnvironmentSize();
    this->ptr = RecyclerNewArrayZ(scriptContext->GetRecycler(), Var, envSize);
    this->memory = this->ptr + module->GetMemoryOffset();
    this->imports = this->ptr + module->GetImportFuncOffset();
    this->functions = this->ptr + module->GetFuncOffset();
    this->table = this->ptr + module->GetTableEnvironmentOffset();
    this->globals = this->ptr + module->GetGlobalOffset();

    if (globals < ptr ||
        (globals + WAsmJs::ConvertToJsVarOffset<byte>(module->GetGlobalsByteSize())) >(ptr + envSize))
    {
        // Something went wrong in the allocation and/or offset calculation failfast
        JavascriptError::ThrowOutOfMemoryError(scriptContext);
    }
}

} // namespace Js
#endif // ENABLE_WASM
