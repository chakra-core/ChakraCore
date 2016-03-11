//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmSignature::WasmSignature(ArenaAllocator * alloc) :
    m_alloc(alloc),
    m_resultType(WasmTypes::Void),
    m_id(Js::Constants::UninitializedValue),
    m_paramSize(Js::Constants::UninitializedValue)
{
    m_params = Anew(m_alloc, WasmTypeArray, m_alloc, 0);
}

void
WasmSignature::AddParam(WasmTypes::WasmType type)
{
    m_params->Add(Wasm::Local(type));
}

void
WasmSignature::SetResultType(WasmTypes::WasmType type)
{
    Assert(m_resultType == WasmTypes::Void);
    m_resultType = type;
}

void
WasmSignature::SetSignatureId(uint32 id)
{
    Assert(m_id == Js::Constants::UninitializedValue);
    m_id = id;
}

WasmTypes::WasmType
WasmSignature::GetParam(uint index) const
{
    if (index < m_params->Count())
    {
        return m_params->GetBuffer()[index].t;
    }
    return WasmTypes::Limit;
}

WasmTypes::WasmType
WasmSignature::GetResultType() const
{
    return m_resultType;
}

uint32
WasmSignature::GetParamCount() const
{
    return m_params->Count();
}

uint32
WasmSignature::GetSignatureId() const
{
    return m_id;
}

uint32
WasmSignature::GetParamSize() const
{
    if (m_paramSize != Js::Constants::UninitializedValue)
    {
        return m_paramSize;
    }

    uint32 m_paramSize = 0;
    for (uint32 i = 0; i < GetParamCount(); ++i)
    {
        switch (GetParam(i))
        {
        case WasmTypes::F32:
        case WasmTypes::I32:
            CompileAssert(sizeof(float) == sizeof(int32));
#ifdef _M_X64
            // on x64, we always alloc (at least) 8 bytes per arguments
            m_paramSize += sizeof(void*);
#elif _M_IX86
            m_paramSize += sizeof(int32);
#else
            Assert(UNREACHED);
#endif
            break;
        case WasmTypes::F64:
        case WasmTypes::I64:
            CompileAssert(sizeof(double) == sizeof(int64));
            m_paramSize += sizeof(int64);
            break;
        default:
            Assume(UNREACHED);
        }
    }

    return m_paramSize;
}

} // namespace Wasm

#endif // ENABLE_WASM
