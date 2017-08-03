//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include "IntMathCommon.h"

class Int32Math: public IntMathCommon<int32>
{
public:
    static bool Add(int32 left, int32 right, int32 *pResult);
    static bool Mul(int32 left, int32 right, int32 *pResult);
    static bool Mul(int32 left, int32 right, int32 *pResult, int32* pOverflowValue);

    static bool Shl(int32 left, int32 right, int32 *pResult);
    static bool Shr(int32 left, int32 right, int32 *pResult);
    static bool ShrU(int32 left, int32 right, int32 *pResult);
};

template <>
inline bool Math::IncImpl<int32>(int32 val, int32 *pResult)
{
    return Int32Math::Inc(val, pResult);
}

template <>
inline bool Math::AddImpl<int32>(int32 left, int32 right, int32 *pResult)
{
    return Int32Math::Add(left, right, pResult);
}
