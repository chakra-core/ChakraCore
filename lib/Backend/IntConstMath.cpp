//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

bool IntConstMath::IsValid(IntConstType val, IRType type)
{
    switch (type)
    {
#if TARGET_32
    case TyInt32:
    case TyUint32:
        CompileAssert(sizeof(IntConstType) == sizeof(int32));
        return true;
#elif TARGET_64
    case TyInt32:
    case TyUint32:
        return Math::FitsInDWord(val);
    case TyInt64:
    case TyUint64:
        CompileAssert(sizeof(IntConstType) == sizeof(int64));
        return true;
#endif
    default:
        Assert(UNREACHED);
        return false;
    }
}

bool IntConstMath::Add(IntConstType left, IntConstType right, IRType type, IntConstType * result)
{
    bool overflowed = IntMathCommon<IntConstType>::Add(left, right, result);
    return overflowed || !IsValid(*result, type);
}

bool IntConstMath::Sub(IntConstType left, IntConstType right, IRType type, IntConstType * result)
{
    bool overflowed = IntMathCommon<IntConstType>::Sub(left, right, result);
    return overflowed || !IsValid(*result, type);
}

bool IntConstMath::Mul(IntConstType left, IntConstType right, IRType type, IntConstType * result)
{
#if TARGET_32
    bool overflowed = Int32Math::Mul(left, right, result);
    CompileAssert(sizeof(IntConstType) == sizeof(int32));
#elif TARGET_64
    bool overflowed = Int64Math::Mul(left, right, result);
    CompileAssert(sizeof(IntConstType) == sizeof(int64));
#endif
    return overflowed || !IsValid(*result, type);
}

bool IntConstMath::Div(IntConstType left, IntConstType right, IRType type, IntConstType * result)
{
    bool overflowed = IntMathCommon<IntConstType>::Div(left, right, result);
    return overflowed || !IsValid(*result, type);
}

bool IntConstMath::Mod(IntConstType left, IntConstType right, IRType type, IntConstType * result)
{
    bool overflowed = IntMathCommon<IntConstType>::Mod(left, right, result);
    return overflowed || !IsValid(*result, type);
}

bool IntConstMath::Dec(IntConstType val, IRType type, IntConstType * result)
{
    bool overflowed = IntMathCommon<IntConstType>::Dec(val, result);
    return overflowed || !IsValid(*result, type);
}

bool IntConstMath::Inc(IntConstType val, IRType type, IntConstType * result)
{
    bool overflowed = IntMathCommon<IntConstType>::Inc(val, result);
    return overflowed || !IsValid(*result, type);
}

bool IntConstMath::Neg(IntConstType val, IRType type, IntConstType * result)
{
    bool overflowed = IntMathCommon<IntConstType>::Neg(val, result);
    return overflowed || !IsValid(*result, type);
}
