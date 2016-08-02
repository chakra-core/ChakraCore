//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Wasm
{

template<typename T>
inline T WasmMath::Div( T aLeft, T aRight )
{
    // Todo:: Trap on aRight == 0 for int64 and uint64
    return aLeft / aRight;
}

constexpr uint64 specialDivLeftValue = (uint64)1 << 63;

template<>
inline int64 WasmMath::Rem( int64 aLeft, int64 aRight )
{
    // Todo:: Trap on aRight == 0
    return (aLeft == specialDivLeftValue && aRight == -1) ? 0 : aLeft % aRight;
}

template<>
inline uint64 WasmMath::Rem( uint64 aLeft, uint64 aRight )
{
    // Todo:: Trap on aRight == 0
    return (aLeft == specialDivLeftValue && aRight == -1) ? specialDivLeftValue : aLeft % aRight;
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
    return __popcnt(value);
}

template<> 
inline int64 WasmMath::PopCnt(int64 value)
{
#if TARGET_64
    return __popcnt64(value);
#else
    return int64(__popcnt((uint)value) + __popcnt((uint)(value >> 32)));
#endif
}


template<typename T>
inline int WasmMath::Eqz(T value)
{
    return value == 0;
}

template<typename T>
inline T WasmMath::Copysign(T aLeft, T aRight)
{
    return (T)_copysign(aLeft, aRight);
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
