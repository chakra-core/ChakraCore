//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Default all macros to nothing
#ifndef ASMJS_MATH_FUNC_NAMES
#define ASMJS_MATH_FUNC_NAMES(name, propertyName, funcInfo)
#endif

#ifndef ASMJS_MATH_CONST_NAMES
#define ASMJS_MATH_CONST_NAMES(name, propertyName, value)
#endif

#ifndef ASMJS_MATH_DOUBLE_CONST_NAMES
#define ASMJS_MATH_DOUBLE_CONST_NAMES(name, propertyName, value) ASMJS_MATH_CONST_NAMES(name, propertyName, value)
#endif

#ifndef ASMJS_ARRAY_NAMES
#define ASMJS_ARRAY_NAMES(name, propertyName)
#endif

#ifndef ASMJS_TYPED_ARRAY_NAMES
#define ASMJS_TYPED_ARRAY_NAMES(name, propertyName) ASMJS_ARRAY_NAMES(name, propertyName)
#endif

// ASMJS_SIMD_NAMES includes all constructors and operations
#ifdef ASMJS_SIMD_NAMES
#define ASMJS_SIMD_C_NAMES(name, propertyName, libName, entryPoint) ASMJS_SIMD_NAMES(name, propertyName, libName, entryPoint)
#define ASMJS_SIMD_O_NAMES(name, propertyName, libName, entryPoint) ASMJS_SIMD_NAMES(name, propertyName, libName, entryPoint)
#else

#define ASMJS_SIMD_NAMES(name, propertyName, libName, entryPoint)

#ifndef ASMJS_SIMD_C_NAMES
#define ASMJS_SIMD_C_NAMES(name, propertyName, libName, entryPoint)
#endif

#ifndef ASMJS_SIMD_O_NAMES
#define ASMJS_SIMD_O_NAMES(name, propertyName, libName, entryPoint)
#endif

#endif

#ifndef ASMJS_SIMD_MARKERS
#define ASMJS_SIMD_MARKERS(name)
#endif

ASMJS_MATH_FUNC_NAMES(sin,      sin,    Math::EntryInfo::Sin    )
ASMJS_MATH_FUNC_NAMES(cos,      cos,    Math::EntryInfo::Cos    )
ASMJS_MATH_FUNC_NAMES(tan,      tan,    Math::EntryInfo::Tan    )
ASMJS_MATH_FUNC_NAMES(asin,     asin,   Math::EntryInfo::Asin   )
ASMJS_MATH_FUNC_NAMES(acos,     acos,   Math::EntryInfo::Acos   )
ASMJS_MATH_FUNC_NAMES(atan,     atan,   Math::EntryInfo::Atan   )
ASMJS_MATH_FUNC_NAMES(ceil,     ceil,   Math::EntryInfo::Ceil   )
ASMJS_MATH_FUNC_NAMES(floor,    floor,  Math::EntryInfo::Floor  )
ASMJS_MATH_FUNC_NAMES(exp,      exp,    Math::EntryInfo::Exp    )
ASMJS_MATH_FUNC_NAMES(log,      log,    Math::EntryInfo::Log    )
ASMJS_MATH_FUNC_NAMES(pow,      pow,    Math::EntryInfo::Pow    )
ASMJS_MATH_FUNC_NAMES(sqrt,     sqrt,   Math::EntryInfo::Sqrt   )
ASMJS_MATH_FUNC_NAMES(abs,      abs,    Math::EntryInfo::Abs    )
ASMJS_MATH_FUNC_NAMES(atan2,    atan2,  Math::EntryInfo::Atan2  )
ASMJS_MATH_FUNC_NAMES(imul,     imul,   Math::EntryInfo::Imul   )
ASMJS_MATH_FUNC_NAMES(fround,   fround, Math::EntryInfo::Fround )
ASMJS_MATH_FUNC_NAMES(min,      min,    Math::EntryInfo::Min    )
ASMJS_MATH_FUNC_NAMES(max,      max,    Math::EntryInfo::Max    )
ASMJS_MATH_FUNC_NAMES(clz32,    clz32,  Math::EntryInfo::Clz32  )

ASMJS_MATH_DOUBLE_CONST_NAMES(e,        E,        Math::E       )
ASMJS_MATH_DOUBLE_CONST_NAMES(ln10,     LN10,     Math::LN10    )
ASMJS_MATH_DOUBLE_CONST_NAMES(ln2,      LN2,      Math::LN2     )
ASMJS_MATH_DOUBLE_CONST_NAMES(log2e,    LOG2E,    Math::LOG2E   )
ASMJS_MATH_DOUBLE_CONST_NAMES(log10e,   LOG10E,   Math::LOG10E  )
ASMJS_MATH_DOUBLE_CONST_NAMES(pi,       PI,       Math::PI      )
ASMJS_MATH_DOUBLE_CONST_NAMES(sqrt1_2,  SQRT1_2,  Math::SQRT1_2 )
ASMJS_MATH_DOUBLE_CONST_NAMES(sqrt2,    SQRT2,    Math::SQRT2   )
ASMJS_MATH_CONST_NAMES(infinity, Infinity, 0             )
ASMJS_MATH_CONST_NAMES(nan,      NaN,      0             )

ASMJS_TYPED_ARRAY_NAMES(Uint8Array,   Uint8Array)
ASMJS_TYPED_ARRAY_NAMES(Int8Array,    Int8Array)
ASMJS_TYPED_ARRAY_NAMES(Uint16Array,  Uint16Array)
ASMJS_TYPED_ARRAY_NAMES(Int16Array,   Int16Array)
ASMJS_TYPED_ARRAY_NAMES(Uint32Array,  Uint32Array)
ASMJS_TYPED_ARRAY_NAMES(Int32Array,   Int32Array)
ASMJS_TYPED_ARRAY_NAMES(Float32Array, Float32Array)
ASMJS_TYPED_ARRAY_NAMES(Float64Array, Float64Array)
ASMJS_ARRAY_NAMES(byteLength,   byteLength)

// Int32x4
ASMJS_SIMD_MARKERS(Int32x4_Start) // just a marker
//               built-in ID                                propertyId                      Type............EntryPoint
ASMJS_SIMD_C_NAMES(Int32x4,                                   Int32x4,                      Int32x4,          Int32x4           )
ASMJS_SIMD_O_NAMES(int32x4_check,                             check,                        Int32x4,          Check             )
ASMJS_SIMD_O_NAMES(int32x4_splat,                             splat,                        Int32x4,          Splat             )
ASMJS_SIMD_O_NAMES(int32x4_fromFloat64x2,                     fromFloat64x2,                Int32x4,          FromFloat64x2     )
ASMJS_SIMD_O_NAMES(int32x4_fromFloat64x2Bits,                 fromFloat64x2Bits,            Int32x4,          FromFloat64x2Bits )
ASMJS_SIMD_O_NAMES(int32x4_fromFloat32x4,                     fromFloat32x4,                Int32x4,          FromFloat32x4     )
ASMJS_SIMD_O_NAMES(int32x4_fromFloat32x4Bits,                 fromFloat32x4Bits,            Int32x4,          FromFloat32x4Bits )
ASMJS_SIMD_O_NAMES(int32x4_fromInt16x8Bits,                   fromInt16x8Bits,              Int32x4,          FromInt16x8Bits   )
ASMJS_SIMD_O_NAMES(int32x4_fromInt8x16Bits,                   fromInt8x16Bits,              Int32x4,          FromInt8x16Bits   )
ASMJS_SIMD_O_NAMES(int32x4_fromUint32x4Bits,                  fromUint32x4Bits,             Int32x4,          FromUint32x4Bits  )
ASMJS_SIMD_O_NAMES(int32x4_fromUint16x8Bits,                  fromUint16x8Bits,             Int32x4,          FromUint16x8Bits  )
ASMJS_SIMD_O_NAMES(int32x4_fromUint8x16Bits,                  fromUint8x16Bits,             Int32x4,          FromUint8x16Bits  )
ASMJS_SIMD_O_NAMES(int32x4_neg,                               neg,                          Int32x4,          Neg               )
ASMJS_SIMD_O_NAMES(int32x4_add,                               add,                          Int32x4,          Add               )
ASMJS_SIMD_O_NAMES(int32x4_sub,                               sub,                          Int32x4,          Sub               )
ASMJS_SIMD_O_NAMES(int32x4_mul,                               mul,                          Int32x4,          Mul               )
ASMJS_SIMD_O_NAMES(int32x4_extractLane,                       extractLane,                  Int32x4,          ExtractLane       )
ASMJS_SIMD_O_NAMES(int32x4_replaceLane,                       replaceLane,                  Int32x4,          ReplaceLane       )
ASMJS_SIMD_O_NAMES(int32x4_swizzle,                           swizzle,                      Int32x4,          Swizzle           )
ASMJS_SIMD_O_NAMES(int32x4_shuffle,                           shuffle,                      Int32x4,          Shuffle           )
ASMJS_SIMD_O_NAMES(int32x4_lessThan,                          lessThan,                     Int32x4,          LessThan          )
ASMJS_SIMD_O_NAMES(int32x4_lessThanOrEqual,                   lessThanOrEqual,              Int32x4,          LessThanOrEqual   )
ASMJS_SIMD_O_NAMES(int32x4_equal,                             equal,                        Int32x4,          Equal             )
ASMJS_SIMD_O_NAMES(int32x4_notEqual,                          notEqual,                     Int32x4,          NotEqual          )
ASMJS_SIMD_O_NAMES(int32x4_greaterThan,                       greaterThan,                  Int32x4,          GreaterThan       )
ASMJS_SIMD_O_NAMES(int32x4_greaterThanOrEqual,                greaterThanOrEqual,           Int32x4,          GreaterThanOrEqual)
ASMJS_SIMD_O_NAMES(int32x4_select,                            select,                       Int32x4,          Select            )
ASMJS_SIMD_O_NAMES(int32x4_and,                               and_,                         Int32x4,          And               )
ASMJS_SIMD_O_NAMES(int32x4_or,                                or_,                          Int32x4,          Or                )
ASMJS_SIMD_O_NAMES(int32x4_xor,                               xor_,                         Int32x4,          Xor               )
ASMJS_SIMD_O_NAMES(int32x4_not,                               not_,                         Int32x4,          Not               )
ASMJS_SIMD_O_NAMES(int32x4_shiftLeftByScalar,                 shiftLeftByScalar,            Int32x4,          ShiftLeftByScalar )
ASMJS_SIMD_O_NAMES(int32x4_shiftRightByScalar,                shiftRightByScalar,           Int32x4,          ShiftRightByScalar)
// keep load/store contiguous
ASMJS_SIMD_O_NAMES(int32x4_load,                              load,                         Int32x4,          Load  )
ASMJS_SIMD_O_NAMES(int32x4_load1,                             load1,                        Int32x4,          Load1 )
ASMJS_SIMD_O_NAMES(int32x4_load2,                             load2,                        Int32x4,          Load2 )
ASMJS_SIMD_O_NAMES(int32x4_load3,                             load3,                        Int32x4,          Load3 )
ASMJS_SIMD_O_NAMES(int32x4_store,                             store,                        Int32x4,          Store )
ASMJS_SIMD_O_NAMES(int32x4_store1,                            store1,                       Int32x4,          Store1)
ASMJS_SIMD_O_NAMES(int32x4_store2,                            store2,                       Int32x4,          Store2)
ASMJS_SIMD_O_NAMES(int32x4_store3,                            store3,                       Int32x4,          Store3)
ASMJS_SIMD_MARKERS(Int32x4_End) // just a marker

ASMJS_SIMD_MARKERS(Bool32x4_Start) // just a marker
ASMJS_SIMD_C_NAMES(Bool32x4,                                  Bool32x4,                     Bool32x4,         Bool32x4)
ASMJS_SIMD_O_NAMES(bool32x4_check,                            check,                        Bool32x4,         Check   )
ASMJS_SIMD_O_NAMES(bool32x4_splat,                            splat,                        Bool32x4,         Splat   )
ASMJS_SIMD_O_NAMES(bool32x4_extractLane,                      extractLane,                  Bool32x4,         ExtractLane)
ASMJS_SIMD_O_NAMES(bool32x4_replaceLane,                      replaceLane,                  Bool32x4,         ReplaceLane)
ASMJS_SIMD_O_NAMES(bool32x4_and,                              and_,                         Bool32x4,         And     )
ASMJS_SIMD_O_NAMES(bool32x4_or,                               or_,                          Bool32x4,         Or      )
ASMJS_SIMD_O_NAMES(bool32x4_xor,                              xor_,                         Bool32x4,         Xor     )
ASMJS_SIMD_O_NAMES(bool32x4_not,                              not_,                         Bool32x4,         Not     )
ASMJS_SIMD_O_NAMES(bool32x4_anyTrue,                          anyTrue,                      Bool32x4,         AnyTrue )
ASMJS_SIMD_O_NAMES(bool32x4_allTrue,                          allTrue,                      Bool32x4,         AllTrue )
ASMJS_SIMD_MARKERS(Bool32x4_End)  // just a marker

ASMJS_SIMD_MARKERS(Bool16x8_Start) // just a marker
ASMJS_SIMD_C_NAMES(Bool16x8,                                  Bool16x8,                     Bool16x8,         Bool16x8)
ASMJS_SIMD_O_NAMES(bool16x8_check,                            check,                        Bool16x8,         Check   )
ASMJS_SIMD_O_NAMES(bool16x8_splat,                            splat,                        Bool16x8,         Splat   )
ASMJS_SIMD_O_NAMES(bool16x8_extractLane,                      extractLane,                  Bool16x8,         ExtractLane)
ASMJS_SIMD_O_NAMES(bool16x8_replaceLane,                      replaceLane,                  Bool16x8,         ReplaceLane)
ASMJS_SIMD_O_NAMES(bool16x8_and,                              and_,                         Bool16x8,         And     )
ASMJS_SIMD_O_NAMES(bool16x8_or,                               or_,                          Bool16x8,         Or      )
ASMJS_SIMD_O_NAMES(bool16x8_xor,                              xor_,                         Bool16x8,         Xor     )
ASMJS_SIMD_O_NAMES(bool16x8_not,                              not_,                         Bool16x8,         Not     )
ASMJS_SIMD_O_NAMES(bool16x8_anyTrue,                          anyTrue,                      Bool16x8,         AnyTrue )
ASMJS_SIMD_O_NAMES(bool16x8_allTrue,                          allTrue,                      Bool16x8,         AllTrue )
ASMJS_SIMD_MARKERS(Bool16x8_End)  // just a marker

ASMJS_SIMD_MARKERS(Bool8x16_Start) // just a marker
ASMJS_SIMD_C_NAMES(Bool8x16,                                  Bool8x16,                     Bool8x16,         Bool8x16)
ASMJS_SIMD_O_NAMES(bool8x16_check,                            check,                        Bool8x16,         Check   )
ASMJS_SIMD_O_NAMES(bool8x16_splat,                            splat,                        Bool8x16,         Splat   )
ASMJS_SIMD_O_NAMES(bool8x16_extractLane,                      extractLane,                  Bool8x16,         ExtractLane)
ASMJS_SIMD_O_NAMES(bool8x16_replaceLane,                      replaceLane,                  Bool8x16,         ReplaceLane)
ASMJS_SIMD_O_NAMES(bool8x16_and,                              and_,                         Bool8x16,         And     )
ASMJS_SIMD_O_NAMES(bool8x16_or,                               or_,                          Bool8x16,         Or      )
ASMJS_SIMD_O_NAMES(bool8x16_xor,                              xor_,                         Bool8x16,         Xor     )
ASMJS_SIMD_O_NAMES(bool8x16_not,                              not_,                         Bool8x16,         Not     )
ASMJS_SIMD_O_NAMES(bool8x16_anyTrue,                          anyTrue,                      Bool8x16,         AnyTrue )
ASMJS_SIMD_O_NAMES(bool8x16_allTrue,                          allTrue,                      Bool8x16,         AllTrue )
ASMJS_SIMD_MARKERS(Bool8x16_End)  // just a marker
// Float32x4
ASMJS_SIMD_MARKERS(Float32x4_Start) // just a marker
ASMJS_SIMD_C_NAMES(Float32x4,                                 Float32x4,                    Float32x4,        Float32x4                   )
ASMJS_SIMD_O_NAMES(float32x4_check,                           check,                        Float32x4,        Check                       )
ASMJS_SIMD_O_NAMES(float32x4_splat,                           splat,                        Float32x4,        Splat                       )
ASMJS_SIMD_O_NAMES(float32x4_fromFloat64x2,                   fromFloat64x2,                Float32x4,        FromFloat64x2               )
ASMJS_SIMD_O_NAMES(float32x4_fromFloat64x2Bits,               fromFloat64x2Bits,            Float32x4,        FromFloat64x2Bits           )
ASMJS_SIMD_O_NAMES(float32x4_fromInt32x4,                     fromInt32x4,                  Float32x4,        FromInt32x4                 )
ASMJS_SIMD_O_NAMES(float32x4_fromInt32x4Bits,                 fromInt32x4Bits,              Float32x4,        FromInt32x4Bits             )
ASMJS_SIMD_O_NAMES(float32x4_fromUint32x4,                    fromUint32x4,                 Float32x4,        FromUint32x4                )
ASMJS_SIMD_O_NAMES(float32x4_fromInt16x8Bits,                 fromInt16x8Bits,              Float32x4,        FromInt16x8Bits             )
ASMJS_SIMD_O_NAMES(float32x4_fromInt8x16Bits,                 fromInt8x16Bits,              Float32x4,        FromInt8x16Bits             )
ASMJS_SIMD_O_NAMES(float32x4_fromUint32x4Bits,                fromUint32x4Bits,             Float32x4,        FromUint32x4Bits            )
ASMJS_SIMD_O_NAMES(float32x4_fromUint16x8Bits,                fromUint16x8Bits,             Float32x4,        FromUint16x8Bits            )
ASMJS_SIMD_O_NAMES(float32x4_fromUint8x16Bits,                fromUint8x16Bits,             Float32x4,        FromUint8x16Bits            )
ASMJS_SIMD_O_NAMES(float32x4_abs,                             abs,                          Float32x4,        Abs                         )
ASMJS_SIMD_O_NAMES(float32x4_neg,                             neg,                          Float32x4,        Neg                         )
ASMJS_SIMD_O_NAMES(float32x4_add,                             add,                          Float32x4,        Add                         )
ASMJS_SIMD_O_NAMES(float32x4_sub,                             sub,                          Float32x4,        Sub                         )
ASMJS_SIMD_O_NAMES(float32x4_mul,                             mul,                          Float32x4,        Mul                         )
ASMJS_SIMD_O_NAMES(float32x4_div,                             div,                          Float32x4,        Div                         )
ASMJS_SIMD_O_NAMES(float32x4_min,                             min,                          Float32x4,        Min                         )
ASMJS_SIMD_O_NAMES(float32x4_max,                             max,                          Float32x4,        Max                         )
ASMJS_SIMD_O_NAMES(float32x4_reciprocal,                      reciprocalApproximation,      Float32x4,        Reciprocal                  )
ASMJS_SIMD_O_NAMES(float32x4_reciprocalSqrt,                  reciprocalSqrtApproximation,  Float32x4,        ReciprocalSqrt              )
ASMJS_SIMD_O_NAMES(float32x4_sqrt,                            sqrt,                         Float32x4,        Sqrt                        )
ASMJS_SIMD_O_NAMES(float32x4_swizzle,                         swizzle,                      Float32x4,        Swizzle                     )
ASMJS_SIMD_O_NAMES(float32x4_shuffle,                         shuffle,                      Float32x4,        Shuffle                     )
ASMJS_SIMD_O_NAMES(float32x4_extractLane,                     extractLane,                  Float32x4,        ExtractLane                 )
ASMJS_SIMD_O_NAMES(float32x4_replaceLane,                     replaceLane,                  Float32x4,        ReplaceLane                 )
ASMJS_SIMD_O_NAMES(float32x4_lessThan,                        lessThan,                     Float32x4,        LessThan                    )
ASMJS_SIMD_O_NAMES(float32x4_lessThanOrEqual,                 lessThanOrEqual,              Float32x4,        LessThanOrEqual             )
ASMJS_SIMD_O_NAMES(float32x4_equal,                           equal,                        Float32x4,        Equal                       )
ASMJS_SIMD_O_NAMES(float32x4_notEqual,                        notEqual,                     Float32x4,        NotEqual                    )
ASMJS_SIMD_O_NAMES(float32x4_greaterThan,                     greaterThan,                  Float32x4,        GreaterThan                 )
ASMJS_SIMD_O_NAMES(float32x4_greaterThanOrEqual,              greaterThanOrEqual,           Float32x4,        GreaterThanOrEqual          )
ASMJS_SIMD_O_NAMES(float32x4_select,                          select,                       Float32x4,        Select                      )
// keep load/store contiguous
ASMJS_SIMD_O_NAMES(float32x4_load,                            load,                         Float32x4,        Load                        )
ASMJS_SIMD_O_NAMES(float32x4_load1,                           load1,                        Float32x4,        Load1                       )
ASMJS_SIMD_O_NAMES(float32x4_load2,                           load2,                        Float32x4,        Load2                       )
ASMJS_SIMD_O_NAMES(float32x4_load3,                           load3,                        Float32x4,        Load3                       )
ASMJS_SIMD_O_NAMES(float32x4_store,                           store,                        Float32x4,        Store                       )
ASMJS_SIMD_O_NAMES(float32x4_store1,                          store1,                       Float32x4,        Store1                      )
ASMJS_SIMD_O_NAMES(float32x4_store2,                          store2,                       Float32x4,        Store2                      )
ASMJS_SIMD_O_NAMES(float32x4_store3,                          store3,                       Float32x4,        Store3                      )
ASMJS_SIMD_MARKERS(Float32x4_End) // just a marker

// Float64x2
// Disabled for now
ASMJS_SIMD_MARKERS(Float64x2_Start) // just a marker
ASMJS_SIMD_C_NAMES(Float64x2,                                 Float64x2,                    Float64x2,        Float64x2                   )
ASMJS_SIMD_O_NAMES(float64x2_check,                           check,                        Float64x2,        Check                       )
ASMJS_SIMD_O_NAMES(float64x2_splat,                           splat,                        Float64x2,        Splat                       )
ASMJS_SIMD_O_NAMES(float64x2_fromFloat32x4,                   fromFloat32x4,                Float64x2,        FromFloat32x4               )
ASMJS_SIMD_O_NAMES(float64x2_fromFloat32x4Bits,               fromFloat32x4Bits,            Float64x2,        FromFloat32x4Bits           )
ASMJS_SIMD_O_NAMES(float64x2_fromInt32x4,                     fromInt32x4,                  Float64x2,        FromInt32x4                 )
ASMJS_SIMD_O_NAMES(float64x2_fromInt32x4Bits,                 fromInt32x4Bits,              Float64x2,        FromInt32x4Bits             )
ASMJS_SIMD_O_NAMES(float64x2_abs,                             abs,                          Float64x2,        Abs                         )
ASMJS_SIMD_O_NAMES(float64x2_neg,                             neg,                          Float64x2,        Neg                         )
ASMJS_SIMD_O_NAMES(float64x2_add,                             add,                          Float64x2,        Add                         )
ASMJS_SIMD_O_NAMES(float64x2_sub,                             sub,                          Float64x2,        Sub                         )
ASMJS_SIMD_O_NAMES(float64x2_mul,                             mul,                          Float64x2,        Mul                         )
ASMJS_SIMD_O_NAMES(float64x2_div,                             div,                          Float64x2,        Div                         )
ASMJS_SIMD_O_NAMES(float64x2_min,                             min,                          Float64x2,        Min                         )
ASMJS_SIMD_O_NAMES(float64x2_max,                             max,                          Float64x2,        Max                         )
ASMJS_SIMD_O_NAMES(float64x2_reciprocal,                      reciprocalApproximation,      Float64x2,        Reciprocal                  )
ASMJS_SIMD_O_NAMES(float64x2_reciprocalSqrt,                  reciprocalSqrtApproximation,  Float64x2,        ReciprocalSqrt              )
ASMJS_SIMD_O_NAMES(float64x2_sqrt,                            sqrt,                         Float64x2,        Sqrt                        )
ASMJS_SIMD_O_NAMES(float64x2_swizzle,                         swizzle,                      Float64x2,        Swizzle                     )
ASMJS_SIMD_O_NAMES(float64x2_shuffle,                         shuffle,                      Float64x2,        Shuffle                     )
ASMJS_SIMD_O_NAMES(float64x2_lessThan,                        lessThan,                     Float64x2,        LessThan                    )
ASMJS_SIMD_O_NAMES(float64x2_lessThanOrEqual,                 lessThanOrEqual,              Float64x2,        LessThanOrEqual             )
ASMJS_SIMD_O_NAMES(float64x2_equal,                           equal,                        Float64x2,        Equal                       )
ASMJS_SIMD_O_NAMES(float64x2_notEqual,                        notEqual,                     Float64x2,        NotEqual                    )
ASMJS_SIMD_O_NAMES(float64x2_greaterThan,                     greaterThan,                  Float64x2,        GreaterThan                 )
ASMJS_SIMD_O_NAMES(float64x2_greaterThanOrEqual,              greaterThanOrEqual,           Float64x2,        GreaterThanOrEqual          )
ASMJS_SIMD_O_NAMES(float64x2_select,                          select,                       Float64x2,        Select                      )
// keep load/store contiguous
ASMJS_SIMD_O_NAMES(float64x2_load,                            load,                         Float64x2,        Load                        )
ASMJS_SIMD_O_NAMES(float64x2_load1,                           load1,                        Float64x2,        Load1                       )
ASMJS_SIMD_O_NAMES(float64x2_store,                           store,                        Float64x2,        Store                       )
ASMJS_SIMD_O_NAMES(float64x2_store1,                          store1,                       Float64x2,        Store1                      )
ASMJS_SIMD_MARKERS(Float64x2_End) // just a marker

ASMJS_SIMD_MARKERS(Int16x8_Start) // just a marker
ASMJS_SIMD_C_NAMES(Int16x8                   ,Int16x8            ,                        Int16x8             ,Int16x8           )
ASMJS_SIMD_O_NAMES(int16x8_check             ,check              ,                        Int16x8             ,Check             )
ASMJS_SIMD_O_NAMES(int16x8_extractLane       ,extractLane        ,                        Int16x8             ,ExtractLane       )
ASMJS_SIMD_O_NAMES(int16x8_swizzle           ,swizzle            ,                        Int16x8             ,Swizzle           )
ASMJS_SIMD_O_NAMES(int16x8_shuffle           ,shuffle            ,                        Int16x8             ,Shuffle           )
ASMJS_SIMD_O_NAMES(int16x8_splat             ,splat              ,                        Int16x8             ,Splat             )
ASMJS_SIMD_O_NAMES(int16x8_replaceLane       ,replaceLane        ,                        Int16x8             ,ReplaceLane       )
ASMJS_SIMD_O_NAMES(int16x8_and               ,and_               ,                        Int16x8             ,And               )
ASMJS_SIMD_O_NAMES(int16x8_or                ,or_                ,                        Int16x8             ,Or                )
ASMJS_SIMD_O_NAMES(int16x8_xor               ,xor_               ,                        Int16x8             ,Xor               )
ASMJS_SIMD_O_NAMES(int16x8_not               ,not_               ,                        Int16x8             ,Not               )
ASMJS_SIMD_O_NAMES(int16x8_add               ,add                ,                        Int16x8             ,Add               )
ASMJS_SIMD_O_NAMES(int16x8_sub               ,sub                ,                        Int16x8             ,Sub               )
ASMJS_SIMD_O_NAMES(int16x8_mul               ,mul                ,                        Int16x8             ,Mul               )
ASMJS_SIMD_O_NAMES(int16x8_neg               ,neg                ,                        Int16x8             ,Neg               )
ASMJS_SIMD_O_NAMES(int16x8_shiftLeftByScalar ,shiftLeftByScalar  ,                        Int16x8             ,ShiftLeftByScalar )
ASMJS_SIMD_O_NAMES(int16x8_shiftRightByScalar,shiftRightByScalar ,                        Int16x8             ,ShiftRightByScalar)
ASMJS_SIMD_O_NAMES(int16x8_lessThan          ,lessThan           ,                        Int16x8             ,LessThan          )
ASMJS_SIMD_O_NAMES(int16x8_lessThanOrEqual   ,lessThanOrEqual    ,                        Int16x8             ,LessThanOrEqual   )
ASMJS_SIMD_O_NAMES(int16x8_equal             ,equal              ,                        Int16x8             ,Equal             )
ASMJS_SIMD_O_NAMES(int16x8_notEqual          ,notEqual           ,                        Int16x8             ,NotEqual          )
ASMJS_SIMD_O_NAMES(int16x8_greaterThan       ,greaterThan        ,                        Int16x8             ,GreaterThan       )
ASMJS_SIMD_O_NAMES(int16x8_greaterThanOrEqual,greaterThanOrEqual ,                        Int16x8             ,GreaterThanOrEqual)
ASMJS_SIMD_O_NAMES(int16x8_select            ,select             ,                        Int16x8             ,Select            )
ASMJS_SIMD_O_NAMES(int16x8_addSaturate       ,addSaturate        ,                        Int16x8             ,AddSaturate       )
ASMJS_SIMD_O_NAMES(int16x8_subSaturate       ,subSaturate        ,                        Int16x8             ,SubSaturate       )
ASMJS_SIMD_O_NAMES(int16x8_load              ,load               ,                        Int16x8             ,Load              )
ASMJS_SIMD_O_NAMES(int16x8_store             ,store              ,                        Int16x8             ,Store             )
ASMJS_SIMD_O_NAMES(int16x8_fromFloat32x4Bits ,fromFloat32x4Bits  ,                        Int16x8             ,FromFloat32x4Bits )
ASMJS_SIMD_O_NAMES(int16x8_fromInt32x4Bits   ,fromInt32x4Bits    ,                        Int16x8             ,FromInt32x4Bits   )
ASMJS_SIMD_O_NAMES(int16x8_fromInt8x16Bits   ,fromInt8x16Bits    ,                        Int16x8             ,FromInt8x16Bits   )
ASMJS_SIMD_O_NAMES(int16x8_fromUint32x4Bits  ,fromUint32x4Bits   ,                        Int16x8             ,FromUint32x4Bits  )
ASMJS_SIMD_O_NAMES(int16x8_fromUint16x8Bits  ,fromUint16x8Bits   ,                        Int16x8             ,FromUint16x8Bits  )
ASMJS_SIMD_O_NAMES(int16x8_fromUint8x16Bits  ,fromUint8x16Bits   ,                        Int16x8             ,FromUint8x16Bits  )
// Int16x8 built-in IDs go here
ASMJS_SIMD_MARKERS(Int16x8_End) // just a marker

ASMJS_SIMD_MARKERS(Int8x16_Start) // just a marker
ASMJS_SIMD_C_NAMES(Int8x16                   ,Int8x16            ,                        Int8x16             ,Int8x16           )
ASMJS_SIMD_O_NAMES(int8x16_check             ,check              ,                        Int8x16             ,Check             )
ASMJS_SIMD_O_NAMES(int8x16_extractLane       ,extractLane        ,                        Int8x16             ,ExtractLane       )
ASMJS_SIMD_O_NAMES(int8x16_swizzle           ,swizzle            ,                        Int8x16             ,Swizzle           )
ASMJS_SIMD_O_NAMES(int8x16_shuffle           ,shuffle            ,                        Int8x16             ,Shuffle           )
ASMJS_SIMD_O_NAMES(int8x16_splat             ,splat              ,                        Int8x16             ,Splat             )
ASMJS_SIMD_O_NAMES(int8x16_replaceLane       ,replaceLane        ,                        Int8x16             ,ReplaceLane       )
ASMJS_SIMD_O_NAMES(int8x16_and               ,and_               ,                        Int8x16             ,And               )
ASMJS_SIMD_O_NAMES(int8x16_or                ,or_                ,                        Int8x16             ,Or                )
ASMJS_SIMD_O_NAMES(int8x16_xor               ,xor_               ,                        Int8x16             ,Xor               )
ASMJS_SIMD_O_NAMES(int8x16_not               ,not_               ,                        Int8x16             ,Not               )
ASMJS_SIMD_O_NAMES(int8x16_add               ,add                ,                        Int8x16             ,Add               )
ASMJS_SIMD_O_NAMES(int8x16_sub               ,sub                ,                        Int8x16             ,Sub               )
ASMJS_SIMD_O_NAMES(int8x16_mul               ,mul                ,                        Int8x16             ,Mul               )
ASMJS_SIMD_O_NAMES(int8x16_neg               ,neg                ,                        Int8x16             ,Neg               )
ASMJS_SIMD_O_NAMES(int8x16_shiftLeftByScalar ,shiftLeftByScalar  ,                        Int8x16             ,ShiftLeftByScalar )
ASMJS_SIMD_O_NAMES(int8x16_shiftRightByScalar,shiftRightByScalar ,                        Int8x16             ,ShiftRightByScalar)
ASMJS_SIMD_O_NAMES(int8x16_lessThan          ,lessThan           ,                        Int8x16             ,LessThan          )
ASMJS_SIMD_O_NAMES(int8x16_lessThanOrEqual   ,lessThanOrEqual    ,                        Int8x16             ,LessThanOrEqual   )
ASMJS_SIMD_O_NAMES(int8x16_equal             ,equal              ,                        Int8x16             ,Equal             )
ASMJS_SIMD_O_NAMES(int8x16_notEqual          ,notEqual           ,                        Int8x16             ,NotEqual          )
ASMJS_SIMD_O_NAMES(int8x16_greaterThan       ,greaterThan        ,                        Int8x16             ,GreaterThan       )
ASMJS_SIMD_O_NAMES(int8x16_greaterThanOrEqual,greaterThanOrEqual ,                        Int8x16             ,GreaterThanOrEqual)
ASMJS_SIMD_O_NAMES(int8x16_select            ,select             ,                        Int8x16             ,Select            )
ASMJS_SIMD_O_NAMES(int8x16_addSaturate       ,addSaturate        ,                        Int8x16             ,AddSaturate       )
ASMJS_SIMD_O_NAMES(int8x16_subSaturate       ,subSaturate        ,                        Int8x16             ,SubSaturate       )
ASMJS_SIMD_O_NAMES(int8x16_load              ,load               ,                        Int8x16             ,Load              )
ASMJS_SIMD_O_NAMES(int8x16_store             ,store              ,                        Int8x16             ,Store             )
ASMJS_SIMD_O_NAMES(int8x16_fromFloat32x4Bits ,fromFloat32x4Bits  ,                        Int8x16             ,FromFloat32x4Bits )
ASMJS_SIMD_O_NAMES(int8x16_fromInt32x4Bits   ,fromInt32x4Bits    ,                        Int8x16             ,FromInt32x4Bits   )
ASMJS_SIMD_O_NAMES(int8x16_fromInt16x8Bits   ,fromInt16x8Bits    ,                        Int8x16             ,FromInt16x8Bits   )
ASMJS_SIMD_O_NAMES(int8x16_fromUint32x4Bits  ,fromUint32x4Bits   ,                        Int8x16             ,FromUint32x4Bits  )
ASMJS_SIMD_O_NAMES(int8x16_fromUint16x8Bits  ,fromUint16x8Bits   ,                        Int8x16             ,FromUint16x8Bits  )
ASMJS_SIMD_O_NAMES(int8x16_fromUint8x16Bits  ,fromUint8x16Bits   ,                        Int8x16             ,FromUint8x16Bits  )
ASMJS_SIMD_MARKERS(Int8x16_End) // just a marker

ASMJS_SIMD_MARKERS(Uint32x4_Start) // just a marker
ASMJS_SIMD_C_NAMES(Uint32x4                   , Uint32x4             ,                    Uint32x4            ,Uint32x4          )
ASMJS_SIMD_O_NAMES(uint32x4_check             , check                ,                    Uint32x4            ,Check             )
ASMJS_SIMD_O_NAMES(uint32x4_extractLane       , extractLane          ,                    Uint32x4            ,ExtractLane       )
ASMJS_SIMD_O_NAMES(uint32x4_swizzle           , swizzle              ,                    Uint32x4            ,Swizzle           )
ASMJS_SIMD_O_NAMES(uint32x4_shuffle           , shuffle              ,                    Uint32x4            ,Shuffle           )
ASMJS_SIMD_O_NAMES(uint32x4_splat             , splat                ,                    Uint32x4            ,Splat             )
ASMJS_SIMD_O_NAMES(uint32x4_replaceLane       , replaceLane          ,                    Uint32x4            ,ReplaceLane       )
ASMJS_SIMD_O_NAMES(uint32x4_and               , and_                 ,                    Uint32x4            ,And               )
ASMJS_SIMD_O_NAMES(uint32x4_or                , or_                  ,                    Uint32x4            ,Or                )
ASMJS_SIMD_O_NAMES(uint32x4_xor               , xor_                 ,                    Uint32x4            ,Xor               )
ASMJS_SIMD_O_NAMES(uint32x4_not               , not_                 ,                    Uint32x4            ,Not               )
ASMJS_SIMD_O_NAMES(uint32x4_neg               , neg                  ,                    Uint32x4            ,Neg               )
ASMJS_SIMD_O_NAMES(uint32x4_add               , add                  ,                    Uint32x4            ,Add               )
ASMJS_SIMD_O_NAMES(uint32x4_sub               , sub                  ,                    Uint32x4            ,Sub               )
ASMJS_SIMD_O_NAMES(uint32x4_mul               , mul                  ,                    Uint32x4            ,Mul               )
ASMJS_SIMD_O_NAMES(uint32x4_shiftLeftByScalar , shiftLeftByScalar    ,                    Uint32x4            ,ShiftLeftByScalar )
ASMJS_SIMD_O_NAMES(uint32x4_shiftRightByScalar, shiftRightByScalar   ,                    Uint32x4            ,ShiftRightByScalar)
ASMJS_SIMD_O_NAMES(uint32x4_lessThan          , lessThan             ,                    Uint32x4            ,LessThan          )
ASMJS_SIMD_O_NAMES(uint32x4_lessThanOrEqual   , lessThanOrEqual      ,                    Uint32x4            ,LessThanOrEqual   )
ASMJS_SIMD_O_NAMES(uint32x4_equal             , equal                ,                    Uint32x4            ,Equal             )
ASMJS_SIMD_O_NAMES(uint32x4_notEqual          , notEqual             ,                    Uint32x4            ,NotEqual          )
ASMJS_SIMD_O_NAMES(uint32x4_greaterThan       , greaterThan          ,                    Uint32x4            ,GreaterThan       )
ASMJS_SIMD_O_NAMES(uint32x4_greaterThanOrEqual, greaterThanOrEqual   ,                    Uint32x4            ,GreaterThanOrEqual)
ASMJS_SIMD_O_NAMES(uint32x4_select            , select               ,                    Uint32x4            ,Select            )
ASMJS_SIMD_O_NAMES(uint32x4_load              , load                 ,                    Uint32x4            ,Load              )
ASMJS_SIMD_O_NAMES(uint32x4_load1             , load1                ,                    Uint32x4            ,Load1             )
ASMJS_SIMD_O_NAMES(uint32x4_load2             , load2                ,                    Uint32x4            ,Load2             )
ASMJS_SIMD_O_NAMES(uint32x4_load3             , load3                ,                    Uint32x4            ,Load3             )
ASMJS_SIMD_O_NAMES(uint32x4_store             , store                ,                    Uint32x4            ,Store             )
ASMJS_SIMD_O_NAMES(uint32x4_store1            , store1               ,                    Uint32x4            ,Store1            )
ASMJS_SIMD_O_NAMES(uint32x4_store2            , store2               ,                    Uint32x4            ,Store2            )
ASMJS_SIMD_O_NAMES(uint32x4_store3            , store3               ,                    Uint32x4            ,Store3            )
ASMJS_SIMD_O_NAMES(uint32x4_fromFloat32x4     , fromFloat32x4        ,                    Uint32x4            ,FromFloat32x4     )
ASMJS_SIMD_O_NAMES(uint32x4_fromFloat32x4Bits , fromFloat32x4Bits    ,                    Uint32x4            ,FromFloat32x4Bits )
ASMJS_SIMD_O_NAMES(uint32x4_fromInt32x4Bits   , fromInt32x4Bits      ,                    Uint32x4            ,FromInt32x4Bits   )
ASMJS_SIMD_O_NAMES(uint32x4_fromInt16x8Bits   , fromInt16x8Bits      ,                    Uint32x4            ,FromInt16x8Bits   )
ASMJS_SIMD_O_NAMES(uint32x4_fromInt8x16Bits   , fromInt8x16Bits      ,                    Uint32x4            ,FromInt8x16Bits   )
ASMJS_SIMD_O_NAMES(uint32x4_fromUint16x8Bits  , fromUint16x8Bits     ,                    Uint32x4            ,FromUint16x8Bits  )
ASMJS_SIMD_O_NAMES(uint32x4_fromUint8x16Bits  , fromUint8x16Bits     ,                    Uint32x4            ,FromUint8x16Bits  )
// Uint32x4 built-in IDs go here
ASMJS_SIMD_MARKERS(Uint32x4_End) // just a marker

ASMJS_SIMD_MARKERS(Uint16x8_Start) // just a marker
ASMJS_SIMD_C_NAMES(Uint16x8                       , Uint16x8             ,                  Uint16x8           ,Uint16x8          )
ASMJS_SIMD_O_NAMES(uint16x8_check                 , check                ,                  Uint16x8           ,Check             )
ASMJS_SIMD_O_NAMES(uint16x8_extractLane           , extractLane          ,                  Uint16x8           ,ExtractLane       )
ASMJS_SIMD_O_NAMES(uint16x8_swizzle               , swizzle              ,                  Uint16x8           ,Swizzle           )
ASMJS_SIMD_O_NAMES(uint16x8_shuffle               , shuffle              ,                  Uint16x8           ,Shuffle           )
ASMJS_SIMD_O_NAMES(uint16x8_splat                 , splat                ,                  Uint16x8           ,Splat             )
ASMJS_SIMD_O_NAMES(uint16x8_replaceLane           , replaceLane          ,                  Uint16x8           ,ReplaceLane       )
ASMJS_SIMD_O_NAMES(uint16x8_and                   , and_                 ,                  Uint16x8           ,And               )
ASMJS_SIMD_O_NAMES(uint16x8_or                    , or_                  ,                  Uint16x8           ,Or                )
ASMJS_SIMD_O_NAMES(uint16x8_xor                   , xor_                 ,                  Uint16x8           ,Xor               )
ASMJS_SIMD_O_NAMES(uint16x8_not                   , not_                 ,                  Uint16x8           ,Not               )
ASMJS_SIMD_O_NAMES(uint16x8_neg                   , neg                  ,                  Uint16x8           ,Neg               )
ASMJS_SIMD_O_NAMES(uint16x8_add                   , add                  ,                  Uint16x8           ,Add               )
ASMJS_SIMD_O_NAMES(uint16x8_sub                   , sub                  ,                  Uint16x8           ,Sub               )
ASMJS_SIMD_O_NAMES(uint16x8_mul                   , mul                  ,                  Uint16x8           ,Mul               )
ASMJS_SIMD_O_NAMES(uint16x8_shiftLeftByScalar     , shiftLeftByScalar    ,                  Uint16x8           ,ShiftLeftByScalar )
ASMJS_SIMD_O_NAMES(uint16x8_shiftRightByScalar    , shiftRightByScalar   ,                  Uint16x8           ,ShiftRightByScalar)
ASMJS_SIMD_O_NAMES(uint16x8_lessThan              , lessThan             ,                  Uint16x8           ,LessThan          )
ASMJS_SIMD_O_NAMES(uint16x8_lessThanOrEqual       , lessThanOrEqual      ,                  Uint16x8           ,LessThanOrEqual   )
ASMJS_SIMD_O_NAMES(uint16x8_equal                 , equal                ,                  Uint16x8           ,Equal             )
ASMJS_SIMD_O_NAMES(uint16x8_notEqual              , notEqual             ,                  Uint16x8           ,NotEqual          )
ASMJS_SIMD_O_NAMES(uint16x8_greaterThan           , greaterThan          ,                  Uint16x8           ,GreaterThan       )
ASMJS_SIMD_O_NAMES(uint16x8_greaterThanOrEqual    , greaterThanOrEqual   ,                  Uint16x8           ,GreaterThanOrEqual)
ASMJS_SIMD_O_NAMES(uint16x8_select                , select               ,                  Uint16x8           ,Select            )
ASMJS_SIMD_O_NAMES(uint16x8_addSaturate           , addSaturate          ,                  Uint16x8           ,AddSaturate       )
ASMJS_SIMD_O_NAMES(uint16x8_subSaturate           , subSaturate          ,                  Uint16x8           ,SubSaturate       )
ASMJS_SIMD_O_NAMES(uint16x8_load                  , load                 ,                  Uint16x8           ,Load              )
ASMJS_SIMD_O_NAMES(uint16x8_store                 , store                ,                  Uint16x8           ,Store             )
ASMJS_SIMD_O_NAMES(uint16x8_fromFloat32x4Bits     , fromFloat32x4Bits    ,                  Uint16x8           ,FromFloat32x4Bits )
ASMJS_SIMD_O_NAMES(uint16x8_fromInt32x4Bits       , fromInt32x4Bits      ,                  Uint16x8           ,FromInt32x4Bits   )
ASMJS_SIMD_O_NAMES(uint16x8_fromInt16x8Bits       , fromInt16x8Bits      ,                  Uint16x8           ,FromInt16x8Bits   )
ASMJS_SIMD_O_NAMES(uint16x8_fromInt8x16Bits       , fromInt8x16Bits      ,                  Uint16x8           ,FromInt8x16Bits   )
ASMJS_SIMD_O_NAMES(uint16x8_fromUint32x4Bits      , fromUint32x4Bits     ,                  Uint16x8           ,FromUint32x4Bits  )
ASMJS_SIMD_O_NAMES(uint16x8_fromUint8x16Bits      , fromUint8x16Bits     ,                  Uint16x8           ,FromUint8x16Bits  )
// Uint16x8 built-in IDs go here
ASMJS_SIMD_MARKERS(Uint16x8_End) // just a marker

ASMJS_SIMD_MARKERS(Uint8x16_Start) // just a marker
ASMJS_SIMD_C_NAMES(Uint8x16                      , Uint8x16              ,                  Uint8x16           , Uint8x16          )
ASMJS_SIMD_O_NAMES(uint8x16_check                , check                 ,                  Uint8x16           , Check             )
ASMJS_SIMD_O_NAMES(uint8x16_extractLane          , extractLane           ,                  Uint8x16           , ExtractLane       )
ASMJS_SIMD_O_NAMES(uint8x16_swizzle              , swizzle               ,                  Uint8x16           , Swizzle           )
ASMJS_SIMD_O_NAMES(uint8x16_shuffle              , shuffle               ,                  Uint8x16           , Shuffle           )
ASMJS_SIMD_O_NAMES(uint8x16_splat                , splat                 ,                  Uint8x16           , Splat             )
ASMJS_SIMD_O_NAMES(uint8x16_replaceLane          , replaceLane           ,                  Uint8x16           , ReplaceLane       )
ASMJS_SIMD_O_NAMES(uint8x16_and                  , and_                   ,                  Uint8x16           , And               )
ASMJS_SIMD_O_NAMES(uint8x16_or                   , or_                    ,                  Uint8x16           , Or                )
ASMJS_SIMD_O_NAMES(uint8x16_xor                  , xor_                   ,                  Uint8x16           , Xor               )
ASMJS_SIMD_O_NAMES(uint8x16_not                  , not_                   ,                  Uint8x16           , Not               )
ASMJS_SIMD_O_NAMES(uint8x16_neg                  , neg                   ,                  Uint8x16           , Neg               )
ASMJS_SIMD_O_NAMES(uint8x16_add                  , add                   ,                  Uint8x16           , Add               )
ASMJS_SIMD_O_NAMES(uint8x16_sub                  , sub                   ,                  Uint8x16           , Sub               )
ASMJS_SIMD_O_NAMES(uint8x16_mul                  , mul                   ,                  Uint8x16           , Mul               )
ASMJS_SIMD_O_NAMES(uint8x16_shiftLeftByScalar    , shiftLeftByScalar     ,                  Uint8x16           , ShiftLeftByScalar )
ASMJS_SIMD_O_NAMES(uint8x16_shiftRightByScalar   , shiftRightByScalar    ,                  Uint8x16           , ShiftRightByScalar)
ASMJS_SIMD_O_NAMES(uint8x16_lessThan             , lessThan              ,                  Uint8x16           , LessThan          )
ASMJS_SIMD_O_NAMES(uint8x16_lessThanOrEqual      , lessThanOrEqual       ,                  Uint8x16           , LessThanOrEqual   )
ASMJS_SIMD_O_NAMES(uint8x16_equal                , equal                 ,                  Uint8x16           , Equal             )
ASMJS_SIMD_O_NAMES(uint8x16_notEqual             , notEqual              ,                  Uint8x16           , NotEqual          )
ASMJS_SIMD_O_NAMES(uint8x16_greaterThan          , greaterThan           ,                  Uint8x16           , GreaterThan       )
ASMJS_SIMD_O_NAMES(uint8x16_greaterThanOrEqual   , greaterThanOrEqual    ,                  Uint8x16           , GreaterThanOrEqual)
ASMJS_SIMD_O_NAMES(uint8x16_select               , select                ,                  Uint8x16           , Select            )
ASMJS_SIMD_O_NAMES(uint8x16_addSaturate          , addSaturate           ,                  Uint8x16           , AddSaturate       )
ASMJS_SIMD_O_NAMES(uint8x16_subSaturate          , subSaturate           ,                  Uint8x16           , SubSaturate       )
ASMJS_SIMD_O_NAMES(uint8x16_load                 , load                  ,                  Uint8x16           , Load              )
ASMJS_SIMD_O_NAMES(uint8x16_store                , store                 ,                  Uint8x16           , Store             )
ASMJS_SIMD_O_NAMES(uint8x16_fromFloat32x4Bits    , fromFloat32x4Bits     ,                  Uint8x16           , FromFloat32x4Bits )
ASMJS_SIMD_O_NAMES(uint8x16_fromInt32x4Bits      , fromInt32x4Bits       ,                  Uint8x16           , FromInt32x4Bits   )
ASMJS_SIMD_O_NAMES(uint8x16_fromInt16x8Bits      , fromInt16x8Bits       ,                  Uint8x16           , FromInt16x8Bits   )
ASMJS_SIMD_O_NAMES(uint8x16_fromInt8x16Bits      , fromInt8x16Bits       ,                  Uint8x16           , FromInt8x16Bits   )
ASMJS_SIMD_O_NAMES(uint8x16_fromUint32x4Bits     , fromUint32x4Bits      ,                  Uint8x16           , FromUint32x4Bits  )
ASMJS_SIMD_O_NAMES(uint8x16_fromUint16x8Bits     , fromUint16x8Bits      ,                  Uint8x16           , FromUint16x8Bits  )
// Uint16x8 built-in IDs go here
ASMJS_SIMD_MARKERS(Uint8x16_End) // just a marker

// help the caller to undefine all the macros
#undef ASMJS_MATH_FUNC_NAMES
#undef ASMJS_MATH_CONST_NAMES
#undef ASMJS_MATH_DOUBLE_CONST_NAMES
#undef ASMJS_ARRAY_NAMES
#undef ASMJS_TYPED_ARRAY_NAMES
#undef ASMJS_SIMD_NAMES
#undef ASMJS_SIMD_C_NAMES
#undef ASMJS_SIMD_O_NAMES
#undef ASMJS_SIMD_MARKERS
