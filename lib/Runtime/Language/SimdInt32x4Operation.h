//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js {

    struct SIMDInt32x4Operation
    {
        // following are operation wrappers for SIMDInt32x4 general implementation
        // input and output are typically SIMDValue
        static SIMDValue OpInt32x4(int x, int y, int z, int w);

        static SIMDValue OpSplat(int x);

        static SIMDValue OpBool(int x, int y, int z, int w);
        static SIMDValue OpBool(const SIMDValue& v);

        // conversion
        static SIMDValue OpFromFloat32x4(const SIMDValue& value, bool &throws);
        static SIMDValue OpFromFloat64x2(const SIMDValue& value);

        // Unary Ops
        static SIMDValue OpAbs(const SIMDValue& v);
        static SIMDValue OpNeg(const SIMDValue& v);
        static SIMDValue OpNot(const SIMDValue& v);

        static SIMDValue OpAdd(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpSub(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMul(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpAnd(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpOr (const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpXor(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMin(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMax(const SIMDValue& aValue, const SIMDValue& bValue);

        static SIMDValue OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);

        static SIMDValue OpShiftLeftByScalar(const SIMDValue& value, int count);
        static SIMDValue OpShiftRightByScalar(const SIMDValue& value, int count);

        static SIMDValue OpSelect(const SIMDValue& mV, const SIMDValue& tV, const SIMDValue& fV);
    };

} // namespace Js
