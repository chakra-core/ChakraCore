//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js {

    struct SIMDUint32x4Operation
    {
        // following are operation wrappers for SIMDUInt32x4 general implementation
        // input and output are typically SIMDValue
        static SIMDValue OpUint32x4(unsigned int x, unsigned int y, unsigned int z, unsigned int w);

        static SIMDValue OpSplat(unsigned int x);

        // conversion
        static SIMDValue OpFromFloat32x4(const SIMDValue& value, bool& throws);
        static SIMDValue OpFromInt32x4(const SIMDValue& value);


        // Unary Ops
        static SIMDValue OpShiftRightByScalar(const SIMDValue& value, int count);

        static SIMDValue OpLessThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpLessThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);

        static SIMDValue OpGreaterThan(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpGreaterThanOrEqual(const SIMDValue& aValue, const SIMDValue& bValue);


        
        static SIMDValue OpMin(const SIMDValue& aValue, const SIMDValue& bValue);
        static SIMDValue OpMax(const SIMDValue& aValue, const SIMDValue& bValue);

    };

} // namespace Js
