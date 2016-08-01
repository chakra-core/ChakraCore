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
    m_startFuncIndex(Js::Constants::UninitializedValue)
{
    m_reader = Anew(&m_alloc, WasmBinaryReader, &m_alloc, this, binaryBuffer, binaryBufferLength);
    m_reader->InitializeReader();
}

void
WasmModule::InitializeMemory(uint32 minPage, uint32 maxPage, bool exported)
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
    m_memory.exported = exported;
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
        return nullptr;
    }

    return m_signatures[index];
}

uint32
WasmModule::GetSignatureCount() const
{
    return m_signaturesCount;
}

void
WasmModule::AllocateIndirectFunctions(uint32 entries)
{
    m_indirectFuncCount = entries;
    m_indirectfuncs = AnewArray(&m_alloc, uint32, entries);
}

void
WasmModule::SetIndirectFunction(uint32 funcIndex, uint32 indirTableIndex)
{
    if (indirTableIndex < GetIndirectFunctionCount())
    {
        m_indirectfuncs[indirTableIndex] = funcIndex;
    }
}

uint32
WasmModule::GetIndirectFunctionIndex(uint32 indirTableIndex) const
{
    if (indirTableIndex >= GetIndirectFunctionCount())
    {
        return Js::Constants::InvalidSourceIndex;
    }
    return m_indirectfuncs[indirTableIndex];
}

uint32
WasmModule::GetIndirectFunctionCount() const
{
    return m_indirectFuncCount;
}

uint32
WasmModule::GetFunctionCount() const
{
    return m_funcCount;
}

void WasmModule::AllocateFunctions(uint32 count)
{
    m_funcCount = count;
    if (count > 0)
    {
        m_functionsInfo = AnewArray(&m_alloc, WasmFunctionInfo*, count);
    }
}

bool
WasmModule::SetFunctionInfo(WasmFunctionInfo* funcInfo, uint32 index)
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
WasmModule::GetFunctionInfo(uint index) const
{
    if (index >= m_funcCount)
    {
        return nullptr;
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
        return nullptr;
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
    size = UInt32Math::Add(size, GetFunctionCount());
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
