//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

namespace Js
{
    SIMDValue SIMDUint16x8Operation::OpUint16x8(uint16 x0,  uint16 x1,  uint16 x2,  uint16 x3
        , uint16 x4,  uint16 x5,  uint16 x6,  uint16 x7)
    {
        SIMDValue result;

        result.u16[0]  = x0;
        result.u16[1]  = x1;
        result.u16[2]  = x2;
        result.u16[3]  = x3;
        result.u16[4]  = x4;
        result.u16[5]  = x5;
        result.u16[6]  = x6;
        result.u16[7]  = x7;

        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for(uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = aValue.u16[idx] * bValue.u16[idx];
        }

        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (aValue.u16[idx] < bValue.u16[idx]) ? aValue.u16[idx] : bValue.u16[idx];
        }

        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (aValue.u16[idx] > bValue.u16[idx]) ? aValue.u16[idx] : bValue.u16[idx];
        }

        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue) //arun::ToDo return bool types
    {
        SIMDValue result;

        for(uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (aValue.u16[idx] < bValue.u16[idx]) ? 0xff : 0x0;
        }
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue) //arun::ToDo return bool types
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (aValue.u16[idx] <= bValue.u16[idx]) ? 0xff : 0x0;
        }
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        if (count > 16)   //Similar to polyfill, maximum shift will happen if the shift amounts and invalid
        {
            count = 16;   
        }

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.u16[idx] = (value.u16[idx] >> count);
        }
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            uint32 a = (uint32)aValue.u16[idx];
            uint32 b = (uint32)bValue.u16[idx];

            result.u16[idx] = ((a + b) > MAXUINT16) ? MAXUINT16 : (uint16)(a + b);
        }
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            int a = (int)aValue.u16[idx];
            int b = (int)bValue.u16[idx];

            result.u16[idx] = ((a - b) < 0) ? 0 : (uint16)(a - b);
        }
        return result;
    }
}

#endif
