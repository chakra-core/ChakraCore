//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmSignature::WasmSignature() :
    m_id(Js::Constants::UninitializedValue),
    m_paramSize(Js::Constants::InvalidArgSlot),
    m_params(nullptr),
    m_paramsCount(0),
    m_shortSig(Js::Constants::InvalidSignature)
{
}

void WasmSignature::AllocateParams(Js::ArgSlot count, Recycler * recycler)
{
    if (GetParamCount() != 0)
    {
        throw WasmCompilationException(_u("Invalid call to WasmSignature::AllocateParams: Params are already allocated"));
    }
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


void WasmSignature::AllocateResults(uint32 count, Recycler * recycler)
{
    if (GetResultCount() != 0)
    {
        throw WasmCompilationException(_u("Invalid call to WasmSignature::AllocateResults: Results are already allocated"));
    }
    if (count > 0)
    {
        m_results = RecyclerNewArrayLeafZ(recycler, WasmTypes::WasmType, count);
    }
    m_resultsCount = count;
}

void WasmSignature::SetResult(Local type, uint32 index)
{
    if (index >= GetResultCount())
    {
        throw WasmCompilationException(_u("Result %u out of range (max %u)"), index, GetResultCount());
    }
    m_results[index] = type;
}

Wasm::Local WasmSignature::GetResult(uint32 index) const
{
    if (index >= GetResultCount())
    {
        throw WasmCompilationException(_u("Result %u out of range (max %u)"), index, GetResultCount());
    }
    return m_results[index];
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

template<bool useShortSig>
bool WasmSignature::IsEquivalent(const WasmSignature* sig) const
{
    if (useShortSig && m_shortSig != Js::Constants::InvalidSignature)
    {
        bool isEquivalent = sig->GetShortSig() == m_shortSig;
        Assert(this->IsEquivalent<false>(sig) == isEquivalent);
        return isEquivalent;
    }
    if (GetResultCount() == sig->GetResultCount() &&
        GetParamCount() == sig->GetParamCount() &&
        GetParamsSize() == sig->GetParamsSize())
    {
        return (
            (GetParamCount() == 0 || memcmp(m_params, sig->m_params, GetParamCount() * sizeof(Local)) == 0) &&
            (GetResultCount() == 0 || memcmp(m_results, sig->m_results, GetResultCount() * sizeof(Local)) == 0)
        );
    }
    return false;
}
template bool WasmSignature::IsEquivalent<true>(const WasmSignature*) const;
template bool WasmSignature::IsEquivalent<false>(const WasmSignature*) const;

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
#ifdef ENABLE_WASM_SIMD
    case WasmTypes::M128:
        Wasm::Simd::EnsureSimdIsEnabled();
        CompileAssert(sizeof(Simd::simdvec) == 16);
        return sizeof(Simd::simdvec);
        break;
#endif
    default:
        WasmTypes::CompileAssertCasesNoFailFast<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_M128_CHECK_TYPE>();
        throw WasmCompilationException(_u("Invalid param type"));
    }
}

void WasmSignature::FinalizeSignature()
{
    if (GetResultCount() > 1)
    {
        // Do not support short signature with multiple returns at this time
        return;
    }
    Local resultType = GetResultCount() == 1 ? GetResult(0) : WasmTypes::Void;
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

    // 3 bits for result type, 3 for each arg
    const uint32 nBitsForResult = 3;
#ifdef ENABLE_WASM_SIMD
    const uint32 nBitsForArgs = 3;
#else
    // We can drop 1 bit by excluding void
    const uint32 nBitsForArgs = 2;
#endif
    CompileAssert(Local::Void == 0);
    // Make sure we can encode all types (including void) with the number of bits reserved
    CompileAssert(Local::Limit <= (1 << nBitsForResult));
    // Make sure we can encode all types (excluding void) with the number of bits reserved
    CompileAssert(Local::Limit - 1 <= (1 << nBitsForArgs));

    ::Math::RecordOverflowPolicy sigOverflow;
    const uint32 bitsRequiredForSig = UInt32Math::MulAdd<nBitsForArgs, nBitsForResult>((uint32)paramCount, sigOverflow);
    if (sigOverflow.HasOverflowed())
    {
        return;
    }

    // we don't need to reserve a sentinel bit because there is no result type with value of 7
    CompileAssert(Local::Limit <= 0b111);
    const uint32 nAvailableBits = sizeof(m_shortSig) * 8;
    if (bitsRequiredForSig <= nAvailableBits)
    {
        // Append the result type to the signature
        m_shortSig = (m_shortSig << nBitsForResult) | resultType;
        for (Js::ArgSlot i = 0; i < paramCount; ++i)
        {
            // Append the param type to the signature, -1 to exclude Void
            m_shortSig = (m_shortSig << nBitsForArgs) | (m_params[i] - 1);
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
    CompileAssert(offsetof(Wasm::WasmSignature, m_id) == offsetof(WasmSignatureIDL, id));
    CompileAssert(offsetof(Wasm::WasmSignature, m_resultsCount) == offsetof(WasmSignatureIDL, resultsCount));
    CompileAssert(offsetof(Wasm::WasmSignature, m_paramSize) == offsetof(WasmSignatureIDL, paramSize));
    CompileAssert(offsetof(Wasm::WasmSignature, m_paramsCount) == offsetof(WasmSignatureIDL, paramsCount));
    CompileAssert(offsetof(Wasm::WasmSignature, m_shortSig) == offsetof(WasmSignatureIDL, shortSig));
    CompileAssert(offsetof(Wasm::WasmSignature, m_params) == offsetof(WasmSignatureIDL, params));
    CompileAssert(offsetof(Wasm::WasmSignature, m_results) == offsetof(WasmSignatureIDL, results));
    CompileAssert(sizeof(Local) == sizeof(int));

    return reinterpret_cast<WasmSignature*>(sig);
}

uint32 WasmSignature::WriteSignatureToString(_Out_writes_(maxlen) char16* out, uint32 maxlen)
{
    bool hasMultiResults = GetResultCount() > 1;
    const uint32 minEnd = GetResultCount() > 1 ? 20 : 12;
    AssertOrFailFast(maxlen > minEnd);
    AssertOrFailFast(out != nullptr);
    uint32 numwritten = 0;
#define WRITE_BUF(msg, ...) numwritten += _snwprintf_s(out+numwritten, maxlen-numwritten, _TRUNCATE, msg, ##__VA_ARGS__);
    WRITE_BUF(_u("("));
    for (Js::ArgSlot i = 0; i < GetParamCount(); i++)
    {
        if (i != 0)
        {
            WRITE_BUF(_u(", "));
        }
        WRITE_BUF(_u("%ls"), WasmTypes::GetTypeName(this->GetParam(i)));
    }
    if (numwritten >= maxlen - minEnd) {
        // null out the last 12 characters so we can properly end it 
        for (uint32 i = 1; i <= minEnd; i++)
        {
            *(out + maxlen - i) = 0;
        }
        if (numwritten < minEnd)
        {
            numwritten = 0;
        }
        else
        {
            numwritten -= minEnd;
        }

        WRITE_BUF(_u("..."));
        if (hasMultiResults)
        {
            // If the signature has multiple results, just print the first one then terminate.
            WRITE_BUF(_u(")->(%s, ...)"), WasmTypes::GetTypeName(this->GetResult(0)));
            return numwritten;
        }
    }
    if (GetResultCount() == 0)
    {
        WRITE_BUF(_u(")->void"));
    }
    else if (GetResultCount() == 1)
    {
        WRITE_BUF(_u(")->%ls"), WasmTypes::GetTypeName(this->GetResult(0)));
    }
    else
    {
        WRITE_BUF(_u(")->("));
        for (uint32 i = 0; i < GetResultCount(); i++)
        {
            if (i != 0)
            {
                WRITE_BUF(_u(", "));
            }
            WRITE_BUF(_u("%ls"), WasmTypes::GetTypeName(this->GetResult(i)));
        }
        WRITE_BUF(_u(")"));
    }
#undef WRITE_BUF
    return numwritten;
}

void WasmSignature::Dump(uint32 maxlen)
{
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    char16 buf[512] = { 0 };
    this->WriteSignatureToString(buf, min(maxlen, 512u));
    Output::Print(buf);
#endif
}

} // namespace Wasm

#endif // ENABLE_WASM
