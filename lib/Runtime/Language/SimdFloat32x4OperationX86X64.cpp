//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if _M_IX86 || _M_AMD64

namespace Js
{
    SIMDValue SIMDFloat32x4Operation::OpFloat32x4(float x, float y, float z, float w)
    {
        X86SIMDValue x86Result;

        // Sets the 4 single-precision, floating-point values, note order starts with W below
        x86Result.m128_value = _mm_set_ps(w, z, y, x);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpSplat(float x)
    {
        X86SIMDValue x86Result;
        // Sets the four single-precision, floating-point values to x
        x86Result.m128_value = _mm_set1_ps(x);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    // Conversions
    SIMDValue SIMDFloat32x4Operation::OpFromFloat64x2(const SIMDValue& value)
    {
        X86SIMDValue x86Result;
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        // Converts the two double-precision, floating-point values of v.m128d_value
        // to single-precision, floating-point values.
        x86Result.m128_value = _mm_cvtpd_ps(v.m128d_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpFromInt32x4(const SIMDValue& value)
    {
        X86SIMDValue x86Result;
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        // Converts the 4 signed 32-bit integer values of v.m128i_value
        // to single-precision, floating-point values.
        x86Result.m128_value = _mm_cvtepi32_ps(v.m128i_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpFromUint32x4(const SIMDValue& value)
    {
        X86SIMDValue x86Result, temp1;

        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        // find unsigned values above 2^31-1. Comparison is signed, so look for values < 0
        temp1.m128i_value = _mm_cmplt_epi32(v.m128i_value, X86_ALL_ZEROS.m128i_value);
        // temp1 has f32(2^32) for unsigned values above 2^31, 0 otherwise
        temp1.m128_value = _mm_and_ps(temp1.m128_value, X86_TWO_32_F4.m128_value);
        // convert
        x86Result.m128_value = _mm_cvtepi32_ps(v.m128i_value);
        // Add f32(2^32) to negative values
        x86Result.m128_value = _mm_add_ps(x86Result.m128_value, temp1.m128_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }



    // Unary Ops
    SIMDValue SIMDFloat32x4Operation::OpAbs(const SIMDValue& value)
    {
        X86SIMDValue x86Result = { 0 };
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        x86Result.m128_value = _mm_and_ps(v.m128_value, X86_ABS_MASK_F4.m128_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpNeg(const SIMDValue& value)
    {
        X86SIMDValue x86Result;
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);
        x86Result.m128_value = _mm_xor_ps(v.m128_value, X86_NEG_MASK_F4.m128_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpNot(const SIMDValue& value)
    {
        X86SIMDValue x86Result;

        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);
        x86Result.m128_value = _mm_xor_ps(v.m128_value, X86_ALL_NEG_ONES.m128_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpReciprocal(const SIMDValue& value)
    {
        X86SIMDValue x86Result;
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        // RCPPS is not precise. Using DIVPS
        // Divides the four single-precision, floating-point values of 1.0 and value
        x86Result.m128_value = _mm_div_ps(X86_ALL_ONES_F4.m128_value, v.m128_value); // result = 1.0/value

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpReciprocalSqrt(const SIMDValue& value)
    {
        X86SIMDValue x86Result;
        X86SIMDValue temp;
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        temp.m128_value = _mm_div_ps(X86_ALL_ONES_F4.m128_value, v.m128_value); // temp = 1.0/value
        x86Result.m128_value = _mm_sqrt_ps(temp.m128_value);              // result = sqrt(1.0/value)

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpSqrt(const SIMDValue& value)
    {
        X86SIMDValue x86Result;

        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        x86Result.m128_value = _mm_sqrt_ps(v.m128_value);   // result = sqrt(value)

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    // Binary Ops
    SIMDValue SIMDFloat32x4Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128_value = _mm_add_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a + b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128_value = _mm_sub_ps(tmpaValue.m128_value, tmpbValue.m128_value);  // a - b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128_value = _mm_mul_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a * b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpDiv(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128_value = _mm_div_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a / b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpAnd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128_value = _mm_and_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a & b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpOr(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128_value = _mm_or_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a | b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpXor(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);

        x86Result.m128_value = _mm_xor_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a ^ b

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    /*
    Min/Max(a, b) spec semantics:
    If any value is NaN, return NaN
    a < b ? a : b; where +0.0 > -0.0 (vice versa for Max)

    X86 MIN/MAXPS semantics:
    If any value is NaN, return 2nd operand
    If both values are +/-0.0, return 2nd operand
    return a < b ? a : b (vice versa for Max)
    */

    SIMDValue SIMDFloat32x4Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        X86SIMDValue tmp1, tmp2;

        // if tmp1 and tmp2 are not identical then either
        // 1) at least one value is NaN, then the OR will set that lane to NaN
        // 2) one value is 0.0 and the other is -0.0, the OR will set the sign bit to have -0.0
        tmp1.m128_value = _mm_min_ps(tmpaValue.m128_value, tmpbValue.m128_value);
        tmp2.m128_value = _mm_min_ps(tmpbValue.m128_value, tmpaValue.m128_value);
        x86Result.m128_value = _mm_or_ps(tmp1.m128_value, tmp2.m128_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        X86SIMDValue tmp1, tmp2, NaNs;

        // if tmp1 and tmp2 are not identical then either
        // 1) at least one value is NaN, then the OR will set that lane to NaN
        // 2) one value is 0.0 and the other is -0.0, the OR will set the sign bit to have -0.0

        // 1's where NaNs are
        NaNs.m128_value = _mm_cmpunord_ps(tmpaValue.m128_value, tmpbValue.m128_value);
        tmp1.m128_value = _mm_max_ps(tmpaValue.m128_value, tmpbValue.m128_value);
        tmp2.m128_value = _mm_max_ps(tmpbValue.m128_value, tmpaValue.m128_value);
        // Force lanes that had +/-0.0 to be +0.0
        // Lanes that had NaNs can be garbage after this step.
        tmp1.m128_value = _mm_and_ps(tmp1.m128_value, tmp2.m128_value);
        // Fix lanes that had NaNs to all 1's (NaNs).
        x86Result.m128_value = _mm_or_ps(tmp1.m128_value, NaNs.m128_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpScale(const SIMDValue& Value, float scaleValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(Value);

        X86SIMDValue scaleVector;
        scaleVector.m128_value = _mm_set1_ps(scaleValue);
        x86Result.m128_value = _mm_mul_ps(v.m128_value, scaleVector.m128_value); // v * scale

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128_value = _mm_cmplt_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a < b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128_value = _mm_cmple_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a <= b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128_value = _mm_cmpeq_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a == b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128_value = _mm_cmpneq_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a != b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128_value = _mm_cmpgt_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a > b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        X86SIMDValue x86Result;
        X86SIMDValue tmpaValue = X86SIMDValue::ToX86SIMDValue(aValue);
        X86SIMDValue tmpbValue = X86SIMDValue::ToX86SIMDValue(bValue);
        x86Result.m128_value = _mm_cmpge_ps(tmpaValue.m128_value, tmpbValue.m128_value); // a >= b?

        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDFloat32x4Operation::OpClamp(const SIMDValue& value, const SIMDValue& lower, const SIMDValue& upper)
    { // SIMD review: do we have intrinsic for the implementation?
        SIMDValue result;

        // lower clamp
        result.f32[SIMD_X] = value.f32[SIMD_X] < lower.f32[SIMD_X] ? lower.f32[SIMD_X] : value.f32[SIMD_X];
        result.f32[SIMD_Y] = value.f32[SIMD_Y] < lower.f32[SIMD_Y] ? lower.f32[SIMD_Y] : value.f32[SIMD_Y];
        result.f32[SIMD_Z] = value.f32[SIMD_Z] < lower.f32[SIMD_Z] ? lower.f32[SIMD_Z] : value.f32[SIMD_Z];
        result.f32[SIMD_W] = value.f32[SIMD_W] < lower.f32[SIMD_W] ? lower.f32[SIMD_W] : value.f32[SIMD_W];

        // upper clamp
        result.f32[SIMD_X] = result.f32[SIMD_X] > upper.f32[SIMD_X] ? upper.f32[SIMD_X] : result.f32[SIMD_X];
        result.f32[SIMD_Y] = result.f32[SIMD_Y] > upper.f32[SIMD_Y] ? upper.f32[SIMD_Y] : result.f32[SIMD_Y];
        result.f32[SIMD_Z] = result.f32[SIMD_Z] > upper.f32[SIMD_Z] ? upper.f32[SIMD_Z] : result.f32[SIMD_Z];
        result.f32[SIMD_W] = result.f32[SIMD_W] > upper.f32[SIMD_W] ? upper.f32[SIMD_W] : result.f32[SIMD_W];

        return result;
    }

    SIMDValue SIMDFloat32x4Operation::OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV)
    {
        X86SIMDValue x86Result;
        X86SIMDValue maskValue  = X86SIMDValue::ToX86SIMDValue(mV);
        X86SIMDValue trueValue  = X86SIMDValue::ToX86SIMDValue(tV);
        X86SIMDValue falseValue = X86SIMDValue::ToX86SIMDValue(fV);

        X86SIMDValue tempTrue, tempFalse;
        tempTrue.m128_value = _mm_and_ps(maskValue.m128_value, trueValue.m128_value);      // mask & True
        tempFalse.m128_value = _mm_andnot_ps(maskValue.m128_value, falseValue.m128_value); // !mask & False
        x86Result.m128_value = _mm_or_ps(tempTrue.m128_value, tempFalse.m128_value); // tempTrue | tempFalse

        return X86SIMDValue::ToSIMDValue(x86Result);
    }
}

#endif
