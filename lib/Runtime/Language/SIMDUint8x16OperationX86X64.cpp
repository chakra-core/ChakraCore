//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if _M_IX86 || _M_AMD64

namespace Js
{
    // SIMD.UInt8x16 operation wrappers that cover instrinsics for x86/x64 system
    SIMDValue SIMDUint8x16Operation::OpUint8x16(uint8 x0, uint8 x1, uint8 x2, uint8 x3
        , uint8 x4, uint8 x5, uint8 x6, uint8 x7
        , uint8 x8, uint8 x9, uint8 x10, uint8 x11
        , uint8 x12, uint8 x13, uint8 x14, uint8 x15)
    {
        X86SIMDValue x86Result;
        // Sets the 16 signed 8-bit integer values, note in revised order: starts with x15 below
        x86Result.m128i_value = _mm_set_epi8((int8)x15, (int8)x14, (int8)x13, (int8)x12, (int8)x11, (int8)x10, (int8)x9, (int8)x8, (int8)x7, (int8)x6, (int8)x5, (int8)x4, (int8)x3, (int8)x2, (int8)x1, (int8)x0);

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
        X86SIMDValue signBits = { {0x80808080,0x80808080, 0x80808080, 0x80808080} };

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
        X86SIMDValue signBits = { { 0x80808080,0x80808080, 0x80808080, 0x80808080 } };

        // Signed comparison of unsigned ints can be done if the ints have the "sign" bit xored with 1
        tmpaValue.m128i_value = _mm_xor_si128(tmpaValue.m128i_value, signBits.m128i_value);
        tmpbValue.m128i_value = _mm_xor_si128(tmpbValue.m128i_value, signBits.m128i_value);
        x86Result.m128i_value = _mm_cmplt_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a < b?
        tmpaValue.m128i_value = _mm_cmpeq_epi8(tmpaValue.m128i_value, tmpbValue.m128i_value); // compare a == b?
        x86Result.m128i_value = _mm_or_si128(x86Result.m128i_value, tmpaValue.m128i_value);   // result = (a<b)|(a==b)

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDUint8x16Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        X86SIMDValue x86Result = { { 0, 0, 0, 0} };
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(value);
        __m128i x86tmp1;

        if (count < 8)
        {
            __m128i mask = _mm_set1_epi8((unsigned char)0xff >> count);

            x86tmp1 = _mm_srli_epi16(tmpaValue.m128i_value, count);
            x86Result.m128i_value = _mm_and_si128(x86tmp1, mask);
        }

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
