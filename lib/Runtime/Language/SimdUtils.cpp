//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

namespace Js
{
#if _M_IX86 || _M_AMD64
    SIMDValue SIMDUtils::FromSimdBits(const SIMDValue value)
    {
        X86SIMDValue x86Result;
        X86SIMDValue v = X86SIMDValue::ToX86SIMDValue(value);

        _mm_store_ps(x86Result.f32, v.m128_value);

        return X86SIMDValue::ToSIMDValue(x86Result);
    }
#else
    SIMDValue SIMDUtils::FromSimdBits(const SIMDValue value)
    {
        SIMDValue result;
        result.i32[SIMD_X] = value.i32[SIMD_X];
        result.i32[SIMD_Y] = value.i32[SIMD_Y];
        result.i32[SIMD_Z] = value.i32[SIMD_Z];
        result.i32[SIMD_W] = value.i32[SIMD_W];
        return result;
    }
#endif

    SIMDValue SIMDUtils::SIMD128InnerShuffle(const SIMDValue src1, const SIMDValue src2, uint32 laneCount, const uint32* lanes)
    {
        SIMDValue result = { 0 };
        Assert(laneCount == 16 || laneCount == 8 || laneCount == 4);
        Assert(lanes != nullptr);
        switch (laneCount)
        {
        case 4:
            for (uint i = 0; i < laneCount; ++i)
            {
                result.i32[i] = lanes[i] < laneCount ? src1.i32[lanes[i]] : src2.i32[lanes[i] - laneCount];
            }
            break;
        case 8:
            for (uint i = 0; i < laneCount; ++i)
            {
                result.i16[i] = lanes[i] < laneCount ? src1.i16[lanes[i]] : src2.i16[lanes[i] - laneCount];
            }
            break;
        case 16:
            for (uint i = 0; i < laneCount; ++i)
            {
                result.i8[i] = lanes[i] < laneCount ? src1.i8[lanes[i]] : src2.i8[lanes[i] - laneCount];
            }
            break;
        default:
            Assert(UNREACHED);
        }
        return result;
    }

    SIMDValue SIMDUtils::SIMDLdData(const SIMDValue *data, uint8 dataWidth)
    {
        SIMDValue result = { 0, 0, 0, 0 };
        // bitwise copy. Always use integer fields to avoid wrong copy of NaNs.
        switch (dataWidth)
        {
        case 16:
            result.i32[SIMD_W] = data->i32[SIMD_W];
            // fall through
        case 12:
            result.i32[SIMD_Z] = data->i32[SIMD_Z];
            // fall through
        case 8:
            result.i32[SIMD_Y] = data->i32[SIMD_Y];
            // fall through
        case 4:
            result.i32[SIMD_X] = data->i32[SIMD_X];
            break;
        default:
            Assert(UNREACHED);
        }
        return result;
    }

    void SIMDUtils::SIMDStData(SIMDValue *data, const SIMDValue simdValue, uint8 dataWidth)
    {
        // bitwise copy. Always use integer fields to avoid wrong copy of NaNs.
        switch (dataWidth)
        {
        case 16:
            data->i32[SIMD_W] = simdValue.i32[SIMD_W];
            // fall through
        case 12:
            data->i32[SIMD_Z] = simdValue.i32[SIMD_Z];
            // fall through
        case 8:
            data->i32[SIMD_Y] = simdValue.i32[SIMD_Y];
            // fall through
        case 4:
            data->i32[SIMD_X] = simdValue.i32[SIMD_X];
            break;
        default:
            Assert(UNREACHED);
        }
    }

#if ENABLE_NATIVE_CODEGEN
    // Maps Simd opcodes which are non-contiguous to a zero-based linear space. Used to index a table using a Simd opcode.
    uint32 SIMDUtils::SimdOpcodeAsIndex(Js::OpCode op)
    {
        if (op <= Js::OpCode::Simd128_End)
        {
            return (uint32)((Js::OpCode)op - Js::OpCode::Simd128_Start);
        }
        else
        {
            Assert(op >= Js::OpCode::Simd128_Start_Extend && op <= Js::OpCode::Simd128_End_Extend);
            return (uint32)((Js::OpCode)op - Js::OpCode::Simd128_Start_Extend) + (uint32)(Js::OpCode::Simd128_End - Js::OpCode::Simd128_Start) + 1;
        }
    }
#endif
}
