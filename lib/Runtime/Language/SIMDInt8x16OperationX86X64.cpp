//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if _M_IX86 || _M_AMD64

namespace Js
{
    // SIMD.Int8x16 operation wrappers that cover intrinsics for x86/x64 system
    SIMDValue SIMDInt8x16Operation::OpInt8x16(int8 x0, int8 x1, int8 x2, int8 x3
        , int8 x4, int8 x5, int8 x6, int8 x7
        , int8 x8, int8 x9, int8 x10, int8 x11
        , int8 x12, int8 x13, int8 x14, int8 x15)
    {
        X86SIMDValue x86Result;
        // Sets the 16 signed 8-bit integer values, note in revised order: starts with x15 below
        x86Result.m128i_value = _mm_set_epi8((int8)x15, (int8)x14, (int8)x13, (int8)x12, (int8)x11, (int8)x10, (int8)x9, (int8)x8, (int8)x7, (int8)x6, (int8)x5, (int8)x4, (int8)x3, (int8)x2, (int8)x1, (int8)x0);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpSplat(int8 x)
    {
        X86SIMDValue x86Result;
        // set 16 signed 8-bit integers values to input value x
        x86Result.m128i_value = _mm_set1_epi8(x);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    //// Unary Ops

    SIMDValue SIMDInt8x16Operation::OpNeg(const SIMDValue& value)
    {
        X86SIMDValue x86Result;

        X86SIMDValue SIGNMASK, temp;
        X86SIMDValue negativeOnes = { { -1, -1, -1, -1 } };
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        temp.m128i_value = _mm_andnot_si128(v.m128i_value, negativeOnes.m128i_value); // (~value) & (negative ones)
        SIGNMASK.m128i_value = _mm_set1_epi8(0x00000001);                            // set SIGNMASK to 1
        x86Result.m128i_value = _mm_add_epi8(SIGNMASK.m128i_value, temp.m128i_value);// add 16 integers respectively

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_add_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // a + b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_sub_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // a - b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue x86tmp1;
        X86SIMDValue x86tmp2;
        X86SIMDValue x86tmp3;
        const _x86_SIMDValue X86_LOWBYTE_MASK  = { 0x00ff00ff, 0x00ff00ff, 0x00ff00ff, 0x00ff00ff };
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        // (ah* 2^8 + al) * (bh *2^8 + bl) = (ah*bh* 2^8 + al*bh + ah* bl) * 2^8 + al * bl
        x86tmp1.m128i_value = _mm_mullo_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value);
        x86tmp2.m128i_value = _mm_and_si128(x86tmp1.m128i_value, X86_LOWBYTE_MASK.m128i_value);

        tmpaValue.m128i_value = _mm_srli_epi16(tmpaValue.m128i_value, 8);
        tmpbValue.m128i_value = _mm_srli_epi16(tmpbValue.m128i_value, 8);
        x86tmp3.m128i_value   = _mm_mullo_epi16(tmpaValue.m128i_value, tmpbValue.m128i_value);
        x86tmp3.m128i_value   = _mm_slli_epi16(x86tmp3.m128i_value, 8);

        x86Result.m128i_value = _mm_or_si128(x86tmp2.m128i_value, x86tmp3.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_adds_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // a + b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_subs_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // a - b

        return X86SIMDValue::ToSIMDValue(x86Result);;
    }


    SIMDValue SIMDInt8x16Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        //Only available in SSE 4
        //x86Result.m128i_value = _mm_min_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // min a b
        //SSE 2
        SIMDValue selector = SIMDInt8x16Operation::OpLessThan(aValue, bValue);
        return SIMDInt8x16Operation::OpSelect(selector, aValue, bValue);
    }

    SIMDValue SIMDInt8x16Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        //Only available in SSE 4
        //x86Result.m128i_value = _mm_max_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // min a b
        //SSE 2
        SIMDValue selector = SIMDInt8x16Operation::OpGreaterThan(aValue, bValue);
        return SIMDInt8x16Operation::OpSelect(selector, aValue, bValue);
    }

    SIMDValue SIMDInt8x16Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128i_value = _mm_cmplt_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a < b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        if (AutoSystemInfo::Data.SSE4_1Available())
        {
            x86Result.m128i_value = _mm_max_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value);  //  max(a,b) == a
            x86Result.m128i_value = _mm_cmpeq_epi8(tmpbValue.m128i_value, x86Result.m128i_value); //
        }
        else
        {
            X86SIMDValue x86Tmp1, x86Tmp2;
            x86Tmp1.m128i_value = _mm_cmplt_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a > b?
            x86Tmp2.m128i_value = _mm_cmpeq_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?
            x86Result.m128i_value = _mm_or_si128(x86Tmp1.m128i_value, x86Tmp2.m128i_value);
        }

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128i_value = _mm_cmpeq_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128i_value = _mm_cmpeq_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a != b?

        X86SIMDValue negativeOnes = { { -1, -1, -1, -1 } };
        x86Result.m128i_value = _mm_andnot_si128(x86Result.m128i_value, negativeOnes.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }


    SIMDValue SIMDInt8x16Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128i_value = _mm_cmpgt_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a > b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        if (AutoSystemInfo::Data.SSE4_1Available())
        {
            x86Result.m128i_value = _mm_max_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value);  //  max(a,b) == b
            x86Result.m128i_value = _mm_cmpeq_epi8(tmpaValue.m128i_value, x86Result.m128i_value); //
        }
        else
        {
            X86SIMDValue x86Tmp1, x86Tmp2;
            x86Tmp1.m128i_value = _mm_cmpgt_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a > b?
            x86Tmp2.m128i_value = _mm_cmpeq_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?
            x86Result.m128i_value = _mm_or_si128(x86Tmp1.m128i_value, x86Tmp2.m128i_value);
        }


        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpShiftLeftByScalar(const SIMDValue& value, int8 count)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(value);
        X86SIMDValue x86tmp1;

        const _x86_SIMDValue X86_LOWBYTE_MASK  = { 0x00ff00ff, 0x00ff00ff, 0x00ff00ff, 0x00ff00ff };
        const _x86_SIMDValue X86_HIGHBYTE_MASK = { 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00 };

        if (count < 0 || count > 8)
        {
            count = 8;
        }

        x86tmp1.m128i_value = _mm_and_si128(tmpaValue.m128i_value, X86_HIGHBYTE_MASK.m128i_value);
        x86tmp1.m128i_value = _mm_slli_epi16(x86tmp1.m128i_value, count);

        tmpaValue.m128i_value = _mm_slli_epi16(tmpaValue.m128i_value, count);
        tmpaValue.m128i_value = _mm_and_si128(tmpaValue.m128i_value, X86_LOWBYTE_MASK.m128i_value);

        x86Result.m128i_value = _mm_or_si128(tmpaValue.m128i_value, x86tmp1.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpShiftRightByScalar(const SIMDValue& value, int8 count)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(value);
        X86SIMDValue x86tmp1;

        const _x86_SIMDValue X86_LOWBYTE_MASK  = { 0x00ff00ff, 0x00ff00ff, 0x00ff00ff, 0x00ff00ff };
        const _x86_SIMDValue X86_HIGHBYTE_MASK = { 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00 };

        if (count < 0 || count > 8)
        {
            count = 8;
        }

        x86tmp1.m128i_value = _mm_slli_epi16(tmpaValue.m128i_value, 8);
        x86tmp1.m128i_value = _mm_srai_epi16(x86tmp1.m128i_value, count + 8);

        x86tmp1.m128i_value = _mm_and_si128(x86tmp1.m128i_value, X86_LOWBYTE_MASK.m128i_value);

        tmpaValue.m128i_value = _mm_srai_epi16(tmpaValue.m128i_value, count);
        tmpaValue.m128i_value = _mm_and_si128(tmpaValue.m128i_value, X86_HIGHBYTE_MASK.m128i_value);

        x86Result.m128i_value = _mm_or_si128(tmpaValue.m128i_value, x86tmp1.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDInt8x16Operation::OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV)
    {
        X86SIMDValue x86Result;
        X86SIMDValue maskValue  = X86SIMDValue::ToX86SIMDValue(mV);
        X86SIMDValue trueValue  = X86SIMDValue::ToX86SIMDValue(tV);
        X86SIMDValue falseValue = X86SIMDValue::ToX86SIMDValue(fV);

        X86SIMDValue tempTrue, tempFalse;
        tempTrue.m128i_value  = _mm_and_si128(maskValue.m128i_value, trueValue.m128i_value); // mask & T
        tempFalse.m128i_value = _mm_andnot_si128(maskValue.m128i_value, falseValue.m128i_value); //!mask & F
        x86Result.m128i_value = _mm_or_si128(tempTrue.m128i_value, tempFalse.m128i_value);  // tempT | temp F

        return X86SIMDValue::ToSIMDValue(x86Result);
    }
}
#endif
