//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmFunctionInfo::WasmFunctionInfo(ArenaAllocator * alloc)
    : m_alloc(alloc),
    m_name(nullptr)
{
    m_locals = Anew(m_alloc, WasmTypeArray, m_alloc, 0);
}

void
WasmFunctionInfo::AddLocal(WasmTypes::WasmType type, uint count)
{
    for (uint i = 0; i < count; ++i)
    {
        m_locals->Add(Wasm::Local(type));
    }
}

WasmTypes::WasmType
WasmFunctionInfo::GetLocal(uint index) const
{
    if (index < m_locals->Count())
    {
        return m_locals->GetBuffer()[index].t;
    }
    return WasmTypes::Limit;
}

WasmTypes::WasmType
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
    return m_locals->Count();
}

uint32
WasmFunctionInfo::GetParamCount() const
{
    return m_signature->GetParamCount();
}

void
WasmFunctionInfo::SetName(char16* name)
{
    m_name = name;
}

char16*
WasmFunctionInfo::GetName() const
{
    return m_name;
}

void
WasmFunctionInfo::SetNumber(UINT32 number)
{
    m_number = number;
}

UINT32
WasmFunctionInfo::GetNumber() const
{
    return m_number;
}

void
WasmFunctionInfo::SetSignature(WasmSignature * signature)
{
    for (uint32 i = 0; i < signature->GetParamCount(); ++i)
    {
        m_locals->Add(Wasm::Local(signature->GetParam(i)));
    }

    m_signature = signature;
}

WasmSignature *
WasmFunctionInfo::GetSignature() const
{
    return m_signature;
}

void
WasmFunctionInfo::SetExitLabel(Js::ByteCodeLabel label)
{
    m_ExitLabel = label;
}

Js::ByteCodeLabel
WasmFunctionInfo::GetExitLabel() const
{
    return m_ExitLabel;
}

void
WasmFunctionInfo::SetLocalName(uint i, char16* n)
{
    Assert(i < GetLocalCount());
    m_locals->GetBuffer()[i].name = n;
}

char16*
WasmFunctionInfo::GetLocalName(uint i)
{
    Assert(i < GetLocalCount());
    return m_locals->GetBuffer()[i].name;
}

} // namespace Wasm
#endif // ENABLE_WASM
