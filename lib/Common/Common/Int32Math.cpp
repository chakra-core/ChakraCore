//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCommonPch.h"

bool
Int32Math::Add(int32 left, int32 right, int32 *pResult)
{
#if __has_builtin(__builtin_add_overflow) || TARGET_32
    return IntMathCommon<int32>::Add(left, right, pResult);
#else

    Assert(sizeof(void *) == 8);
    int64 result64 = (int64)left + (int64)right;
    *pResult = (int32)result64;
    return result64 != (int64)(*pResult);

#endif
}

bool
Int32Math::Mul(int32 left, int32 right, int32 *pResult)
{
#if __has_builtin(__builtin_mul_overflow) && !(defined(_ARM_) && defined(__clang__))
    return IntMathCommon<int32>::Mul(left, right, pResult);
#else

    bool fOverflow;
#if _M_IX86
    __asm
    {
        mov eax, left
        imul right
        seto fOverflow
        mov ecx, pResult
        mov[ecx], eax
    }
#else
    int64 result64 = (int64)left * (int64)right;

    *pResult = (int32)result64;

    fOverflow = (result64 != (int64)(*pResult));
#endif

    return fOverflow;

#endif  // !__has_builtin(__builtin_mul_overflow)
}

bool
Int32Math::Mul(int32 left, int32 right, int32 *pResult, int32* pOverflowValue)
{
    bool fOverflow;
#if _M_IX86
    __asm
    {
        mov eax, left
        imul right
        seto fOverflow
        mov ecx, pResult
        mov[ecx], eax
        mov ecx, pOverflowValue
        mov[ecx], edx
    }
#else
    int64 result64 = (int64)left * (int64)right;

    *pResult = (int32)result64;
    *pOverflowValue = (int32)(result64 >> 32);

    fOverflow = (result64 != (int64)(*pResult));
#endif

    return fOverflow;
}

bool
Int32Math::Shl(int32 left, int32 right, int32 *pResult)
{
    *pResult = left << (right & 0x1F);
    return (left != (int32)((uint32)*pResult >> right));
}

bool
Int32Math::Shr(int32 left, int32 right, int32 *pResult)
{
    *pResult = left >> (right & 0x1F);
    return false;
}

bool
Int32Math::ShrU(int32 left, int32 right, int32 *pResult)
{
    uint32 uResult = ((uint32)left) >> (right & 0x1F);
    *pResult = uResult;
    return false;
}
