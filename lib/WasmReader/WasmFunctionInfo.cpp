//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmFunctionInfo::WasmFunctionInfo(ArenaAllocator * alloc, WasmSignature* signature, uint32 number) : 
    m_alloc(alloc),
    m_signature(signature),
    m_body(nullptr),
    m_name(nullptr),
    m_customReader(nullptr),
    m_nameLength(0),
    m_number(number),
    m_locals(alloc, signature->GetParamCount())
#if DBG_DUMP
    , importedFunctionReference(nullptr)
#endif
{
    for (uint32 i = 0; i < signature->GetParamCount(); ++i)
    {
        m_locals.Add(Wasm::Local(signature->GetParam(i)));
    }
}

void
WasmFunctionInfo::AddLocal(WasmTypes::WasmType type, uint count)
{
    for (uint i = 0; i < count; ++i)
    {
        m_locals.Add(Wasm::Local(type));
    }
}

Local
WasmFunctionInfo::GetLocal(uint index) const
{
    if (index < GetLocalCount())
    {
        return m_locals.ItemInBuffer(index);
    }
    return WasmTypes::Limit;
}

Local
WasmFunctionInfo::GetParam(uint index) const
{
    return m_signature->GetParam(index);
}

WasmTypes::WasmType
WasmFunctionInfo::GetResultType() const
{
    return m_signature->GetResultType();
}

uint32
WasmFunctionInfo::GetLocalCount() const
{
    return m_locals.Count();
}

uint32
WasmFunctionInfo::GetParamCount() const
{
    return m_signature->GetParamCount();
}


} // namespace Wasm
#endif // ENABLE_WASM
