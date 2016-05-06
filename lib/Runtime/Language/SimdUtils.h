//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#define SIMD128_TYPE_SPEC_FLAG Js::Configuration::Global.flags.Simd128TypeSpec

// The representations below assume little-endian.
#define SIMD_X 0
#define SIMD_Y 1
#define SIMD_Z 2
#define SIMD_W 3
#define FLOAT64_SIZE 8
#define FLOAT32_SIZE 4
#define INT32_SIZE   4
#define INT16_SIZE   2
#define INT8_SIZE    1
#define SIMD_INDEX_VALUE_MAX     5
#define SIMD_STRING_BUFFER_MAX   1024
#define SIMD_DATA     \
    int32   i32[4];\
    int16   i16[8];\
    int8    i8[16];\
    uint32  u32[4];\
    uint16  u16[8];\
    uint8   u8[16];\
    float   f32[4];\
    double  f64[2];
#define SIMD_TEMP_SIZE 3
struct _SIMDValue
{
    union{
        SIMD_DATA
    };

    void SetValue(_SIMDValue value)
    {
        i32[SIMD_X] = value.i32[SIMD_X];
        i32[SIMD_Y] = value.i32[SIMD_Y];
        i32[SIMD_Z] = value.i32[SIMD_Z];
        i32[SIMD_W] = value.i32[SIMD_W];
    }
    void Zero()
    {
        f64[SIMD_X] = f64[SIMD_Y] = 0;
    }
    bool operator==(const _SIMDValue& r)
    {
        // don't compare f64/f32 because NaN bit patterns will not be considered equal.
        return (this->i32[SIMD_X] == r.i32[SIMD_X] &&
            this->i32[SIMD_Y] == r.i32[SIMD_Y] &&
            this->i32[SIMD_Z] == r.i32[SIMD_Z] &&
            this->i32[SIMD_W] == r.i32[SIMD_W]);
    }
    bool IsZero()
    {
        return (i32[SIMD_X] == 0 && i32[SIMD_Y] == 0 && i32[SIMD_Z] == 0 && i32[SIMD_W] == 0);
    }

};
typedef _SIMDValue SIMDValue;

// For dictionary use
template <>
struct DefaultComparer<_SIMDValue>
{
    __forceinline static bool Equals(_SIMDValue x, _SIMDValue y)
    {
        return x == y;
    }

    __forceinline static hash_t GetHashCode(_SIMDValue d)
    {
        return (hash_t)(d.i32[SIMD_X] ^ d.i32[SIMD_Y] ^ d.i32[SIMD_Z] ^ d.i32[SIMD_W]);
    }
};

#if _M_IX86 || _M_AMD64
struct _x86_SIMDValue
{
    union{
        SIMD_DATA
        __m128  m128_value;
        __m128d m128d_value;
        __m128i m128i_value;
    };

    static _x86_SIMDValue ToX86SIMDValue(const SIMDValue& val)
    {
        _x86_SIMDValue result;
        result.m128_value = _mm_loadu_ps((float*) &val);
        return result;
    }

    static SIMDValue ToSIMDValue(const _x86_SIMDValue& val)
    {
        SIMDValue result;
        _mm_storeu_ps((float*) &result, val.m128_value);
        return result;
    }
};

// These global values are 16-byte aligned.
const _x86_SIMDValue X86_ABS_MASK_F4 = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff };
const _x86_SIMDValue X86_ABS_MASK_I4 = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff };
const _x86_SIMDValue X86_ABS_MASK_D2 = { 0xffffffff, 0x7fffffff, 0xffffffff, 0x7fffffff };

const _x86_SIMDValue X86_NEG_MASK_F4 = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
const _x86_SIMDValue X86_NEG_MASK_D2 = { 0x00000000, 0x80000000, 0x00000000, 0x80000000 };

const _x86_SIMDValue X86_ALL_ONES_F4 = { 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000 }; // {1.0, 1.0, 1.0, 1.0}
const _x86_SIMDValue X86_ALL_ONES_I4 = { 0x00000001, 0x00000001, 0x00000001, 0x00000001 }; // {1, 1, 1, 1}
const _x86_SIMDValue X86_ALL_ONES_D2 = { 0x00000000, 0x3ff00000, 0x00000000, 0x3ff00000 }; // {1.0, 1.0}

const _x86_SIMDValue X86_ALL_NEG_ONES = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
const _x86_SIMDValue X86_ALL_NEG_ONES_F4 = { 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000 }; //-1.0, -1.0, -1.0, -1.0

const _x86_SIMDValue X86_ALL_ONES_I8  = { 0x00010001, 0x00010001, 0x00010001, 0x00010001 }; // {1, 1, 1, 1, 1, 1, 1, 1}

const _x86_SIMDValue X86_ALL_ZEROS          = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
const _x86_SIMDValue X86_ALL_ONES_I16       = { 0x01010101, 0x01010101, 0x01010101, 0x01010101 }; // {1, 1, 1, 1,1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
const _x86_SIMDValue X86_LANE0_ONES_I16     = { 0x000000ff, 0x00000000, 0x00000000, 0x00000000 };
const _x86_SIMDValue X86_LOWBYTES_MASK      = { 0x00ff00ff, 0x00ff00ff, 0x00ff00ff, 0x00ff00ff };
const _x86_SIMDValue X86_HIGHBYTES_MASK     = { 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00 };

const _x86_SIMDValue X86_LANE_W_ZEROS = { 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000 };

const _x86_SIMDValue X86_TWO_31_F4          = { 0x4f000000, 0x4f000000, 0x4f000000, 0x4f000000 }; // f32(2^31), ....
const _x86_SIMDValue X86_NEG_TWO_31_F4      = { 0xcf000000, 0xcf000000, 0xcf000000, 0xcf000000 }; // f32(-2^31), ....
const _x86_SIMDValue X86_TWO_32_F4          = { 0x4f800000, 0x4f800000, 0x4f800000, 0x4f800000 }; // f32(2^32), ....
const _x86_SIMDValue X86_TWO_31_I4          = X86_NEG_MASK_F4;                                    // 2^31, ....
const _x86_SIMDValue X86_WORD_SIGNBITS      = { 0x80008000, 0x80008000, 0x80008000, 0x80008000 };
const _x86_SIMDValue X86_DWORD_SIGNBITS     = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
const _x86_SIMDValue X86_BYTE_SIGNBITS      = { 0x80808080, 0x80808080, 0x80808080, 0x80808080 };
const _x86_SIMDValue X86_4LANES_MASKS[]     = {{ 0xffffffff, 0x00000000, 0x00000000, 0x00000000 }, 
                                               { 0x00000000, 0xffffffff, 0x00000000, 0x00000000 },
                                               { 0x00000000, 0x00000000, 0xffffffff, 0x00000000 },
                                               { 0x00000000, 0x00000000, 0x00000000, 0xffffffff }};

// auxiliary SIMD values in memory to help JIT'ed code. E.g. used for Int8x16 shuffle. 
extern _x86_SIMDValue X86_TEMP_SIMD[];

typedef _x86_SIMDValue X86SIMDValue;
CompileAssert(sizeof(X86SIMDValue) == 16);
#endif

typedef SIMDValue     AsmJsSIMDValue; // alias for asmjs
CompileAssert(sizeof(SIMDValue) == 16);

class ValueType;

namespace Js {

    bool IsSimdType(Var aVar);
    uint32 inline SIMDGetShiftAmountMask(uint32 eleSizeInBytes){ return (eleSizeInBytes << 3) - 1; }
    int32 SIMDCheckTypedArrayIndex(ScriptContext* scriptContext, Var index);
    int32 SIMDCheckLaneIndex(ScriptContext* scriptContext, Var lane, const int32 range = 4);

    template <int laneCount = 4>
    SIMDValue SIMD128InnerShuffle(SIMDValue src1, SIMDValue src2, int32 lane0, int32 lane1, int32 lane2, int32 lane3);
    template <int laneCount = 8>
    SIMDValue SIMD128InnerShuffle(SIMDValue src1, SIMDValue src2, const int32* lanes = nullptr);

    template <class SIMDType, int laneCount = 4>
    Var SIMD128SlowShuffle(Var src1, Var src2, Var lane0, Var lane1, Var lane2, Var lane3, int range, ScriptContext* scriptContext);
    template <class SIMDType, int laneCount = 8>
    Var SIMD128SlowShuffle(Var src1, Var src2, Var *lanes, const uint range, ScriptContext* scriptContext);

    //TypeConvert
    template<class SIMDType1, class SIMDType2>
    Var SIMDConvertTypeFromBits(SIMDType1 &instance, ScriptContext& requestContext);

    //Lane Access
    template<class SIMDType, int laneCount, typename T>
    inline T SIMD128ExtractLane(Var src, Var lane, ScriptContext* scriptContext);
    template<class SIMDType, int laneCount, typename T>
    inline SIMDValue SIMD128ReplaceLane(Var src, Var lane, T value, ScriptContext* scriptContext);

    SIMDValue SIMD128InnerReplaceLaneF4(const SIMDValue& src1, const int32 lane, const float value);
    float SIMD128InnerExtractLaneF4(const SIMDValue& src1, const int32 lane);

    SIMDValue SIMD128InnerReplaceLaneI4(const SIMDValue& src1, const int32 lane, const int value);
    int SIMD128InnerExtractLaneI4(const SIMDValue& src1, const int32 lane);

    SIMDValue SIMD128InnerReplaceLaneI8(const SIMDValue& src1, const int32 lane, const int16 value);
    int16 SIMD128InnerExtractLaneI8(const SIMDValue& src1, const int32 lane);

    SIMDValue SIMD128InnerReplaceLaneI16(const SIMDValue& src1, const int32 lane, const int8 value);
    int8 SIMD128InnerExtractLaneI16(const SIMDValue& src1, const int32 lane);

    int32 SIMDCheckInt32Number(ScriptContext* scriptContext, Var value);
    bool        SIMDIsSupportedTypedArray(Var value);
    SIMDValue*  SIMDCheckTypedArrayAccess(Var arg1, Var arg2, TypedArrayBase **tarray, int32 *index, uint32 dataWidth, ScriptContext *scriptContext);
    SIMDValue SIMDLdData(SIMDValue *data, uint8 dataWidth);
    void SIMDStData(SIMDValue *data, SIMDValue simdValue, uint8 dataWidth);

    template <class SIMDType>
    Var SIMD128TypedArrayLoad(Var arg1, Var arg2, uint32 dataWidth, ScriptContext *scriptContext)
    {
        Assert(dataWidth >= 4 && dataWidth <= 16);

        TypedArrayBase *tarray = NULL;
        int32 index = -1;
        SIMDValue* data = NULL;

        data = SIMDCheckTypedArrayAccess(arg1, arg2, &tarray, &index, dataWidth, scriptContext);

        Assert(tarray != NULL);
        Assert(index >= 0);
        Assert(data != NULL);

        SIMDValue result = SIMDLdData(data, (uint8)dataWidth);

        return SIMDType::New(&result, scriptContext);

    }

    template <class SIMDType>
    void SIMD128TypedArrayStore(Var arg1, Var arg2, Var simdVar, uint32 dataWidth, ScriptContext *scriptContext)
    {
        Assert(dataWidth >= 4 && dataWidth <= 16);

        TypedArrayBase *tarray = NULL;
        int32 index = -1;
        SIMDValue* data = NULL;

        data = SIMDCheckTypedArrayAccess(arg1, arg2, &tarray, &index, dataWidth, scriptContext);

        Assert(tarray != NULL);
        Assert(index >= 0);
        Assert(data != NULL);

        SIMDValue simdValue = SIMDType::FromVar(simdVar)->GetValue();
        SIMDStData(data, simdValue, (uint8)dataWidth);
    }

    //SIMD Type conversion
    SIMDValue FromSimdBits(SIMDValue value);

    template<class SIMDType1, class SIMDType2>
    Var SIMDConvertTypeFromBits(SIMDType1* instance, ScriptContext* requestContext)
    {
        SIMDValue result = FromSimdBits(instance->GetValue());
        return SIMDType2::New(&result, requestContext);
    }

    enum class OpCode : ushort;
    uint32 SimdOpcodeAsIndex(Js::OpCode op);

}
