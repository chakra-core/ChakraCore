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
    m_alloc(alloc)
{
    m_signatures = Anew(m_alloc, WasmSignatureArray, m_alloc, 0);
    m_indirectfuncs = Anew(m_alloc, WasmIndirectFuncArray, m_alloc, 0);
}

bool
ModuleInfo::InitializeMemory(size_t minSize, size_t maxSize, bool exported)
{
    if (m_memory.minSize != 0)
    {
        return false;
    }

    if (maxSize < minSize)
    {
        return false;
    }

    if (minSize == 0 || minSize % AutoSystemInfo::PageSize != 0)
    {
        return false;
    }

    if (maxSize % AutoSystemInfo::PageSize != 0)
    {
        return false;
    }

    m_memory.minSize = minSize;
    m_memory.maxSize = maxSize;
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

} // namespace Wasm

#endif // ENABLE_WASM
