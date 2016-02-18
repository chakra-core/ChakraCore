//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {

    struct SIMDBool16x8Operation
    {
        // following are operation wrappers of SIMD.Bool16x8 general implementation
        static SIMDValue OpBool16x8(bool b[]);
        static SIMDValue OpBool16x8(const SIMDValue& v);

        static SIMDValue OpSplat(bool x);
        static SIMDValue OpSplat(const SIMDValue& v);

        // Unary Ops
        // Done via Int16x8 ops and Bool32x4

        // Binary Ops
        // Done via Int16x8 ops
    };

} // namespace Js
