//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {

    struct SIMDInt64x2Operation
    {
        static SIMDValue OpSplat(int64 x);
        static SIMDValue OpAdd(const SIMDValue& a, const SIMDValue& b);
        static SIMDValue OpSub(const SIMDValue& a, const SIMDValue& b);
        static SIMDValue OpNeg(const SIMDValue& a);

        //These are directly called into from JIT as of now. Pointer-based arguments are used to ensure 
        //that calling conventions are consistent across x86/x64 and match call sequences JIT generates.
        //TODO: Change back to "const SIMDValue& a" after getting rid of the jit helpers.
        template<typename T> static void OpTrunc(SIMDValue* dst, SIMDValue* src);
        static void OpShiftLeftByScalar(SIMDValue* dst, SIMDValue* src, int count);
        static void OpShiftRightByScalar(SIMDValue* dst, SIMDValue* src, int count);
        static void OpShiftRightByScalarU(SIMDValue* dst, SIMDValue* src, int count);
        static void OpReplaceLane(SIMDValue* dst, SIMDValue* src, int64 val, uint index);
    };

} // namespace Js
