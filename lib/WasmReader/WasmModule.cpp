//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmModule::WasmModule(Js::ScriptContext* scriptContext, byte* binaryBuffer, uint binaryBufferLength) :
    m_memory(),
    m_alloc(_u("WasmModule"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory),
    m_functionsInfo(nullptr),
    m_funcCount(0),
    m_importCount(0),
    m_indirectfuncs(nullptr),
    m_indirectFuncCount(0),
    m_exports(nullptr),
    m_exportCount(0),
    m_datasegCount(0),
    m_signatures(nullptr),
    m_signaturesCount(0),
    m_startFuncIndex(Js::Constants::UninitializedValue),
    indirFuncTableOffset(0),
    heapOffset(0),
    funcOffset(0),
    importFuncOffset(0)
{
    m_reader = Anew(&m_alloc, WasmBinaryReader, &m_alloc, this, binaryBuffer, binaryBufferLength);
    m_reader->InitializeReader();
}

uint32
WasmModule::GetMaxFunctionIndex() const
{
    return UInt32Math::Add(GetImportCount(), GetWasmFunctionCount());
}

Wasm::WasmSignature*
WasmModule::GetFunctionSignature(uint32 funcIndex) const
{
    FunctionIndexTypes::Type funcType = GetFunctionIndexType(funcIndex);
    if (funcType == FunctionIndexTypes::Invalid)
    {
        throw WasmCompilationException(_u("Function index out of range"));
    }

    funcIndex = NormalizeFunctionIndex(funcIndex);
    switch (funcType)
    {
    case FunctionIndexTypes::Import:
        return GetSignature(GetFunctionImport(funcIndex)->sigId);
    case FunctionIndexTypes::Function:
        return GetWasmFunctionInfo(funcIndex)->GetSignature();
    default:
        throw WasmCompilationException(_u("Unknown function index type"));
    }
}

FunctionIndexTypes::Type
WasmModule::GetFunctionIndexType(uint32 funcIndex) const
{
    if (funcIndex >= GetMaxFunctionIndex())
    {
        return FunctionIndexTypes::Invalid;
    }
    if (funcIndex < GetImportCount())
    {
        return FunctionIndexTypes::Import;
    }
    return FunctionIndexTypes::Function;
}

uint32
WasmModule::NormalizeFunctionIndex(uint32 funcIndex) const
{
    switch (GetFunctionIndexType(funcIndex))
    {
    case FunctionIndexTypes::Import:
        return funcIndex;
    case FunctionIndexTypes::Function:
        Assert(funcIndex >= GetImportCount());
        return funcIndex - GetImportCount();
    default:
        throw WasmCompilationException(_u("Unable to normalize function index"));
    }
}

void
WasmModule::InitializeMemory(uint32 minPage, uint32 maxPage)
{
    if (m_memory.minSize != 0)
    {
        throw WasmCompilationException(_u("Memory already allocated"));
    }

    if (maxPage < minPage)
    {
        throw WasmCompilationException(_u("Memory: MaxPage (%d) must be greater than MinPage (%d)"), maxPage, minPage);
    }

    CompileAssert(Memory::PAGE_SIZE < INT_MAX);
    m_memory.minSize = (uint64)minPage * Memory::PAGE_SIZE;
    m_memory.maxSize = (uint64)maxPage * Memory::PAGE_SIZE;
}

const WasmModule::Memory *
WasmModule::GetMemory() const
{
    return &m_memory;
}

void
WasmModule::SetSignature(uint32 index, WasmSignature * signature)
{
    Assert(index < GetSignatureCount());
    signature->SetSignatureId(index);
    m_signatures[index] = signature;
}

WasmSignature *
WasmModule::GetSignature(uint32 index) const
{
    if (index >= GetSignatureCount())
    {
        throw WasmCompilationException(_u("Invalid signature index %u"), index);
    }

    return m_signatures[index];
}

uint32
WasmModule::GetSignatureCount() const
{
    return m_signaturesCount;
}

void
WasmModule::AllocateTable(uint32 entries)
{
    m_indirectFuncCount = entries;
    m_indirectfuncs = AnewArray(&m_alloc, uint32, entries);
    for (uint32 i = 0; i < entries ; i++)
    {
        // Initialize with invalid index
        m_indirectfuncs[i] = Js::Constants::UninitializedValue;
    }
}

void
WasmModule::SetTableValue(uint32 funcIndex, uint32 tableIndex)
{
    if (tableIndex < GetTableSize())
    {
        m_indirectfuncs[tableIndex] = funcIndex;
    }
}

uint32
WasmModule::GetTableValue(uint32 tableIndex) const
{
    if (tableIndex >= GetTableSize())
    {
        return Js::Constants::InvalidSourceIndex;
    }
    return m_indirectfuncs[tableIndex];
}

uint32
WasmModule::GetTableSize() const
{
    return m_indirectFuncCount;
}

uint32
WasmModule::GetWasmFunctionCount() const
{
    return m_funcCount;
}

void WasmModule::AllocateWasmFunctions(uint32 count)
{
    m_funcCount = count;
    if (count > 0)
    {
        m_functionsInfo = AnewArray(&m_alloc, WasmFunctionInfo*, count);
    }
}

bool
WasmModule::SetWasmFunctionInfo(WasmFunctionInfo* funcInfo, uint32 index)
{
    if (index < m_funcCount)
    {
        m_functionsInfo[index] = funcInfo;
        Assert(funcInfo->GetNumber() == index);
        return true;
    }
    return false;
}

WasmFunctionInfo*
WasmModule::GetWasmFunctionInfo(uint index) const
{
    if (index >= m_funcCount)
    {
        throw WasmCompilationException(_u("Invalid function index %u"), index);
    }

    return m_functionsInfo[index];
}

void WasmModule::AllocateFunctionExports(uint32 entries)
{
    m_exports = AnewArrayZ(&m_alloc, WasmExport, entries);
    m_exportCount = entries;
}

void WasmModule::SetFunctionExport(uint32 iExport, uint32 funcIndex, char16* exportName, uint32 nameLength)
{
    m_exports[iExport].funcIndex = funcIndex;
    m_exports[iExport].nameLength = nameLength;
    m_exports[iExport].name = exportName;
}

Wasm::WasmExport* WasmModule::GetFunctionExport(uint32 iExport) const
{
    if (iExport >= m_exportCount)
    {
        return nullptr;
    }
    return &m_exports[iExport];
}

void
WasmModule::AllocateFunctionImports(uint32 entries)
{
    m_imports = AnewArrayZ(&m_alloc, WasmImport, entries);
    m_importCount = entries;
}

void
WasmModule::SetFunctionImport(uint32 i, uint32 sigId, char16* modName, uint32 modNameLen, char16* fnName, uint32 fnNameLen)
{
    m_imports[i].sigId = sigId;
    m_imports[i].modNameLen = modNameLen;
    m_imports[i].modName = modName;
    m_imports[i].fnNameLen = fnNameLen;
    m_imports[i].fnName = fnName;
}

Wasm::WasmImport*
WasmModule::GetFunctionImport(uint32 i) const
{
    if (i >= m_importCount)
    {
        throw WasmCompilationException(_u("Import function index out of range"));
    }
    return &m_imports[i];
}

void
WasmModule::AllocateDataSegs(uint32 count)
{
    Assert(count != 0);
    m_datasegCount = count;
    m_datasegs = AnewArray(&m_alloc, WasmDataSegment*, count);
}

bool
WasmModule::AddDataSeg(WasmDataSegment* seg, uint32 index)
{
    if (index >= m_datasegCount)
    {
        return false;
    }
    m_datasegs[index] = seg;
    return true;
}

WasmDataSegment*
WasmModule::GetDataSeg(uint32 index) const
{
    if (index >= m_datasegCount)
    {
        return nullptr;
    }
    return m_datasegs[index];
}

void
WasmModule::SetStartFunction(uint32 i)
{
    if (i >= m_funcCount) {
        TRACE_WASM_DECODER(L"Invalid start function index");
        return;
    }
    m_startFuncIndex = i;
}

uint32
WasmModule::GetStartFunction() const
{
    return m_startFuncIndex;
}

void WasmModule::SetSignatureCount(uint32 count)
{
    Assert(m_signaturesCount == 0 && m_signatures == nullptr);
    m_signaturesCount = count;
    m_signatures = AnewArray(&m_alloc, WasmSignature*, count);
}

uint32 WasmModule::GetModuleEnvironmentSize() const
{
    // 1 for the heap
    uint32 size = 1;
    size = UInt32Math::Add(size, GetWasmFunctionCount());
    // reserve space for as many function tables as there are signatures, though we won't fill them all
    size = UInt32Math::Add(size, GetSignatureCount());
    size = UInt32Math::Add(size, GetImportCount());
    return size;
}

void WasmModule::Finalize(bool isShutdown)
{
    m_alloc.Clear();
}

void WasmModule::Dispose(bool isShutdown)
{
    Assert(m_alloc.Size() == 0);
}

void WasmModule::Mark(Recycler * recycler)
{
}

} // namespace Wasm

#endif // ENABLE_WASM
