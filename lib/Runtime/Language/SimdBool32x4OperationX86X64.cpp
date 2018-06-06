//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if _M_IX86 || _M_AMD64

namespace Js
{
    SIMDValue SIMDBool32x4Operation::OpBool32x4(bool x, bool y, bool z, bool w)
    {
        X86SIMDValue x86Result;
        x86Result.m128i_value = _mm_set_epi32(w?-1:0, z?-1:0, y?-1:0, x?-1:0);
        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDBool32x4Operation::OpBool32x4(const SIMDValue& v)
    {
        // overload function with input parameter as SIMDValue for completeness
        SIMDValue result;
        result = v;
        return result;
    }

    // Unary Ops
    template <typename T>
    bool SIMDBool32x4Operation::OpAnyTrue(const SIMDValue& simd)
    {
        SIMDValue canonSimd = SIMDUtils::CanonicalizeToBools<T>(simd); //copy-by-value since we need to modify the copy
        X86SIMDValue x86Simd = X86SIMDValue::ToX86SIMDValue(canonSimd);
        int mask_8 = _mm_movemask_epi8(x86Simd.m128i_value); //latency 3, throughput 1
        return mask_8 != 0;
    }

    template <typename T>
    bool SIMDBool32x4Operation::OpAllTrue(const SIMDValue& simd)
    {
        SIMDValue canonSimd = SIMDUtils::CanonicalizeToBools<T>(simd); //copy-by-value since we need to modify the copy
        X86SIMDValue x86Simd = X86SIMDValue::ToX86SIMDValue(canonSimd);
        int mask_8 = _mm_movemask_epi8(x86Simd.m128i_value); //latency 3, throughput 1
        return mask_8 == 0xFFFF;
    }

    template bool SIMDBool32x4Operation::OpAllTrue<int64>(const SIMDValue& simd);
    template bool SIMDBool32x4Operation::OpAllTrue<int32>(const SIMDValue& simd);
    template bool SIMDBool32x4Operation::OpAllTrue<int16>(const SIMDValue& simd);
    template bool SIMDBool32x4Operation::OpAllTrue<int8>(const SIMDValue& simd);
    //
    template bool SIMDBool32x4Operation::OpAnyTrue<int64>(const SIMDValue& simd);
    template bool SIMDBool32x4Operation::OpAnyTrue<int32>(const SIMDValue& simd);
    template bool SIMDBool32x4Operation::OpAnyTrue<int16>(const SIMDValue& simd);
    template bool SIMDBool32x4Operation::OpAnyTrue<int8>(const SIMDValue& simd);
}


#endif
