//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
#include "../WasmReader/WasmReaderPch.h"
namespace Js
{

WebAssemblyEnvironment::WebAssemblyEnvironment(WebAssemblyModule* module):
    m_alloc(_u("WebAssemblyEnvironment"), module->GetScriptContext()->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory)
{
    this->module = module;
    ScriptContext* scriptContext = module->GetScriptContext();
    uint32 size = module->GetModuleEnvironmentSize();
    this->start = RecyclerNewArrayZ(scriptContext->GetRecycler(), Field(Var), size);
    this->end = start + size;
    Assert(start < end);
    this->memory = this->start + module->GetMemoryOffset();
    this->imports = this->start + module->GetImportFuncOffset();
    this->functions = this->start + module->GetFuncOffset();
    this->table = this->start + module->GetTableEnvironmentOffset();
    this->globals = this->start + module->GetGlobalOffset();

    uint32 globalsSize = WAsmJs::ConvertOffset<byte, Js::Var>(module->GetGlobalsByteSize());
    // Assumes globals are last
    Assert(globals > table && globals > functions && globals > imports && globals > memory);
    if (globals < start ||
        (globals + globalsSize) > end)
    {
        // Something went wrong in the allocation and/or offset calculation failfast
        JavascriptError::ThrowOutOfMemoryError(scriptContext);
    }
    AssertMsg(globals + globalsSize + 0x10 > end, "We don't expect to allocate much more memory than what's needed");

    elementSegmentOffsets = AnewArrayZ(&m_alloc, uint, module->GetElementSegCount());
    dataSegmentOffsets = AnewArrayZ(&m_alloc, uint, module->GetDataSegCount());
}

template<typename T>
void WebAssemblyEnvironment::CheckPtrIsValid(intptr_t ptr) const
{
    if (ptr < (intptr_t)PointerValue(start) || (intptr_t)(ptr + sizeof(T)) > (intptr_t)PointerValue(end))
    {
        Js::Throw::InternalError();
    }
}

template<typename T>
T* WebAssemblyEnvironment::GetVarElement(Field(Var)* ptr, uint32 index, uint32 maxCount) const
{
    if (index >= maxCount)
    {
        Js::Throw::InternalError();
    }

    Field(Var)* functionPtr = ptr + index;
    CheckPtrIsValid<T*>((intptr_t)functionPtr);
    Var varFunc = *functionPtr;
    if (varFunc)
    {
        if (!VarIs<T>(varFunc))
        {
            Js::Throw::InternalError();
        }
        return VarTo<T>(varFunc);
    }
    return nullptr;
}

template<typename T>
void WebAssemblyEnvironment::SetVarElement(Field(Var)* ptr, T* val, uint32 index, uint32 maxCount)
{
    if (index >= maxCount ||
        !VarIsCorrectType(val))
    {
        Js::Throw::InternalError();
    }

    Field(Var)* dst = ptr + index;
    CheckPtrIsValid<Var>((intptr_t)dst);
    AssertMsg(*dst == nullptr, "We shouldn't overwrite anything on the environment once it is set");
    *dst = val;
}

WasmScriptFunction* WebAssemblyEnvironment::GetWasmFunction(uint32 index) const
{
    if (!(module->GetFunctionIndexType(index) == Wasm::FunctionIndexTypes::Function ||
          module->GetFunctionIndexType(index) == Wasm::FunctionIndexTypes::ImportThunk))
    {
        Js::Throw::InternalError();
    }
    return GetVarElement<WasmScriptFunction>(functions, index, module->GetWasmFunctionCount());
}

void WebAssemblyEnvironment::SetWasmFunction(uint32 index, WasmScriptFunction* func)
{
    if (!(module->GetFunctionIndexType(index) == Wasm::FunctionIndexTypes::Function ||
          module->GetFunctionIndexType(index) == Wasm::FunctionIndexTypes::ImportThunk))
    {
        Js::Throw::InternalError();
    }
    SetVarElement<WasmScriptFunction>(functions, func, index, module->GetWasmFunctionCount());
}

void WebAssemblyEnvironment::SetImportedFunction(uint32 index, Var importedFunc)
{
    SetVarElement<JavascriptFunction>(imports, (JavascriptFunction*)importedFunc, index, module->GetWasmFunctionCount());
}

Js::WebAssemblyTable* WebAssemblyEnvironment::GetTable(uint32 index) const
{
    return GetVarElement<WebAssemblyTable>(table, index, 1);
}

void WebAssemblyEnvironment::SetTable(uint32 index, WebAssemblyTable* table)
{
    SetVarElement<WebAssemblyTable>(this->table, table, index, 1);
}

WebAssemblyMemory* WebAssemblyEnvironment::GetMemory(uint32 index) const
{
    return GetVarElement<WebAssemblyMemory>(memory, index, 1);
}

void WebAssemblyEnvironment::SetMemory(uint32 index, WebAssemblyMemory* mem)
{
    SetVarElement<WebAssemblyMemory>(this->memory, mem, index, 1);
}

template<typename T>
T WebAssemblyEnvironment::GetGlobalInternal(uint32 offset) const
{
    Field(T)* ptr = (Field(T)*)PointerValue(start) + offset;
    CheckPtrIsValid<T>((intptr_t)ptr);
    return *ptr;
}

template<typename T>
void WebAssemblyEnvironment::SetGlobalInternal(uint32 offset, T val)
{
    Field(T)* ptr = (Field(T)*)PointerValue(start) + offset;
    CheckPtrIsValid<T>((intptr_t)PointerValue(ptr));
    AssertMsg(*ptr == 0, "We shouldn't overwrite anything on the environment once it is set");
    *ptr = val;
}

Wasm::WasmConstLitNode WebAssemblyEnvironment::GetGlobalValue(Wasm::WasmGlobal* global) const
{
    AssertOrFailFast(global);
    Wasm::WasmConstLitNode cnst;
    uint32 offset = module->GetOffsetForGlobal(global);

    switch (global->GetType())
    {
    case Wasm::WasmTypes::I32: cnst.i32 = GetGlobalInternal<int>(offset); break;
    case Wasm::WasmTypes::I64: cnst.i64 = GetGlobalInternal<int64>(offset); break;
    case Wasm::WasmTypes::F32: cnst.f32 = GetGlobalInternal<float>(offset); break;
    case Wasm::WasmTypes::F64: cnst.f64 = GetGlobalInternal<double>(offset); break;
#ifdef ENABLE_WASM_SIMD
    case Wasm::WasmTypes::M128: AssertOrFailFastMsg(UNREACHED, "Wasm.Simd globals not supported");
#endif
    default:
        Wasm::WasmTypes::CompileAssertCases<Wasm::WasmTypes::I32, Wasm::WasmTypes::I64, Wasm::WasmTypes::F32, Wasm::WasmTypes::F64, WASM_M128_CHECK_TYPE>();
    }
    return cnst;
}

void WebAssemblyEnvironment::SetGlobalValue(Wasm::WasmGlobal* global, Wasm::WasmConstLitNode cnst)
{
    AssertOrFailFast(global);
    uint32 offset = module->GetOffsetForGlobal(global);

    switch (global->GetType())
    {
    case Wasm::WasmTypes::I32: SetGlobalInternal<int>(offset, cnst.i32); break;
    case Wasm::WasmTypes::I64: SetGlobalInternal<int64>(offset, cnst.i64); break;
    case Wasm::WasmTypes::F32: SetGlobalInternal<float>(offset, cnst.f32); break;
    case Wasm::WasmTypes::F64: SetGlobalInternal<double>(offset, cnst.f64); break;
#ifdef ENABLE_WASM_SIMD
    case Wasm::WasmTypes::M128: AssertOrFailFastMsg(UNREACHED, "Wasm.Simd globals not supported");
#endif
    default:
        Wasm::WasmTypes::CompileAssertCases<Wasm::WasmTypes::I32, Wasm::WasmTypes::I64, Wasm::WasmTypes::F32, Wasm::WasmTypes::F64, WASM_M128_CHECK_TYPE>();
    }
}

void WebAssemblyEnvironment::CalculateOffsets(WebAssemblyTable* table, WebAssemblyMemory* memory)
{
    DebugOnly(offsetInitialized = true);
    ScriptContext* scriptContext = module->GetScriptContext();
    if (!table || !memory)
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
    }

    int32 hCode = WASMERR_ElementSegOutOfRange;
    auto outOfRangeError = [scriptContext, &hCode] { JavascriptError::ThrowWebAssemblyLinkError(scriptContext, hCode); };

    for (uint elementsIndex = 0; elementsIndex < module->GetElementSegCount(); ++elementsIndex)
    {
        Wasm::WasmElementSegment* eSeg = module->GetElementSeg(elementsIndex);
        uint offset = module->GetOffsetFromInit(eSeg->GetOffsetExpr(), this);
        if (UInt32Math::Add(offset, eSeg->GetNumElements(), outOfRangeError) > table->GetCurrentLength())
        {
            outOfRangeError();
        }
        this->elementSegmentOffsets[elementsIndex] = offset;
    }

    ArrayBufferBase* buffer = memory->GetBuffer();
    Assert(!buffer->IsDetached());
    hCode = WASMERR_DataSegOutOfRange;
    for (uint32 iSeg = 0; iSeg < module->GetDataSegCount(); ++iSeg)
    {
        Wasm::WasmDataSegment* segment = module->GetDataSeg(iSeg);
        Assert(segment != nullptr);
        const uint32 offset = module->GetOffsetFromInit(segment->GetOffsetExpr(), this);
        const uint32 size = segment->GetSourceSize();

        if (UInt32Math::Add(offset, size, outOfRangeError) > buffer->GetByteLength())
        {
            outOfRangeError();
        }
        this->dataSegmentOffsets[iSeg] = offset;
    }
}

uint32 WebAssemblyEnvironment::GetElementSegmentOffset(uint32 index) const
{
    Assert(offsetInitialized);
    if (index >= module->GetElementSegCount())
    {
        AssertMsg(UNREACHED, "We should only be using valid indexes");
        JavascriptError::ThrowRangeError(module->GetScriptContext(), JSERR_ArgumentOutOfRange);
    }
    return elementSegmentOffsets[index];
}

uint32 WebAssemblyEnvironment::GetDataSegmentOffset(uint32 index) const
{
    Assert(offsetInitialized);
    if (index >= module->GetDataSegCount())
    {
        AssertMsg(UNREACHED, "We should only be using valid indexes");
        JavascriptError::ThrowRangeError(module->GetScriptContext(), JSERR_ArgumentOutOfRange);
    }
    return dataSegmentOffsets[index];
}

} // namespace Js
#endif // ENABLE_WASM
