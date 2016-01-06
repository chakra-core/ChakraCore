//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#if defined(_M_ARM32_OR_ARM64)

namespace Js
{
    SIMDValue SIMDInt8x16Operation::OpInt8x16(int8 x0,  int8 x1,  int8 x2,  int8 x3
        , int8 x4,  int8 x5,  int8 x6,  int8 x7
        , int8 x8,  int8 x9,  int8 x10, int8 x11
        , int8 x12, int8 x13, int8 x14, int8 x15)
    {
        SIMDValue result;

        result.i8[0]  = x0;
        result.i8[1]  = x1;
        result.i8[2]  = x2;
        result.i8[3]  = x3;
        result.i8[4]  = x4;
        result.i8[5]  = x5;
        result.i8[6]  = x6;
        result.i8[7]  = x7;
        result.i8[8]  = x8;
        result.i8[9]  = x9;
        result.i8[10] = x10;
        result.i8[11] = x11;
        result.i8[12] = x12;
        result.i8[13] = x13;
        result.i8[14] = x14;
        result.i8[15] = x15;

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpSplat(int8 x)
    {
        SIMDValue result;

        result.i8[0] = result.i8[1] = result.i8[2] = result.i8[3] = result.i8[4] = result.i8[5] = result.i8[6]= result.i8[7] = result.i8[8] = result.i8[9]= result.i8[10] = result.i8[11] = result.i8[12]= result.i8[13] = result.i8[14] = result.i8[15] = x;

        return result;
    }

    //// Unary Ops
    SIMDValue SIMDInt8x16Operation::OpNeg(const SIMDValue& value)
    {
        SIMDValue result;

        result.i8[0]  = -1 * value.i8[0];
        result.i8[1]  = -1 * value.i8[1];
        result.i8[2]  = -1 * value.i8[2];
        result.i8[3]  = -1 * value.i8[3];
        result.i8[4]  = -1 * value.i8[4];
        result.i8[5]  = -1 * value.i8[5];
        result.i8[6]  = -1 * value.i8[6];
        result.i8[7]  = -1 * value.i8[7];
        result.i8[8]  = -1 * value.i8[8];
        result.i8[9]  = -1 * value.i8[9];
        result.i8[10] = -1 * value.i8[10];
        result.i8[11] = -1 * value.i8[11];
        result.i8[12] = -1 * value.i8[12];
        result.i8[13] = -1 * value.i8[13];
        result.i8[14] = -1 * value.i8[14];
        result.i8[15] = -1 * value.i8[15];

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpAdd(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = aValue.i8[idx] + bValue.i8[idx];
        }
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpSub(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = aValue.i8[idx] - bValue.i8[idx];
        }

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpMul(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = aValue.i8[idx] * bValue.i8[idx];
        }

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        int mask = 0x80;
        for (uint idx = 0; idx < 16; ++idx)
        {
            int8 val1 = aValue.i8[idx];
            int8 val2 = bValue.i8[idx];
            int8 sum = val1 + val2;

            result.i8[idx] = sum;
            if (val1 > 0 && val2 > 0 && sum < 0)
                result.i8[idx] = 0x7F;
            else if (val1 < 0 && val2 < 0 && sum > 0)
                result.i8[idx] = static_cast<int8>(mask);
        }
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;
        int mask = 0x80;
        for (uint idx = 0; idx < 16; ++idx)
        {
            int8 val1 = aValue.i8[idx];
            int8 val2 = bValue.i8[idx];
            int16 diff = val1 + val2;

            result.i8[idx] = static_cast<int8>(diff);
            if (diff > 0x7F)
                result.i8[idx] = 0x7F;
            else if (diff < 0x80)
                result.i8[idx] = static_cast<int8>(mask);
        }
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpMin(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] < bValue.i8[idx]) ? aValue.i8[idx] : bValue.i8[idx];
        }

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpMax(const SIMDValue& aValue, const SIMDValue& bValue)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] > bValue.i8[idx]) ? aValue.i8[idx] : bValue.i8[idx];
        }

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue) // TODO arun: return bool types
    {
        SIMDValue result;

        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] < bValue.i8[idx]) ? 0xff : 0x0;
        }
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue) //arun::ToDo return bool types
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] <= bValue.i8[idx]) ? 0xff : 0x0;
        }
        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpEqual(const SIMDValue& aValue, const SIMDValue& bValue) // TODO arun: return bool types
    {
        SIMDValue result;

        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] == bValue.i8[idx]) ? 0xff : 0x0;
        }

        return result;
    }


    SIMDValue SIMDInt8x16Operation::OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue) //arun::ToDo return bool types
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] != bValue.i8[idx]) ? 0xff : 0x0;
        }

        return result;
    }


    SIMDValue SIMDInt8x16Operation::OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue) //arun::ToDo return bool types
    {
        SIMDValue result;

        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] > bValue.i8[idx]) ? 0xff : 0x0; //Return should be bool vector according to spec
        }

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue) //arun::ToDo return bool types
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (aValue.i8[idx] >= bValue.i8[idx]) ? 0xff : 0x0; //Return should be bool vector accordig to spec
        }

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpShiftLeftByScalar(const SIMDValue& value, int8 count)
    {
        SIMDValue result;
        if (count < 0 || count > 8) // Similar to polyfill, maximum shift will happen if the shift amounts and invalid
        {
            count = 8;
        }

        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = value.i8[idx] << count;
        }

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpShiftRightByScalar(const SIMDValue& value, int8 count)
    {
        SIMDValue result;

        if (count < 0 || count > 8) // Similar to polyfill, maximum shift will happen if the shift amounts and invalid
        {
            count = 8;
        }

        for(uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = value.i8[idx] >> count;
        }

        return result;
    }

    SIMDValue SIMDInt8x16Operation::OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV)
    {
        SIMDValue result;

        for (uint idx = 0; idx < 16; ++idx)
        {
            result.i8[idx] = (mV.i8[idx] == 1) ? tV.i8[idx] : fV.i8[idx];
        }
        return result;
    }

}
#endif
