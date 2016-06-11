//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"
#if defined(_M_ARM32_OR_ARM64)

namespace Js
{
    SIMDValue SIMDInt16x8Operation::OpInt16x8(int16 values[])
    {
        SIMDValue result;
        for (uint i = 0; i < 8; i ++)
        { 
            result.i16[i] = values[i];
        }
        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpSplat(int16 x)
    {
        SIMDValue result;

        result.i16[0] = result.i16[1] = result.i16[2] = result.i16[3] = x;
        result.i16[4] = result.i16[5] = result.i16[6] = result.i16[7] = x;

        return result;
    }

    // Unary Ops
    SIMDValue SIMDInt16x8Operation::OpNeg(const SIMDValue& value)
    {
        SIMDValue result;

        result.i16[0] = -1 * value.i16[0];
        result.i16[1] = -1 * value.i16[1];
        result.i16[2] = -1 * value.i16[2];
        result.i16[3] = -1 * value.i16[3];
        result.i16[4] = -1 * value.i16[4];
        result.i16[5] = -1 * value.i16[5];
        result.i16[6] = -1 * value.i16[6];
        result.i16[7] = -1 * value.i16[7];

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpNot(const SIMDValue& value)
    {
        SIMDValue result;

        result.i16[0] = ~(value.i16[0]);
        result.i16[1] = ~(value.i16[1]);
        result.i16[2] = ~(value.i16[2]);
        result.i16[3] = ~(value.i16[3]);
        result.i16[4] = ~(value.i16[4]);
        result.i16[5] = ~(value.i16[5]);
        result.i16[6] = ~(value.i16[6]);
        result.i16[7] = ~(value.i16[7]);

        return result;
    }

    // Binary Ops
    SIMDValue SIMDInt16x8Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        result.i16[0] = aValue.i16[0] + bValue.i16[0];
        result.i16[1] = aValue.i16[1] + bValue.i16[1];
        result.i16[2] = aValue.i16[2] + bValue.i16[2];
        result.i16[3] = aValue.i16[3] + bValue.i16[3];
        result.i16[4] = aValue.i16[4] + bValue.i16[4];
        result.i16[5] = aValue.i16[5] + bValue.i16[5];
        result.i16[6] = aValue.i16[6] + bValue.i16[6];
        result.i16[7] = aValue.i16[7] + bValue.i16[7];

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        result.i16[0] = aValue.i16[0] - bValue.i16[0];
        result.i16[1] = aValue.i16[1] - bValue.i16[1];
        result.i16[2] = aValue.i16[2] - bValue.i16[2];
        result.i16[3] = aValue.i16[3] - bValue.i16[3];
        result.i16[4] = aValue.i16[4] - bValue.i16[4];
        result.i16[5] = aValue.i16[5] - bValue.i16[5];
        result.i16[6] = aValue.i16[6] - bValue.i16[6];
        result.i16[7] = aValue.i16[7] - bValue.i16[7];

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        result.i16[0] = aValue.i16[0] * bValue.i16[0];
        result.i16[1] = aValue.i16[1] * bValue.i16[1];
        result.i16[2] = aValue.i16[2] * bValue.i16[2];
        result.i16[3] = aValue.i16[3] * bValue.i16[3];
        result.i16[4] = aValue.i16[4] * bValue.i16[4];
        result.i16[5] = aValue.i16[5] * bValue.i16[5];
        result.i16[6] = aValue.i16[6] * bValue.i16[6];
        result.i16[7] = aValue.i16[7] * bValue.i16[7];

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpAnd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        result.i16[0] = aValue.i16[0] & bValue.i16[0];
        result.i16[1] = aValue.i16[1] & bValue.i16[1];
        result.i16[2] = aValue.i16[2] & bValue.i16[2];
        result.i16[3] = aValue.i16[3] & bValue.i16[3];
        result.i16[4] = aValue.i16[4] & bValue.i16[4];
        result.i16[5] = aValue.i16[5] & bValue.i16[5];
        result.i16[6] = aValue.i16[6] & bValue.i16[6];
        result.i16[7] = aValue.i16[7] & bValue.i16[7];

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpOr(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        result.i16[0] = aValue.i16[0] | bValue.i16[0];
        result.i16[1] = aValue.i16[1] | bValue.i16[1];
        result.i16[2] = aValue.i16[2] | bValue.i16[2];
        result.i16[3] = aValue.i16[3] | bValue.i16[3];
        result.i16[4] = aValue.i16[4] | bValue.i16[4];
        result.i16[5] = aValue.i16[5] | bValue.i16[5];
        result.i16[6] = aValue.i16[6] | bValue.i16[6];
        result.i16[7] = aValue.i16[7] | bValue.i16[7];

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpXor(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        result.i16[0] = aValue.i16[0] ^ bValue.i16[0];
        result.i16[1] = aValue.i16[1] ^ bValue.i16[1];
        result.i16[2] = aValue.i16[2] ^ bValue.i16[2];
        result.i16[3] = aValue.i16[3] ^ bValue.i16[3];
        result.i16[4] = aValue.i16[4] ^ bValue.i16[4];
        result.i16[5] = aValue.i16[5] ^ bValue.i16[5];
        result.i16[6] = aValue.i16[6] ^ bValue.i16[6];
        result.i16[7] = aValue.i16[7] ^ bValue.i16[7];

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        int mask = 0x8000;
        for (uint idx = 0; idx < 8; ++idx)
        {
            int16 val1 = aValue.i16[idx];
            int16 val2 = bValue.i16[idx];
            int16 sum = val1 + val2;

            result.i16[idx] = sum;
            if (val1 > 0 && val2 > 0 && sum < 0)
            {
                result.i16[idx] = 0x7fff;
            }
            else if (val1 < 0 && val2 < 0 && sum > 0)
            {
                result.i16[idx] = static_cast<int16>(mask);
            }
        }
        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        int mask = 0x8000;
        for (uint idx = 0; idx < 8; ++idx)
        {
            int16 val1 = aValue.i16[idx];
            int16 val2 = bValue.i16[idx];
            int16 diff = val1 + val2;

            result.i16[idx] = static_cast<int16>(diff);
            if (diff > 0x7fff)
                result.i16[idx] = 0x7fff;
            else if (diff < 0x8000)
                result.i16[idx] = static_cast<int16>(mask);
        }
        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        for (int idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] < bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }
        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (int idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] > bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }

        return result;
    }

    // compare ops
    SIMDValue SIMDInt16x8Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] < bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] <= bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] == bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] != bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] > bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 8; ++idx)
        {
            result.i16[idx] = (aValue.i16[idx] >= bValue.i16[idx]) ? aValue.i16[idx] : bValue.i16[idx];
        }

        return result;
    }

    // ShiftOps
    SIMDValue SIMDInt16x8Operation::OpShiftLeftByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(2);

        result.i16[0] = value.i16[0] << count;
        result.i16[1] = value.i16[1] << count;
        result.i16[2] = value.i16[2] << count;
        result.i16[3] = value.i16[3] << count;
        result.i16[4] = value.i16[4] << count;
        result.i16[5] = value.i16[5] << count;
        result.i16[6] = value.i16[6] << count;
        result.i16[7] = value.i16[7] << count;

        return result;
    }

    SIMDValue SIMDInt16x8Operation::OpShiftRightByScalar(const SIMDValue& value, int count)
    {
        SIMDValue result;

        count = count & SIMDUtils::SIMDGetShiftAmountMask(2);

        result.i16[0] = value.i16[0] >> count;
        result.i16[1] = value.i16[1] >> count;
        result.i16[2] = value.i16[2] >> count;
        result.i16[3] = value.i16[3] >> count;
        result.i16[4] = value.i16[4] >> count;
        result.i16[5] = value.i16[5] >> count;
        result.i16[6] = value.i16[6] >> count;
        result.i16[7] = value.i16[7] >> count;

        return result;
    }

}

#endif
