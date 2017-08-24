//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Wasm
{
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

template <typename STYPE,
    typename UTYPE,
    UTYPE MAX,
    UTYPE NEG_ZERO,
    UTYPE NEG_ONE,
    WasmMath::CmpPtr<UTYPE> CMP1,
    WasmMath::CmpPtr<UTYPE> CMP2>
bool WasmMath::isInRange(STYPE srcVal)
{
    Assert(sizeof(STYPE) == sizeof(UTYPE));
    UTYPE val = *reinterpret_cast<UTYPE*> (&srcVal);
    return (CMP1(val, MAX)) || (val >= NEG_ZERO && CMP2(val, NEG_ONE));
}

template <typename STYPE> bool  WasmMath::isNaN(STYPE src)
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

}
