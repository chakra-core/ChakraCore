//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

using namespace Wasm;

template<typename T> 
inline T WasmMath::Shl( T aLeft, T aRight )
{
    return aLeft << (aRight & (sizeof(T)*8-1));
}
template<typename T> 
inline T WasmMath::Shr( T aLeft, T aRight )
{
    return aLeft >> (aRight & (sizeof(T)*8-1));
}

template<typename T> 
inline T WasmMath::ShrU( T aLeft, T aRight )
{
    return aLeft >> (aRight & (sizeof(T)*8-1));
}
template<> 
inline int WasmMath::Ctz(int value)
{
    DWORD index;
    if (_BitScanForward(&index, value))
    {
        return index;
    }
    return 32;
}

template<> 
inline int64 WasmMath::Ctz(int64 value)
{
    DWORD index;
#if TARGET_64
    if (_BitScanForward64(&index, value))
    {
        return index;
    }
#else
    if (_BitScanForward(&index, (int32)value))
    {
        return index;
    }
    if (_BitScanForward(&index, (int32)(value >> 32)))
    {
        return index + 32;
    }
#endif
    return 64;
}

template<> 
inline int64 WasmMath::Clz(int64 value)
{
    DWORD index;
#if TARGET_64
    if (_BitScanReverse64(&index, value))
    {
        return 63 - index;
    }
#else
    if (_BitScanReverse(&index, (int32)(value >> 32)))
    {
        return 31 - index;
    }
    if (_BitScanReverse(&index, (int32)value))
    {
        return 63 - index;
    }
#endif
    return 64;
}

template<> 
inline int WasmMath::PopCnt(int value)
{
    return ::Math::PopCnt32(value);
}

template<> 
inline int64 WasmMath::PopCnt(int64 value)
{
    uint64 v = (uint64)value;
    // https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    v = v - ((v >> 1) & 0x5555555555555555LL);
    v = (v & 0x3333333333333333LL) + ((v >> 2) & 0x3333333333333333LL);
    v = (v + (v >> 4)) & 0x0f0f0f0f0f0f0f0f;
    v = (uint64)(v * 0x0101010101010101LL) >> (sizeof(uint64) - 1) * CHAR_BIT;
    return (int64)v;
}


template<typename T>
inline int WasmMath::Eqz(T value)
{
    return value == 0;
}

template<>
inline double WasmMath::Copysign(double aLeft, double aRight)
{
    uint64 aLeftI64 = *(uint64*)(&aLeft);
    uint64 aRightI64 = *(uint64*)(&aRight);
    uint64 res = ((aLeftI64 & 0x7fffffffffffffffull) | (aRightI64 & 0x8000000000000000ull));
    return *(double*)(&res);
}

template<>
inline float WasmMath::Copysign(float aLeft, float aRight)
{
    uint32 aLeftI32 = *(uint32*)(&aLeft);
    uint32 aRightI32 = *(uint32*)(&aRight);
    uint32 res = ((aLeftI32 & 0x7fffffffu) | (aRightI32 & 0x80000000u));
    return *(float*)(&res);
}

template <typename T> bool WasmMath::LessThan(T aLeft, T aRight)
{
    return aLeft < aRight;
}

template <typename T> bool WasmMath::LessOrEqual(T aLeft, T aRight)
{
    return aLeft <= aRight;
}

template <
    typename SrcType,
    typename DstType,
    typename ReinterpretType,
    ReinterpretType Max,
    ReinterpretType NegZero,
    ReinterpretType NegOne,
    WasmMath::CmpPtr<ReinterpretType> MaxCmp,
    WasmMath::CmpPtr<ReinterpretType> NegOneCmp,
    bool Saturate,
    DstType MinResult,
    DstType MaxResult>
DstType WasmMath::ConvertFloatToInt(SrcType srcVal, _In_ Js::ScriptContext * scriptContext)
{
    CompileAssert(sizeof(SrcType) == sizeof(ReinterpretType));

    if (IsNaN(srcVal))
    {
        if (!Saturate)
        {
            Js::JavascriptError::ThrowWebAssemblyRuntimeError(scriptContext, VBSERR_Overflow);
        }
        return 0;
    }

    ReinterpretType val = *reinterpret_cast<ReinterpretType*> (&srcVal);
    if (MaxCmp(val, Max) || (val >= NegZero && NegOneCmp(val, NegOne)))
    {
        return static_cast<DstType>(srcVal);
    }
    if (!Saturate)
    {
        Js::JavascriptError::ThrowWebAssemblyRuntimeError(scriptContext, VBSERR_Overflow);
    }
    return (srcVal < 0) ? MinResult : MaxResult;
}

template <typename STYPE> bool  WasmMath::IsNaN(STYPE src)
{
    return src != src;
}

template<typename T>
inline T WasmMath::Trunc(T value)
{
    if (value == 0.0)
    {
        return value;
    }
    else
    {
        T result;
        if (value < 0.0)
        {
            result = ceil(value);
        }
        else
        {
            result = floor(value);
        }
        // TODO: Propagating NaN sign for now awaiting consensus on semantics
        return result;
    }
}

template<typename T>
inline T WasmMath::Nearest(T value)
{
    if (value == 0.0)
    {
        return value;
    }
    else
    {
        T result;
        T u = ceil(value);
        T d = floor(value);
        T um = fabs(value - u);
        T dm = fabs(value - d);
        if (um < dm || (um == dm && floor(u / 2) == u / 2))
        {
            result = u;
        }
        else
        {
            result = d;
        }
        // TODO: Propagating NaN sign for now awaiting consensus on semantics
        return result;
    }
}

template<>
inline int WasmMath::Rol(int aLeft, int aRight)
{
    return _rotl(aLeft, aRight);
}

template<>
inline int64 WasmMath::Rol(int64 aLeft, int64 aRight)
{
    return _rotl64(aLeft, (int)aRight);
}

template<>
inline int WasmMath::Ror(int aLeft, int aRight)
{
    return _rotr(aLeft, aRight);
}

template<>
inline int64 WasmMath::Ror(int64 aLeft, int64 aRight)
{
    return _rotr64(aLeft, (int)aRight);
}

template<typename To, typename From>
To WasmMath::SignExtend(To value)
{
    return static_cast<To>(static_cast<From>(value));
}

template <bool Saturate>
int32 WasmMath::F32ToI32(float src, _In_ Js::ScriptContext* scriptContext)
{
    return WasmMath::ConvertFloatToInt<
        float, // SrcType
        int32, // DstType
        uint32, // ReinterpretType
        Js::NumberConstants::k_Float32TwoTo31,
        Js::NumberConstants::k_Float32NegZero,
        Js::NumberConstants::k_Float32NegTwoTo31,
        &WasmMath::LessThan<uint32>,
        &WasmMath::LessOrEqual<uint32>,
        Saturate,
        INT32_MIN,
        INT32_MAX>(
            src,
            scriptContext);
}

template <bool Saturate>
uint32 WasmMath::F32ToU32(float src, _In_ Js::ScriptContext* scriptContext)
{
    return WasmMath::ConvertFloatToInt<
        float, // SrcType
        uint32, // DstType
        uint32, // ReinterpretType
        Js::NumberConstants::k_Float32TwoTo32,
        Js::NumberConstants::k_Float32NegZero,
        Js::NumberConstants::k_Float32NegOne,
        &WasmMath::LessThan<uint32>,
        &WasmMath::LessThan<uint32>,
        Saturate,
        0,
        UINT32_MAX>(
            src,
            scriptContext);
}

template <bool Saturate>
int32 WasmMath::F64ToI32(double src, _In_ Js::ScriptContext* scriptContext)
{
    return WasmMath::ConvertFloatToInt<
        double, // SrcType
        int32, // DstType
        uint64, // ReinterpretType
        Js::NumberConstants::k_TwoTo31,
        Js::NumberConstants::k_NegZero,
        Js::NumberConstants::k_NegTwoTo31,
        &WasmMath::LessOrEqual<uint64>,
        &WasmMath::LessOrEqual<uint64>,
        Saturate,
        INT32_MIN,
        INT32_MAX>(
            src,
            scriptContext);
}

template <bool Saturate>
uint32 WasmMath::F64ToU32(double src, _In_ Js::ScriptContext* scriptContext)
{
    return WasmMath::ConvertFloatToInt<
        double, // SrcType
        uint32, // DstType
        uint64, // ReinterpretType
        Js::NumberConstants::k_TwoTo32,
        Js::NumberConstants::k_NegZero,
        Js::NumberConstants::k_NegOne,
        &WasmMath::LessOrEqual<uint64>,
        &WasmMath::LessThan<uint64>,
        Saturate,
        0,
        UINT32_MAX>(
            src,
            scriptContext);
}

template <bool Saturate>
int64 WasmMath::F32ToI64(float src, _In_ Js::ScriptContext* scriptContext)
{
    return WasmMath::ConvertFloatToInt<
        float, // SrcType
        int64, // DstType
        uint32, // ReinterpretType
        Js::NumberConstants::k_Float32TwoTo63,
        Js::NumberConstants::k_Float32NegZero,
        Js::NumberConstants::k_Float32NegTwoTo63,
        &WasmMath::LessThan<uint32>,
        &WasmMath::LessOrEqual<uint32>,
        Saturate,
        INT64_MIN,
        INT64_MAX>(
            src,
            scriptContext);
}

template <bool Saturate>
uint64 WasmMath::F32ToU64(float src, _In_ Js::ScriptContext* scriptContext)
{
    return WasmMath::ConvertFloatToInt<
        float, // SrcType
        uint64, // DstType
        uint32, // ReinterpretType
        Js::NumberConstants::k_Float32TwoTo64,
        Js::NumberConstants::k_Float32NegZero,
        Js::NumberConstants::k_Float32NegOne,
        &WasmMath::LessThan<uint32>,
        &WasmMath::LessThan<uint32>,
        Saturate,
        0,
        UINT64_MAX>(
            src,
            scriptContext);
}

template <bool Saturate>
int64 WasmMath::F64ToI64(double src, _In_ Js::ScriptContext* scriptContext)
{
    return WasmMath::ConvertFloatToInt<
        double, // SrcType
        int64, // DstType
        uint64, // ReinterpretType
        Js::NumberConstants::k_TwoTo63,
        Js::NumberConstants::k_NegZero,
        Js::NumberConstants::k_NegTwoTo63,
        &WasmMath::LessThan<uint64>,
        &WasmMath::LessOrEqual<uint64>,
        Saturate,
        INT64_MIN,
        INT64_MAX>(
            src,
            scriptContext);
}

template <bool Saturate>
uint64 WasmMath::F64ToU64(double src, _In_ Js::ScriptContext* scriptContext)
{
    return WasmMath::ConvertFloatToInt<
        double, // SrcType
        uint64, // DstType
        uint64, // ReinterpretType
        Js::NumberConstants::k_TwoTo64,
        Js::NumberConstants::k_NegZero,
        Js::NumberConstants::k_NegOne,
        &WasmMath::LessThan<uint64>,
        &WasmMath::LessThan<uint64>,
        Saturate,
        0,
        UINT64_MAX>(
            src,
            scriptContext);
}
