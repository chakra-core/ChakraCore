//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeMathPch.h"

namespace Js
{
    // These implementations need to be compiled with /arch:sse2
#define DivImpl(type) template<> type AsmJsMath::Div(type aLeft, type aRight) { return aLeft / aRight; }
#define MulImpl(type) template<> type AsmJsMath::Mul(type aLeft, type aRight) { return aLeft * aRight; }
#define DivMulImpl(type) DivImpl(type) MulImpl(type)

    DivMulImpl(float)
    DivMulImpl(double)
    DivMulImpl(int64)
    DivMulImpl(uint64)
    MulImpl(int32)
    MulImpl(uint32)

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
