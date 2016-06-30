//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if _M_IX86 || _M_AMD64

namespace Js
{
    SIMDValue SIMDUint32x4Operation::OpUint32x4(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
    {
        X86SIMDValue x86Result;
        x86Result.m128i_value = _mm_set_epi32(w, z, y, x);
        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint32x4Operation::OpSplat(unsigned int x)
    {
        X86SIMDValue x86Result;
        // set 4 signed 32-bit integers values to input value x
        x86Result.m128i_value = _mm_set1_epi32(x);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint32x4Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpValue = X86SIMDValue::ToX86SIMDValue(value);
        // Shifts the 4 signed 32-bit integers right by count bits while shifting in zeros
        x86Result.m128i_value = _mm_srli_epi32(tmpValue.m128i_value, count & SIMDUtils::SIMDGetShiftAmountMask(4));

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint32x4Operation::OpFromFloat32x4(const SIMDValue& value, bool& throws)
    {
        X86SIMDValue x86Result = { 0 };
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);
        X86SIMDValue temp, temp2;
        X86SIMDValue two_31_f4, two_31_i4;
        int mask = 0;

        // any lanes <= -1.0 ?
        temp.m128_value = _mm_cmple_ps(v.m128_value, X86_ALL_NEG_ONES_F4.m128_value);
        mask = _mm_movemask_ps(temp.m128_value);
        // negative value are out of range, caller should throw Range Error
        if (mask)
        {
            throws = true;
            return X86SIMDValue::ToSIMDValue(x86Result);
        }
        // CVTTPS2DQ does a range check over signed range [-2^31, 2^31-1], so will fail to convert values >= 2^31.
        // To fix this, subtract 2^31 from values >= 2^31, do CVTTPS2DQ, then add 2^31 back.
        _mm_store_ps(two_31_f4.f32, X86_TWO_31_F4.m128_value);
        // any lanes >= 2^31 ?
        temp.m128_value = _mm_cmpge_ps(v.m128_value, two_31_f4.m128_value);
        // two_31_f4 has f32(2^31) for lanes >= 2^31, 0 otherwise
        two_31_f4.m128_value = _mm_and_ps(two_31_f4.m128_value, temp.m128_value);
        // subtract 2^31 from lanes >= 2^31, unchanged otherwise.
        v.m128_value = _mm_sub_ps(v.m128_value, two_31_f4.m128_value);

        // CVTTPS2DQ
        x86Result.m128i_value = _mm_cvttps_epi32(v.m128_value);

        // check if any value is out of range (i.e. >= 2^31, meaning originally >= 2^32 before value adjustment)
        temp2.m128i_value = _mm_cmpeq_epi32(x86Result.m128i_value, X86_NEG_MASK_F4.m128i_value); // any value == 0x80000000 ?
        mask = _mm_movemask_ps(temp2.m128_value);
        if (mask)
        {
            throws = true;
            return X86SIMDValue::ToSIMDValue(x86Result);
        }
        // we pass range check

        // add 2^31 values back to adjusted values.
        // Use first bit from the 2^31 float mask (0x4f000...0 << 1)
        // and result with 2^31 int mask (0x8000..0) setting first bit to zero if lane hasn't been adjusted
        _mm_store_ps(two_31_i4.f32, X86_TWO_31_I4.m128_value);
        two_31_f4.m128i_value = _mm_slli_epi32(two_31_f4.m128i_value, 1);
        two_31_i4.m128i_value = _mm_and_si128(two_31_i4.m128i_value, two_31_f4.m128i_value);
        // add 2^31 back to adjusted values
        // Note at this point all values are in [0, 2^31-1]. Adding 2^31 is guaranteed not to overflow.
        x86Result.m128i_value = _mm_add_epi32(x86Result.m128i_value, two_31_i4.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    // Unary Ops

    SIMDValue SIMDUint32x4Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        // _mm_min_epu32 is SSE4.1
        //X86SIMDValue x86Result;
        //X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        //X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        //x86Result.m128i_value = _mm_min_epu32(tmpaValue.m128i_value, tmpbValue.m128i_value);

        SIMDValue selector = SIMDUint32x4Operation::OpLessThan(aValue, bValue);
        return SIMDInt32x4Operation::OpSelect(selector, aValue, bValue);
    }

    SIMDValue SIMDUint32x4Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        // _mm_min_epu32 is SSE4.1
        //X86SIMDValue x86Result;
        //X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        //X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        //x86Result.m128i_value = _mm_min_epu32(tmpaValue.m128i_value, tmpbValue.m128i_value);

        SIMDValue selector = SIMDUint32x4Operation::OpLessThan(bValue, aValue);
        return SIMDInt32x4Operation::OpSelect(selector, aValue, bValue);
    }

    SIMDValue SIMDUint32x4Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        X86SIMDValue signBits;
        signBits.m128i_value = _mm_set1_epi32(0x80000000);

        // Signed comparison of unsigned ints can be done if the ints have the "sign" bit xored with 1
        tmpaValue.m128i_value = _mm_xor_si128(tmpaValue.m128i_value, signBits.m128i_value);
        tmpbValue.m128i_value = _mm_xor_si128(tmpbValue.m128i_value, signBits.m128i_value);
        x86Result.m128i_value = _mm_cmplt_epi32(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a < b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint32x4Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        X86SIMDValue signBits;
        signBits.m128i_value = _mm_set1_epi32(0x80000000);

        // Signed comparison of unsigned ints can be done if the ints have the "sign" bit xored with 1
        tmpaValue.m128i_value = _mm_xor_si128(tmpaValue.m128i_value, signBits.m128i_value);
        tmpbValue.m128i_value = _mm_xor_si128(tmpbValue.m128i_value, signBits.m128i_value);
        x86Result.m128i_value = _mm_cmplt_epi32(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a < b?
        tmpaValue.m128i_value = _mm_cmpeq_epi32(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?
        x86Result.m128i_value = _mm_or_si128(x86Result.m128i_value, tmpaValue.m128i_value);   // result = (a<b)|(a==b)

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint32x4Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        result = SIMDUint32x4Operation::OpLessThan(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
        return result;
    }

    SIMDValue SIMDUint32x4Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        result = SIMDUint32x4Operation::OpLessThanOrEqual(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
        return result;
    }
    
}

#endif
