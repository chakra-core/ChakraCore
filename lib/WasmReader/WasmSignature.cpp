//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmSignature::WasmSignature() :
    m_resultType(WasmTypes::Void),
    m_id(Js::Constants::UninitializedValue),
    m_paramSize(Js::Constants::InvalidArgSlot),
    m_params(nullptr),
    m_paramsCount(0),
    m_shortSig(Js::Constants::InvalidSignature)
{
}

void WasmSignature::AllocateParams(Js::ArgSlot count, Recycler * recycler)
{
    if (count > 0)
    {
        m_params = RecyclerNewArrayLeafZ(recycler, Local, count);
    }
    m_paramsCount = count;
}

void WasmSignature::SetParam(WasmTypes::WasmType type, Js::ArgSlot index)
{
    if (index >= GetParamCount())
    {
        throw WasmCompilationException(_u("Parameter %u out of range (max %u)"), index, GetParamCount());
    }
    m_params[index] = Local(type);
}

void WasmSignature::SetResultType(WasmTypes::WasmType type)
{
    Assert(m_resultType == WasmTypes::Void);
    m_resultType = type;
}

void WasmSignature::SetSignatureId(uint32 id)
{
    Assert(m_id == Js::Constants::UninitializedValue);
    m_id = id;
}

Local WasmSignature::GetParam(Js::ArgSlot index) const
{
    if (index >= GetParamCount())
    {
        throw WasmCompilationException(_u("Parameter %u out of range (max %u)"), index, GetParamCount());
    }
    return m_params[index];
}

WasmTypes::WasmType WasmSignature::GetResultType() const
{
    return m_resultType;
}

Js::ArgSlot WasmSignature::GetParamCount() const
{
    return m_paramsCount;
}

uint32 WasmSignature::GetSignatureId() const
{
    return m_id;
}

size_t WasmSignature::GetShortSig() const
{
    return m_shortSig;
}

bool WasmSignature::IsEquivalent(const WasmSignature* sig) const
{
    if (m_shortSig != Js::Constants::InvalidSignature)
    {
        return sig->GetShortSig() == m_shortSig;
    }
    if (GetResultType() == sig->GetResultType() &&
        GetParamCount() == sig->GetParamCount() &&
        GetParamsSize() == sig->GetParamsSize())
    {
        return GetParamCount() == 0 || memcmp(m_params, sig->m_params, GetParamCount() * sizeof(Local)) == 0;
    }
    return false;
}

Js::ArgSlot WasmSignature::GetParamSize(Js::ArgSlot index) const
{
    switch (GetParam(index))
    {
    case WasmTypes::F32:
    case WasmTypes::I32:
        CompileAssert(sizeof(float) == sizeof(int32));
#ifdef _M_X64
        // on x64, we always alloc (at least) 8 bytes per arguments
        return sizeof(void*);
#elif _M_IX86
        return sizeof(int32);
#else
        Assert(UNREACHED);
#endif
        break;
    case WasmTypes::F64:
    case WasmTypes::I64:
        CompileAssert(sizeof(double) == sizeof(int64));
        return sizeof(int64);
        break;
    default:
        throw WasmCompilationException(_u("Invalid param type"));
    }
}

void WasmSignature::FinalizeSignature()
{
    Assert(m_paramSize == Js::Constants::InvalidArgSlot);
    Assert(m_shortSig == Js::Constants::InvalidSignature);
    const Js::ArgSlot paramCount = GetParamCount();
    m_paramSize = 0;
    for (Js::ArgSlot i = 0; i < paramCount; ++i)
    {
        if (ArgSlotMath::Add(m_paramSize, GetParamSize(i), &m_paramSize))
        {
            throw WasmCompilationException(_u("Argument size too big"));
        }
    }

    CompileAssert(Local::Limit - 1 <= 4);
    CompileAssert(Local::Void == 0);

    // 3 bits for result type, 2 for each arg
    // we don't need to reserve a sentinel bit because there is no result type with value of 7
    uint32 sigSize = ((uint32)paramCount) * 2 + 3;
    if (sigSize <= sizeof(m_shortSig) << 3)
    {
        m_shortSig = (m_shortSig << 3) | m_resultType;
        for (Js::ArgSlot i = 0; i < paramCount; ++i)
        {
            // we can use 2 bits per arg by dropping void
            m_shortSig = (m_shortSig << 2) | (m_params[i] - 1);
        }
    }
}

Js::ArgSlot WasmSignature::GetParamsSize() const
{
    return m_paramSize;
}

WasmSignature* WasmSignature::FromIDL(WasmSignatureIDL* sig)
{
    // must update WasmSignatureIDL when changing WasmSignature
    CompileAssert(sizeof(Wasm::WasmSignature) == sizeof(WasmSignatureIDL));
    CompileAssert(offsetof(Wasm::WasmSignature, m_resultType) == offsetof(WasmSignatureIDL, resultType));
    CompileAssert(offsetof(Wasm::WasmSignature, m_id) == offsetof(WasmSignatureIDL, id));
    CompileAssert(offsetof(Wasm::WasmSignature, m_paramSize) == offsetof(WasmSignatureIDL, paramSize));
    CompileAssert(offsetof(Wasm::WasmSignature, m_paramsCount) == offsetof(WasmSignatureIDL, paramsCount));
    CompileAssert(offsetof(Wasm::WasmSignature, m_params) == offsetof(WasmSignatureIDL, params));
    CompileAssert(offsetof(Wasm::WasmSignature, m_shortSig) == offsetof(WasmSignatureIDL, shortSig));
    CompileAssert(sizeof(Local) == sizeof(int));

    return reinterpret_cast<WasmSignature*>(sig);
}

uint32 WasmSignature::WriteSignatureToString(_Out_writes_(maxlen) char16* out, uint32 maxlen)
{
    AssertOrFailFast(out != nullptr);
    uint32 numwritten = 0;
    numwritten += _snwprintf_s(out+numwritten, maxlen-numwritten, _TRUNCATE, _u("("));
    for (Js::ArgSlot i = 0; i < this->GetParamCount(); i++)
    {
        if (i != 0)
        {
            numwritten += _snwprintf_s(out+numwritten, maxlen-numwritten, _TRUNCATE, _u(", "));
        }
        numwritten += _snwprintf_s(out+numwritten, maxlen-numwritten, _TRUNCATE, _u("%ls"), WasmTypes::GetTypeName(this->GetParam(i)));
    }
    if (numwritten >= maxlen-12) {
        // null out the last 12 characters so we can properly end it 
        for (int i = 1; i <= 12; i++) {
            *(out + maxlen - i) = 0;
        }
        numwritten -= 12;
        numwritten += _snwprintf_s(out + numwritten, maxlen - numwritten, _TRUNCATE, _u("..."));
    }
    numwritten += _snwprintf_s(out + numwritten, maxlen - numwritten, _TRUNCATE, _u(")->%ls"), WasmTypes::GetTypeName(this->GetResultType()));
    return numwritten;
}

void WasmSignature::Dump()
{
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    char16 buf[512] = { 0 };
    this->WriteSignatureToString(buf, 512);
    Output::Print(buf);
#endif
}

} // namespace Wasm

#endif // ENABLE_WASM
