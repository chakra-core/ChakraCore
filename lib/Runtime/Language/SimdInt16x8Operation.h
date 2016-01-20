//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#pragma once

namespace Js {

    struct SIMDInt16x8Operation
    {
        // following are operation wrappers for SIMDInt16x8 general implementation
        // input and output are typically SIMDValue
        static SIMDValue OpInt16x8(int16 values[]);

        static SIMDValue OpSplat(int16 x);

        // Unary Ops
        static SIMDValue OpNeg(const SIMDValue& v);
        static SIMDValue OpNot(const SIMDValue& v);

        // Binary Ops
        static SIMDValue OpAdd(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpSub(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMul(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpAnd(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpOr (const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpXor(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMin(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMax(const SIMDValue& aValue, const SIMDValue& bValue);

        // CompareOps
        static SIMDValue OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpNotEqual(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);

        // ShiftOps
        static SIMDValue OpShiftLeftByScalar(const SIMDValue& value, int count);
        static SIMDValue OpShiftRightByScalar(const SIMDValue& value, int count);

        // load&store
        //static SIMDValue OpLoad(int* v, const int index);
        //static SIMDValue OpStore(int* v, const int index);

    };

} // namespace Js
