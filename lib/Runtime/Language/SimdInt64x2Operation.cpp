//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

namespace Js
{
    SIMDValue SIMDInt64x2Operation::OpSplat(int64 val)
    {
        SIMDValue result;
        result.i64[0] = val;
        result.i64[1] = val;
        return result;
    }

    SIMDValue SIMDInt64x2Operation::OpAdd(const SIMDValue& a, const SIMDValue& b)
    {
        SIMDValue result;
        result.i64[0] = a.i64[0] + b.i64[0];
        result.i64[1] = a.i64[1] + b.i64[1];
        return result;
    }

    SIMDValue SIMDInt64x2Operation::OpSub(const SIMDValue& a, const SIMDValue& b)
    {
        SIMDValue result;
        result.i64[0] = a.i64[0] - b.i64[0];
        result.i64[1] = a.i64[1] - b.i64[1];
        return result;
    }

    SIMDValue SIMDInt64x2Operation::OpNeg(const SIMDValue& a)
    {
        SIMDValue result;
        result.i64[0] = -a.i64[0];
        result.i64[1] = -a.i64[1];
        return result;
    }

    static bool IsInRange(double val, uint64& out)
    {
        if (val != val || val <= (double)0)
        {
            out = 0;
            return false;
        }

        if (val >= (double)ULLONG_MAX)
        {
            out = ULLONG_MAX;
            return false;
        }

        return true;
    }

    static bool IsInRange(double val, int64& out)
    {
        if (val != val)
        {
            out = 0;
            return false;
        }

        if (val <= (double)LLONG_MIN)
        {
            out = LLONG_MIN;
            return false;
        }

        if (val >= (double)LLONG_MAX)
        {
            out = LLONG_MAX;
            return false;
        }

        return true;
    }

    template<typename T>
    void SIMDInt64x2Operation::OpTrunc(SIMDValue* dst, SIMDValue* src)
    {
        T convertedVal;
        dst->i64[0] = IsInRange(src->f64[0], convertedVal) ? (T)src->f64[0] : convertedVal;
        dst->i64[1] = IsInRange(src->f64[1], convertedVal) ? (T)src->f64[1] : convertedVal;
    }

    template void SIMDInt64x2Operation::OpTrunc<int64>(SIMDValue* dst, SIMDValue* src);
    template void SIMDInt64x2Operation::OpTrunc<uint64>(SIMDValue* dst, SIMDValue* src);

    void SIMDInt64x2Operation::OpShiftLeftByScalar(SIMDValue* dst, SIMDValue* src, int count)
    {
        count = count & SIMDUtils::SIMDGetShiftAmountMask(8);
        dst->i64[0] = src->i64[0] << count;
        dst->i64[1] = src->i64[1] << count;
    }

    void SIMDInt64x2Operation::OpShiftRightByScalar(SIMDValue* dst, SIMDValue* src, int count)
    {
        count = count & SIMDUtils::SIMDGetShiftAmountMask(8);
        dst->i64[0] = src->i64[0] >> count;
        dst->i64[1] = src->i64[1] >> count;
    }

    void SIMDInt64x2Operation::OpShiftRightByScalarU(SIMDValue* dst, SIMDValue* src, int count)
    {
        count = count & SIMDUtils::SIMDGetShiftAmountMask(8);
        dst->i64[0] = (uint64)src->i64[0] >> count;
        dst->i64[1] = (uint64)src->i64[1] >> count;
    }

    void SIMDInt64x2Operation::OpReplaceLane(SIMDValue* dst, SIMDValue* src, int64 val, uint index)
    {
        dst->SetValue(*src);
        dst->i64[index] = val;
    }
}
