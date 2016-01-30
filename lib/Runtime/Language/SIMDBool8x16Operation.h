//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {

    struct SIMDBool8x16Operation
    {
        // following are operation wrappers of SIMD.Bool8x16 general implementation
        static SIMDValue OpBool8x16(bool b[]);
        static SIMDValue OpBool8x16(const SIMDValue& v);

        static SIMDValue OpSplat(bool x);
        static SIMDValue OpSplat(const SIMDValue& v);

        // Unary Ops
        // Done via Int8x16 and Bool32x4

        // Binary Ops
        // Done via Int8x16 ops
    };

} // namespace Js
