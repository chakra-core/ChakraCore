//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Wasm
{
/* static */
inline int
WasmMath::Ctz(int value)
{
    DWORD index;
    if (_BitScanForward(&index, value))
    {
        return index;
    }
    return 32;
}

inline int
WasmMath::Eqz(int value)
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


inline int
WasmMath::Rol(int aLeft, int aRight)
{
    return _rotl(aLeft, aRight);
}

inline int
WasmMath::Ror(int aLeft, int aRight)
{
    return _rotr(aLeft, aRight);
}

}
