//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

ModuleInfo::ModuleInfo(ArenaAllocator * alloc) :
    m_memory(),
    m_alloc(alloc),
    m_funsigs(nullptr),
    m_funcCount(0),
    m_importCount(0),
    m_indirectfuncs(nullptr),
    m_indirectFuncCount(0),
    m_exports(nullptr),
    m_exportCount(0),
    m_datasegCount(0),
    m_signatures(nullptr),
    m_signaturesCount(0),
    m_startFunc(Js::Constants::UninitializedValue)
{
}

bool
ModuleInfo::InitializeMemory(uint32 minPage, uint32 maxPage, bool exported)
{
    if (m_memory.minSize != 0)
    {
        return false;
    }

    if (maxPage < minPage)
    {
        return false;
    }

    CompileAssert(Memory::PAGE_SIZE < INT_MAX);
    m_memory.minSize = (uint64)minPage * Memory::PAGE_SIZE;
    m_memory.maxSize = (uint64)maxPage * Memory::PAGE_SIZE;
    m_memory.exported = exported;

    return true;
}

const ModuleInfo::Memory *
ModuleInfo::GetMemory() const
{
    return &m_memory;
}

void
ModuleInfo::SetSignature(uint32 index, WasmSignature * signature)
{
    Assert(index < GetSignatureCount());
    signature->SetSignatureId(index);
    m_signatures[index] = signature;
}

WasmSignature *
ModuleInfo::GetSignature(uint32 index) const
{
    if (index >= GetSignatureCount())
    {
        return nullptr;
    }

    return m_signatures[index];
}

uint32
ModuleInfo::GetSignatureCount() const
{
    return m_signaturesCount;
}

void
ModuleInfo::AllocateIndirectFunctions(uint32 entries)
{
    m_indirectFuncCount = entries;
    m_indirectfuncs = AnewArray(m_alloc, uint32, entries);
}

void
ModuleInfo::SetIndirectFunction(uint32 funcIndex, uint32 indirTableIndex)
{
    if (indirTableIndex < GetIndirectFunctionCount())
    {
        m_indirectfuncs[indirTableIndex] = funcIndex;
    }
}

uint32
ModuleInfo::GetIndirectFunctionIndex(uint32 indirTableIndex) const
{
    if (indirTableIndex >= GetIndirectFunctionCount())
    {
        return Js::Constants::InvalidSourceIndex;
    }
    return m_indirectfuncs[indirTableIndex];
}

uint32
ModuleInfo::GetIndirectFunctionCount() const
{
    return m_indirectFuncCount;
}

void
ModuleInfo::SetFunctionCount(uint count)
{
    m_funcCount = count;
}

uint32
ModuleInfo::GetFunctionCount() const
{
    return m_funcCount;
}

void ModuleInfo::AllocateFunctions(uint32 count)
{
    m_funcCount = count;
    if (count > 0)
    {
        m_funsigs = AnewArray(m_alloc, WasmFunctionInfo*, count);
    }
}

bool
ModuleInfo::SetFunSig(WasmFunctionInfo* funsig, uint32 index)
{
    if (index < m_funcCount)
    {
        m_funsigs[index] = funsig;
        funsig->SetNumber(index);
        return true;
    }
    return false;
}

WasmFunctionInfo*
ModuleInfo::GetFunSig(uint index) const
{
    if (index >= m_funcCount)
    {
        return nullptr;
    }
    return m_funsigs[index];
}

void ModuleInfo::AllocateFunctionExports(uint32 entries)
{
    m_exports = AnewArrayZ(m_alloc, WasmExport, entries);
    m_exportCount = entries;
}

void ModuleInfo::SetFunctionExport(uint32 iExport, uint32 funcIndex, char16* exportName, uint32 nameLength)
{
    m_exports[iExport].funcIndex = funcIndex;
    m_exports[iExport].nameLength = nameLength;
    m_exports[iExport].name = exportName;
}

Wasm::WasmExport* ModuleInfo::GetFunctionExport(uint32 iExport) const
{
    if (iExport >= m_exportCount)
    {
        return nullptr;
    }
    return &m_exports[iExport];
}

void
ModuleInfo::AllocateFunctionImports(uint32 entries)
{
    m_imports = AnewArrayZ(m_alloc, WasmImport, entries);
    m_importCount = entries;
}

void
ModuleInfo::SetFunctionImport(uint32 i, uint32 sigId, char16* modName, uint32 modNameLen, char16* fnName, uint32 fnNameLen)
{
    m_imports[i].sigId = sigId;
    m_imports[i].modNameLen = modNameLen;
    m_imports[i].modName = modName;
    m_imports[i].fnNameLen = fnNameLen;
    m_imports[i].fnName = fnName;
}

Wasm::WasmImport*
ModuleInfo::GetFunctionImport(uint32 i) const
{
    if (i >= m_importCount)
    {
        return nullptr;
    }
    return &m_imports[i];
}

void
ModuleInfo::AllocateDataSegs(uint32 count)
{
    Assert(count != 0);
    m_datasegCount = count;
    m_datasegs = AnewArray(m_alloc, WasmDataSegment*, count);
}

bool
ModuleInfo::AddDataSeg(WasmDataSegment* seg, uint32 index)
{
    if (index >= m_datasegCount)
    {
        return false;
    }
    m_datasegs[index] = seg;
    return true;
}

WasmDataSegment*
ModuleInfo::GetDataSeg(uint32 index) const
{
    if (index >= m_datasegCount)
    {
        return nullptr;
    }
    return m_datasegs[index];
}

void
ModuleInfo::SetStartFunction(uint32 i)
{
    if (i >= m_funcCount) {
        TRACE_WASM_DECODER(L"Invalid start function index");
        return;
    }
    m_startFunc = i;
}

uint32
ModuleInfo::GetStartFunction() const
{
    return m_startFunc;
}

void ModuleInfo::SetSignatureCount(uint32 count)
{
    Assert(m_signaturesCount == 0 && m_signatures == nullptr);
    m_signaturesCount = count;
    m_signatures = AnewArray(m_alloc, WasmSignature*, count);
}

} // namespace Wasm

#endif // ENABLE_WASM
