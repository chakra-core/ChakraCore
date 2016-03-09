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
    m_exportCount(0)
{
    m_signatures = Anew(m_alloc, WasmSignatureArray, m_alloc, 0);
    m_indirectfuncs = Anew(m_alloc, WasmIndirectFuncArray, m_alloc, 0);
    m_funsigs = nullptr;
    m_exports = nullptr;
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

uint32
ModuleInfo::AddSignature(WasmSignature * signature)
{
    uint32 id = m_signatures->Count();

    signature->SetSignatureId(id);
    m_signatures->Add(signature);

    return id;
}

WasmSignature *
ModuleInfo::GetSignature(uint32 index) const
{
    if (index >= m_signatures->Count())
    {
        return nullptr;
    }

    return m_signatures->GetBuffer()[index];
}

uint32
ModuleInfo::GetSignatureCount() const
{
    return m_signatures->Count();
}

void
ModuleInfo::AddIndirectFunctionIndex(uint32 funcIndex)
{
    m_indirectfuncs->Add(funcIndex);
}

uint32
ModuleInfo::GetIndirectFunctionIndex(uint32 indirTableIndex) const
{
    if (indirTableIndex >= GetIndirectFunctionCount())
    {
        return Js::Constants::InvalidSourceIndex;
    }
    return m_indirectfuncs->GetBuffer()[indirTableIndex];
}

uint32
ModuleInfo::GetIndirectFunctionCount() const
{
    return m_indirectfuncs->Count();
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

void ModuleInfo::SetFunctionExport(uint32 iExport, uint32 funcIndex, wchar_t* exportName, uint32 nameLength)
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

} // namespace Wasm

#endif // ENABLE_WASM
