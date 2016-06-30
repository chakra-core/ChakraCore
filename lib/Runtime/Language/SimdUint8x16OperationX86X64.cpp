//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if _M_IX86 || _M_AMD64

namespace Js
{
    // SIMD.UInt8x16 operation wrappers that cover instrinsics for x86/x64 system
    SIMDValue SIMDUint8x16Operation::OpUint8x16(uint8 values[])
    {
        X86SIMDValue x86Result;
        // Sets the 16 signed 8-bit integer values, note in revised order: starts with x15 below
        x86Result.m128i_value = _mm_set_epi8((int8)values[15], (int8)values[14], (int8)values[13], (int8)values[12],
                                             (int8)values[11], (int8)values[10], (int8)values[9], (int8)values[8],
                                             (int8)values[7], (int8)values[6], (int8)values[5], (int8)values[4],
                                             (int8)values[3], (int8)values[2], (int8)values[1], (int8)values[0]);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint8x16Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_min_epu8(tmpaValue.m128i_value, tmpbValue.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint8x16Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128i_value = _mm_max_epu8(tmpaValue.m128i_value, tmpbValue.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint8x16Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

#pragma warning(push)
#pragma warning(disable:4838) // conversion from 'unsigned int' to 'int32' requires a narrowing conversion
        X86SIMDValue signBits = { {0x80808080,0x80808080, 0x80808080, 0x80808080} };
#pragma warning(pop)

        // Signed comparison of unsigned ints can be done if the ints have the "sign" bit xored with 1
        tmpaValue.m128i_value = _mm_xor_si128(tmpaValue.m128i_value, signBits.m128i_value);
        tmpbValue.m128i_value = _mm_xor_si128(tmpbValue.m128i_value, signBits.m128i_value);
        x86Result.m128i_value = _mm_cmplt_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a < b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint8x16Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

#pragma warning(push)
#pragma warning(disable:4838) // conversion from 'unsigned int' to 'int32' requires a narrowing conversion
        X86SIMDValue signBits = { { 0x80808080,0x80808080, 0x80808080, 0x80808080 } };
#pragma warning(pop)

        // Signed comparison of unsigned ints can be done if the ints have the "sign" bit xored with 1
        tmpaValue.m128i_value = _mm_xor_si128(tmpaValue.m128i_value, signBits.m128i_value);
        tmpbValue.m128i_value = _mm_xor_si128(tmpbValue.m128i_value, signBits.m128i_value);
        x86Result.m128i_value = _mm_cmplt_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a < b?
        tmpaValue.m128i_value = _mm_cmpeq_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?
        x86Result.m128i_value = _mm_or_si128(x86Result.m128i_value, tmpaValue.m128i_value);   // result = (a<b)|(a==b)

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint8x16Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        result = SIMDUint8x16Operation::OpLessThan(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        result = SIMDUint8x16Operation::OpLessThanOrEqual(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        X86SIMDValue x86Result = { { 0, 0, 0, 0} };
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(value);
        __m128i x86tmp1;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(1);

       __m128i mask = _mm_set1_epi8((unsigned char)0xff >> count);
        x86tmp1 = _mm_srli_epi16(tmpaValue.m128i_value, count);
        x86Result.m128i_value = _mm_and_si128(x86tmp1, mask);
       
        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint8x16Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128i_value = _mm_adds_epu8(tmpaValue.m128i_value, tmpbValue.m128i_value); // a + b saturated

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint8x16Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128i_value = _mm_subs_epu8(tmpaValue.m128i_value, tmpbValue.m128i_value); // a - b saturated

        return X86SIMDValue::ToSIMDValue(x86Result);
    }
}

#endif
