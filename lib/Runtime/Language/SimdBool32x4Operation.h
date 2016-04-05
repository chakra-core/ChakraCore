//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {

    struct SIMDBool32x4Operation
    {
        // following are operation wrappers of SIMD.Bool32x4 general implementation
        static SIMDValue OpBool32x4(bool x, bool y, bool z, bool w);
        static SIMDValue OpBool32x4(const SIMDValue& v);

        // Unary Ops
        static SIMDValue OpNot(const SIMDValue& v);
        static bool OpAnyTrue(const SIMDValue& v);
        static bool OpAllTrue(const SIMDValue& v);

        // Binary Ops
        // Done via Int32x4 ops
    };

} // namespace Js
