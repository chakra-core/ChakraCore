//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

namespace Js
{
    SIMDValue SIMDBool16x8Operation::OpBool16x8(bool b[])
    {
        SIMDValue result;
        for (uint i = 0; i < 8; i++)
        {
            result.i16[i] = b[i] ? -1 : 0;
        }

        return result;
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
