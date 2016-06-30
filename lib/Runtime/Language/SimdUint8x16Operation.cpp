//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

namespace Js
{
    SIMDValue SIMDUint8x16Operation::OpUint8x16(uint8 values[])
    {
        SIMDValue result;
        for (uint i = 0; i < 16; i++)
        {
            result.u8[i] = values[i];
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
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(1);

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
