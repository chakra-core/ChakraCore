//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js {

    struct SIMDUint16x8Operation
    {
        // following are operation wrappers for SIMDUint16x8 general implementation
        // input and output are typically SIMDValue
        static SIMDValue OpUint16x8(uint16 x0, uint16 x1, uint16 x2, uint16 x3, uint16 x4, uint16 x5, uint16 x6, uint16 x7);

        //// Unary Ops
        static SIMDValue OpMul(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMin(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMax(const SIMDValue& aValue, const SIMDValue& bValue);

        static SIMDValue OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);

        static SIMDValue OpShiftRightByScalar(const SIMDValue& value, int count);

        static SIMDValue OpAddSaturate(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpSubSaturate(const SIMDValue& aValue, const SIMDValue& bValue);
    };

} // namespace Js
