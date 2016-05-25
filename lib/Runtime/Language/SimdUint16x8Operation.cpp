//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

namespace Js
{
    SIMDValue SIMDUint16x8Operation::OpUint16x8(uint16 values[])
    {
        SIMDValue result;

        for (uint i = 0; i < 8; i++)
        {
            result.u16[i] = values[i];
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

    SIMDValue SIMDUint16x8Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        result = SIMDUint16x8Operation::OpLessThan(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        result = SIMDUint16x8Operation::OpLessThanOrEqual(aValue, bValue);
        result = SIMDInt32x4Operation::OpNot(result);
        return result;
    }

    SIMDValue SIMDUint16x8Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(2);

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
