//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

namespace Js
{
    SIMDValue SIMDUint8x16Operation::OpUint8x16(uint8 x0, uint8 x1, uint8 x2, uint8 x3
        , uint8 x4, uint8 x5, uint8 x6, uint8 x7
        , uint8 x8, uint8 x9, uint8 x10, uint8 x11
        , uint8 x12, uint8 x13, uint8 x14, uint8 x15)
    {
        SIMDValue result;

        result.u8[0] = x0;
        result.u8[1] = x1;
        result.u8[2] = x2;
        result.u8[3] = x3;
        result.u8[4] = x4;
        result.u8[5] = x5;
        result.u8[6] = x6;
        result.u8[7] = x7;
        result.u8[8] = x8;
        result.u8[9] = x9;
        result.u8[10] = x10;
        result.u8[11] = x11;
        result.u8[12] = x12;
        result.u8[13] = x13;
        result.u8[14] = x14;
        result.u8[15] = x15;

        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = aValue.u8[idx] * bValue.u8[idx];
        }

        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (aValue.u8[idx] < bValue.u8[idx]) ? aValue.u8[idx] : bValue.u8[idx];
        }

        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (aValue.u8[idx] > bValue.u8[idx]) ? aValue.u8[idx] : bValue.u8[idx];
        }

        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue) //arun::ToDo return bool types
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (aValue.u8[idx] < bValue.u8[idx]) ? 0xff : 0x0;
        }
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue) //arun::ToDo return bool types
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (aValue.u8[idx] <= bValue.u8[idx]) ? 0xff : 0x0;
        }
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        if (count > 8)   //Similar to polyfill, maximum shift will happen if the shift amounts and invalid
        {
            count = 8;
        }

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.u8[idx] = (value.u8[idx] >> count);
        }
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            uint16 a = (uint16)aValue.u8[idx];
            uint16 b = (uint16)bValue.u8[idx];

            result.u8[idx] = ((a + b) > MAXUINT8) ? MAXUINT8 : (uint8)(a + b);
        }
        return result;
    }

    SIMDValue SIMDUint8x16Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            int a = (int)aValue.u8[idx];
            int b = (int)bValue.u8[idx];

            result.u8[idx] = ((a - b) < 0) ? 0 : (uint8)(a - b);
        }
        return result;
    }
}

#endif
