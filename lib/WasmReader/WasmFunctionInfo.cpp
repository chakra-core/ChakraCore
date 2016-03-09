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
    m_imported(false),
    m_name(nullptr)
{
    m_i32Consts = Anew(m_alloc, ConstMap<int32>, m_alloc);
    m_i64Consts = Anew(m_alloc, ConstMap<int64>, m_alloc);
    m_f32Consts = Anew(m_alloc, ConstMap<float>, m_alloc);
    m_f64Consts = Anew(m_alloc, ConstMap<double>, m_alloc);
    m_locals = Anew(m_alloc, WasmTypeArray, m_alloc, 0);
}

void
WasmFunctionInfo::AddLocal(WasmTypes::WasmType type)
{
    m_locals->Add(type);
}

template<>
void
WasmFunctionInfo::AddConst<int32>(int32 constVal, Js::RegSlot reg)
{
    int result = m_i32Consts->Add(constVal, reg);
    Assert(result != -1); // REVIEW: should always succeed (or at least throw OOM)?
}

template<>
void
WasmFunctionInfo::AddConst<int64>(int64 constVal, Js::RegSlot reg)
{
    int result = m_i64Consts->Add(constVal, reg);
    Assert(result != -1);
}

template<>
void
WasmFunctionInfo::AddConst<float>(float constVal, Js::RegSlot reg)
{
    int result = m_f32Consts->Add(constVal, reg);
    Assert(result != -1);
}

template<>
void
WasmFunctionInfo::AddConst<double>(double constVal, Js::RegSlot reg)
{
    int result = m_f64Consts->Add(constVal, reg);
    Assert(result != -1);
}

WasmTypes::WasmType
WasmFunctionInfo::GetLocal(uint index) const
{
    if (index < m_locals->Count())
    {
        return m_locals->GetBuffer()[index];
    }
    return WasmTypes::Limit;
}

WasmTypes::WasmType
WasmFunctionInfo::GetParam(uint index) const
{
    return m_signature->GetParam(index);
}

template<>
Js::RegSlot
WasmFunctionInfo::GetConst<int32>(int32 constVal) const
{
    return m_i32Consts->Lookup(constVal, Js::Constants::NoRegister);
}

template<>
Js::RegSlot
WasmFunctionInfo::GetConst<int64>(int64 constVal) const
{
    return m_i64Consts->Lookup(constVal, Js::Constants::NoRegister);
}

template<>
Js::RegSlot
WasmFunctionInfo::GetConst<float>(float constVal) const
{
    return m_f32Consts->Lookup(constVal, Js::Constants::NoRegister);
}

template<>
Js::RegSlot
WasmFunctionInfo::GetConst<double>(double constVal) const
{
    return m_f64Consts->Lookup(constVal, Js::Constants::NoRegister);
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
WasmFunctionInfo::SetImported(const bool imported)
{
    m_imported = imported;
}

bool
WasmFunctionInfo::Imported() const
{
    return m_imported;
}

void
WasmFunctionInfo::SetName(LPCUTF8 name)
{
    m_name = name;
}

LPCUTF8
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
        m_locals->Add(signature->GetParam(i));
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

} // namespace Wasm
#endif // ENABLE_WASM
