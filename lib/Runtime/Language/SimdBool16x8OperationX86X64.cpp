//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if _M_IX86 || _M_AMD64
namespace Js
{
    SIMDValue SIMDBool16x8Operation::OpBool16x8(bool b[])
    {
        X86SIMDValue x86Result;
        x86Result.m128i_value = _mm_set_epi16(b[7] * -1, b[6] * -1, b[5] * -1, b[4] * -1, b[3] * -1, b[2] * -1, b[1] * -1, b[0] * -1);
        return X86SIMDValue::ToSIMDValue(x86Result);
    }

    SIMDValue SIMDBool16x8Operation::OpBool16x8(const SIMDValue& v)
    {
        // overload function with input parameter as SIMDValue for completeness
        SIMDValue result;
        result = v;
        return result;
    }
}
#endif
