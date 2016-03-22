//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js {

    struct SIMDInt8x16Operation
    {
        // following are operation wrappers for SIMDInt8x16 general implementation
        // input and output are typically SIMDValue
        static SIMDValue OpInt8x16(int8 values[]);

        static SIMDValue OpSplat(int8 x);

        //// conversion
        static SIMDValue OpFromInt32x4Bits(const SIMDValue& value);
        static SIMDValue OpFromFloat32x4Bits(const SIMDValue& value);

        //// Unary Ops
        static SIMDValue OpNeg(const SIMDValue& v);

        static SIMDValue OpAdd(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpSub(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMul(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMin(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMax(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue);

        static SIMDValue OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);

        static SIMDValue OpShiftLeftByScalar(const SIMDValue& value, int count);
        static SIMDValue OpShiftRightByScalar(const SIMDValue& value, int count);

        //Select
        static SIMDValue OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV);
    };
} // namespace Js
