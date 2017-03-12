//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeMathPch.h"

namespace Js
{
    // These implementations need to be compiled with /arch:sse2
#define DivImpl(type, noinline) template<> noinline type AsmJsMath::Div(type aLeft, type aRight) { return aLeft / aRight; }
#define MulImpl(type, noinline) template<> noinline type AsmJsMath::Mul(type aLeft, type aRight) { return aLeft * aRight; }
#define DivMulImpl(type, noinline) DivImpl(type, noinline) MulImpl(type, noinline)

#if _M_IX86
#define DB_NOINLINE _NOINLINE
#else
#define DB_NOINLINE
#endif
    DivMulImpl(double, DB_NOINLINE)
    DivMulImpl(float,)
    DivMulImpl(int64,)
    DivMulImpl(uint64,)
    MulImpl(int32,)
    MulImpl(uint32,)

    template<>
    int32 AsmJsMath::Div<int32>(int32 aLeft, int32 aRight)
    {
        return aRight == 0 ? 0 : (aLeft == (1 << 31) && aRight == -1) ? aLeft : aLeft / aRight;
    }

    template<>
    uint32 AsmJsMath::Div<uint32>(uint32 aLeft, uint32 aRight)
    {
        return aRight == 0 ? 0 : aLeft / aRight;
    }
}
