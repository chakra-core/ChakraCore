//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if _M_IX86 || _M_AMD64

namespace Js
{
    // SIMD.Int16x8 operation wrappers that cover instrinsics for x86/x64 system
    SIMDValue SIMDInt16x8Operation::OpInt16x8(int16 values[])
    {
        X86SIMDValue x86Result;
        // Sets the 8 signed 16-bit integer values, note in revised order: starts with x7
        x86Result.m128i_value = _mm_set_epi16(values[7], values[6], values[5], values[4], values[3], values[2], values[1], values[0]);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpSplat(int16 x)
    {
        X86SIMDValue x86Result;
        // set 8 signed 16-bit integers values to input value x
        x86Result.m128i_value = _mm_set1_epi16(x);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    // Unary Ops

     SIMDValue SIMDInt16x8Operation::OpNeg(const SIMDValue& value)
    {
        X86SIMDValue x86Result;

        X86SIMDValue SIGNMASK, temp;
        X86SIMDValue negativeOnes = { { -1, -1, -1, -1} };
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        temp.m128i_value = _mm_andnot_si128(v.m128i_value, negativeOnes.m128i_value); // (~value) & (negative ones)
        SIGNMASK.m128i_value = _mm_set1_epi16(0x0001);                            // set SIGNMASK to 1
        x86Result.m128i_value = _mm_add_epi16(SIGNMASK.m128i_value, temp.m128i_value);// add 4 integers respectively

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpNot(const SIMDValue& value)
    {
        X86SIMDValue x86Result;

        X86SIMDValue negativeOnes = { { -1, -1, -1, -1} };
        X86SIMDValue temp = X86SIMDValue::ToX86SIMDValue(value);
        x86Result.m128i_value = _mm_andnot_si128(temp.m128i_value, negativeOnes.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_add_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // a + b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_sub_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // a - b

        return X86SIMDValue::ToSIMDValue(x86Result);;
    }

    SIMDValue SIMDInt16x8Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_mullo_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value);
        result = X86SIMDValue::ToSIMDValue(x86Result);

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpAnd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_and_si128(tmpaValue.m128i_value, tmpbValue.m128i_value); // a & b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpOr(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_or_si128(tmpaValue.m128i_value, tmpbValue.m128i_value); // a | b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpXor(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_xor_si128(tmpaValue.m128i_value, tmpbValue.m128i_value); // a ^ b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_adds_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // a + b saturates

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_subs_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // a - b saturates

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_min_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // min a b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_max_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // min a b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128i_value = _mm_cmplt_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a < b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result, x86Result1, x86Result2;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result1.m128i_value = _mm_cmplt_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a < b?
        x86Result2.m128i_value = _mm_cmpeq_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?
        x86Result.m128i_value  = _mm_or_si128(x86Result1.m128i_value, x86Result2.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128i_value = _mm_cmpeq_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result, tmpResult;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        tmpResult.m128i_value = _mm_cmpeq_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?
        X86SIMDValue negativeOnes = { { -1, -1, -1, -1} };
        x86Result.m128i_value = _mm_andnot_si128(tmpResult.m128i_value, negativeOnes.m128i_value);
        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128i_value = _mm_cmpgt_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a > b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result, x86Result1, x86Result2;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result1.m128i_value = _mm_cmpgt_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a > b?
        x86Result2.m128i_value = _mm_cmpeq_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?
        x86Result.m128i_value = _mm_or_si128(x86Result1.m128i_value, x86Result2.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    // ShiftOps
    SIMDValue SIMDInt16x8Operation::OpShiftLeftByScalar(const SIMDValue& value, int count)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpValue = X86SIMDValue::ToX86SIMDValue(value);
        // Shifts the 8 signed 16-bit integers in a left by count bits while shifting in zeros
        x86Result.m128i_value = _mm_slli_epi16(tmpValue.m128i_value, count & SIMDUtils::SIMDGetShiftAmountMask(2));

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt16x8Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpValue = X86SIMDValue::ToX86SIMDValue(value);
        // Shifts the 8 signed 16-bit integers right by count bits while shifting in the sign bit
        x86Result.m128i_value = _mm_srai_epi16(tmpValue.m128i_value, count &  SIMDUtils::SIMDGetShiftAmountMask(2));

        return X86SIMDValue::ToSIMDValue(x86Result);
    }
}

#endif

