//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
SIMD.js opcodes
- All opcodes are typed.
- Used as bytecode for AsmJs Interpreter.
- Used as IR by the backend only for both AsmJs and non-AsmJs code.
*/

// used as both AsmJs bytecode and IR
#ifndef MACRO_SIMD
#define MACRO_SIMD(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...)
#endif

#ifndef MACRO_SIMD_WMS
#define MACRO_SIMD_WMS(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...)
#endif

// used as AsmJs bytecode only
#ifndef MACRO_SIMD_ASMJS_ONLY_WMS
#define MACRO_SIMD_ASMJS_ONLY_WMS(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...)
#endif

// used as IR only
#ifndef MACRO_SIMD_BACKEND_ONLY
#define MACRO_SIMD_BACKEND_ONLY(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr)
#endif

// same as above but with extended opcodes
#ifndef MACRO_SIMD_EXTEND
#define MACRO_SIMD_EXTEND(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...)
#endif

#ifndef MACRO_SIMD_EXTEND_WMS
#define MACRO_SIMD_EXTEND_WMS(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...)
#endif

// used as AsmJs bytecode only
#ifndef MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS
#define MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr, ...)
#endif

// used as IR only
#ifndef MACRO_SIMD_BACKEND_ONLY_EXTEND
#define MACRO_SIMD_BACKEND_ONLY_EXTEND(opcode, asmjsLayout, opCodeAttrAsmJs, OpCodeAttr)
#endif


// helper macros
#define T_F4    ValueType::GetSimd128(ObjectType::Simd128Float32x4)
#define T_I4    ValueType::GetSimd128(ObjectType::Simd128Int32x4)
#define T_INT   ValueType::GetInt(false)
#define T_FLT   ValueType::Float

// TODO: Enable OpCanCSE for all opcodes. Dependent on extending ExprHash to 64-bits.

//                              OpCode                             , LayoutAsmJs                , OpCodeAttrAsmJs,          OpCodeAttr              Addition macro args     FuncInfo        Ret and Args ValueTypes
//                                |                                    |                                |                       |                       |                      |                  |
//                                v                                    v                                v                       v                       v                      v                  v
MACRO_SIMD                  ( Simd128_Start                     , Empty                              , None           ,        None                          ,        0)               // Just a marker to indicate SIMD opcodes region

// Int32x4
MACRO_SIMD_WMS              ( Simd128_IntsToI4                  , Int32x4_1Int4                     , None           ,        OpCanCSE         ,       6,  &Js::SIMDInt32x4Lib::EntryInfo::Int32x4, T_I4, T_INT, T_INT, T_INT, T_INT)
MACRO_SIMD_WMS              ( Simd128_Splat_I4                  , Int32x4_1Int1                     , None           ,        OpCanCSE         ,       3,  &Js::SIMDInt32x4Lib::EntryInfo::Splat  , T_I4, T_INT)
//MACRO_SIMD_WMS            ( Simd128_FromFloat64x2_I4          , Int32x4_1Float64x2_1              , None           ,        OpCanCSE         ,       0)
//MACRO_SIMD_WMS            ( Simd128_FromFloat64x2Bits_I4      , Int32x4_1Float64x2_1              , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_WMS              ( Simd128_FromFloat32x4_I4          , Int32x4_1Float32x4_1              , None           ,        OpCanCSE         ,       3,  &Js::SIMDInt32x4Lib::EntryInfo::FromFloat32x4     , T_I4, T_F4)
MACRO_SIMD_WMS              ( Simd128_FromFloat32x4Bits_I4      , Int32x4_1Float32x4_1              , None           ,        OpCanCSE         ,       3,  &Js::SIMDInt32x4Lib::EntryInfo::FromFloat32x4Bits , T_I4, T_F4)
MACRO_SIMD_WMS              ( Simd128_Neg_I4                    , Int32x4_2                         , None           ,        OpCanCSE         ,       3,  &Js::SIMDInt32x4Lib::EntryInfo::Neg,    T_I4, T_I4)
MACRO_SIMD_WMS              ( Simd128_Add_I4                    , Int32x4_3                         , None           ,        OpCanCSE         ,       4,  &Js::SIMDInt32x4Lib::EntryInfo::Add,    T_I4, T_I4, T_I4)
MACRO_SIMD_WMS              ( Simd128_Sub_I4                    , Int32x4_3                         , None           ,        OpCanCSE         ,       4,  &Js::SIMDInt32x4Lib::EntryInfo::Sub,    T_I4, T_I4, T_I4)
MACRO_SIMD_WMS              ( Simd128_Mul_I4                    , Int32x4_3                         , None           ,        OpCanCSE         ,       4,  &Js::SIMDInt32x4Lib::EntryInfo::Mul,    T_I4, T_I4, T_I4)
MACRO_SIMD_WMS              ( Simd128_Lt_I4                     , Bool32x4_1Int32x4_2               , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_WMS              ( Simd128_Gt_I4                     , Bool32x4_1Int32x4_2               , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_WMS              ( Simd128_Eq_I4                     , Bool32x4_1Int32x4_2               , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_WMS              ( Simd128_Select_I4                 , Int32x4_1Bool32x4_1Int32x4_2      , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_WMS              ( Simd128_And_I4                    , Int32x4_3                         , None           ,        OpCanCSE         ,       4,  &Js::SIMDInt32x4Lib::EntryInfo::And,    T_I4, T_I4, T_I4)
MACRO_SIMD_WMS              ( Simd128_Or_I4                     , Int32x4_3                         , None           ,        OpCanCSE         ,       4,  &Js::SIMDInt32x4Lib::EntryInfo::Or,     T_I4, T_I4, T_I4)
MACRO_SIMD_WMS              ( Simd128_Xor_I4                    , Int32x4_3                         , None           ,        OpCanCSE         ,       4,  &Js::SIMDInt32x4Lib::EntryInfo::Xor,    T_I4, T_I4, T_I4)
MACRO_SIMD_WMS              ( Simd128_Not_I4                    , Int32x4_2                         , None           ,        OpCanCSE         ,       3,  &Js::SIMDInt32x4Lib::EntryInfo::Not,    T_I4, T_I4)
MACRO_SIMD_WMS              ( Simd128_ShLtByScalar_I4           , Int32x4_2Int1                     , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_WMS              ( Simd128_ShRtByScalar_I4           , Int32x4_2Int1                     , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_WMS              ( Simd128_Swizzle_I4                , Int32x4_2Int4                     , None           ,        OpCanCSE         ,       7,   &Js::SIMDInt32x4Lib::EntryInfo::Swizzle,   T_I4, T_I4, T_INT, T_INT, T_INT, T_INT)
MACRO_SIMD_WMS              ( Simd128_Shuffle_I4                , Int32x4_3Int4                     , None           ,        OpCanCSE         ,       8,   &Js::SIMDInt32x4Lib::EntryInfo::Shuffle,   T_I4, T_I4, T_I4, T_INT, T_INT, T_INT, T_INT)
MACRO_SIMD_ASMJS_ONLY_WMS   ( Simd128_Ld_I4                     , Int32x4_2                         , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_WMS   ( Simd128_LdSlot_I4                 , ElementSlot                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_WMS   ( Simd128_StSlot_I4                 , ElementSlot                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_WMS   ( Simd128_Return_I4                 , Int32x4_2                         , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_WMS   ( Simd128_I_ArgOut_I4               , Reg1Int32x4_1                     , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_WMS   ( Simd128_I_Conv_VTI4               , Int32x4_2                         , None           ,        None                      )

// Extended opcodes. Running out of 1-byte opcode space. Add new opcodes here.
MACRO_SIMD(Simd128_End, Empty, None, None) // Just a marker to indicate SIMD opcodes region
MACRO_SIMD_EXTEND(Simd128_Start_Extend, Empty, None, None, 0) // Just a marker to indicate SIMD extended opcodes region

// Float32x4
MACRO_SIMD_EXTEND_WMS           ( Simd128_FloatsToF4                , Float32x4_1Float4                 , None           ,        OpCanCSE          ,      6,   &Js::SIMDFloat32x4Lib::EntryInfo::Float32x4, T_F4, T_FLT, T_FLT, T_FLT, T_FLT)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Splat_F4                  , Float32x4_1Float1                 , None           ,        OpCanCSE          ,      3,   &Js::SIMDFloat32x4Lib::EntryInfo::Splat    , T_F4, T_FLT)
//MACRO_SIMD_EXTEND_WMS         ( Simd128_FromFloat64x2_F4          , Float32x4_1Float64x2_1            , None           ,        OpCanCSE          ,      0)
//MACRO_SIMD_EXTEND_WMS         ( Simd128_FromFloat64x2Bits_F4      , Float32x4_1Float64x2_1            , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS           ( Simd128_FromInt32x4_F4            , Float32x4_1Int32x4_1              , None           ,        OpCanCSE          ,      3,   &Js::SIMDFloat32x4Lib::EntryInfo::FromInt32x4    , T_F4, T_I4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_FromInt32x4Bits_F4        , Float32x4_1Int32x4_1              , None           ,        OpCanCSE          ,      3,   &Js::SIMDFloat32x4Lib::EntryInfo::FromInt32x4Bits, T_F4, T_I4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Abs_F4                    , Float32x4_2                       , None           ,        OpCanCSE          ,      3,   &Js::SIMDFloat32x4Lib::EntryInfo::Abs,   T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Neg_F4                    , Float32x4_2                       , None           ,        OpCanCSE          ,      3,   &Js::SIMDFloat32x4Lib::EntryInfo::Neg,   T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Add_F4                    , Float32x4_3                       , None           ,        OpCanCSE          ,      4,   &Js::SIMDFloat32x4Lib::EntryInfo::Add,   T_F4, T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Sub_F4                    , Float32x4_3                       , None           ,        OpCanCSE          ,      4,   &Js::SIMDFloat32x4Lib::EntryInfo::Sub,   T_F4, T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Mul_F4                    , Float32x4_3                       , None           ,        OpCanCSE          ,      4,   &Js::SIMDFloat32x4Lib::EntryInfo::Mul,   T_F4, T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Div_F4                    , Float32x4_3                       , None           ,        OpCanCSE          ,      4,   &Js::SIMDFloat32x4Lib::EntryInfo::Div,   T_F4, T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Clamp_F4                  , Float32x4_4                       , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Min_F4                    , Float32x4_3                       , None           ,        OpCanCSE          ,      4,   &Js::SIMDFloat32x4Lib::EntryInfo::Min,   T_F4, T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Max_F4                    , Float32x4_3                       , None           ,        OpCanCSE          ,      4,   &Js::SIMDFloat32x4Lib::EntryInfo::Max,   T_F4, T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Rcp_F4                    , Float32x4_2                       , None           ,        OpCanCSE          ,      3,   &Js::SIMDFloat32x4Lib::EntryInfo::Reciprocal,     T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_RcpSqrt_F4                , Float32x4_2                       , None           ,        OpCanCSE          ,      3,   &Js::SIMDFloat32x4Lib::EntryInfo::ReciprocalSqrt, T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Sqrt_F4                   , Float32x4_2                       , None           ,        OpCanCSE          ,      3,   &Js::SIMDFloat32x4Lib::EntryInfo::Sqrt,   T_F4, T_F4)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Swizzle_F4                , Float32x4_2Int4                   , None           ,        OpCanCSE          ,      7,   &Js::SIMDFloat32x4Lib::EntryInfo::Swizzle,   T_F4, T_F4, T_INT, T_INT, T_INT, T_INT)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Shuffle_F4                , Float32x4_3Int4                   , None           ,        OpCanCSE          ,      8,   &Js::SIMDFloat32x4Lib::EntryInfo::Shuffle,   T_F4, T_F4, T_F4, T_INT, T_INT, T_INT, T_INT)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Lt_F4                     , Bool32x4_1Float32x4_2             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS           ( Simd128_LtEq_F4                   , Bool32x4_1Float32x4_2             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Eq_F4                     , Bool32x4_1Float32x4_2             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Neq_F4                    , Bool32x4_1Float32x4_2             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Gt_F4                     , Bool32x4_1Float32x4_2             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS           ( Simd128_GtEq_F4                   , Bool32x4_1Float32x4_2             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS           ( Simd128_Select_F4                 , Float32x4_1Bool32x4_1Float32x4_2  , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_Ld_F4                     , Float32x4_2                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_LdSlot_F4                 , ElementSlot                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_StSlot_F4                 , ElementSlot                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_Return_F4                 , Float32x4_2                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_I_ArgOut_F4               , Reg1Float32x4_1                   , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_I_Conv_VTF4               , Float32x4_2                       , None           ,        None                      )

// Float64x2
#if 0 //Disabling this type until the specification decides to include or not.
MACRO_SIMD_EXTEND_WMS(Simd128_DoublesToD2, Float64x2_1Double2, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Splat_D2, Float64x2_1Double1, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_FromFloat32x4_D2, Float64x2_1Float32x4_1, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_FromFloat32x4Bits_D2, Float64x2_1Float32x4_1, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_FromInt32x4_D2, Float64x2_1Int32x4_1, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_FromInt32x4Bits_D2, Float64x2_1Int32x4_1, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Abs_D2, Float64x2_2, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Neg_D2, Float64x2_2, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Add_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Sub_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Mul_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Div_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Clamp_D2, Float64x2_4, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Min_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Max_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Rcp_D2, Float64x2_2, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_RcpSqrt_D2, Float64x2_2, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Sqrt_D2, Float64x2_2, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Lt_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Select_D2, Float64x2_1Int32x4_1Float64x2_2, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_LdSignMask_D2, Int1Float64x2_1, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_LtEq_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Eq_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Neq_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Gt_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_GtEq_D2, Float64x2_3, None, None, 0)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS(Simd128_Return_D2, Float64x2_2, None, None)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS(Simd128_I_ArgOut_D2, Reg1Float64x2_1, None, None)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS(Simd128_I_Conv_VTD2, Float64x2_2, None, None)

#endif // 0 //Disabling this type until the specification decides to include or not.
//Int8x16
MACRO_SIMD_EXTEND_WMS(Simd128_IntsToI16       , Int8x16_1Int16              , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Splat_I16       , Int8x16_1Int1               , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_ExtractLane_I16 , Int1Int8x16_1Int1           , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_ReplaceLane_I16 , Int8x16_2Int2               , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Swizzle_I16     , Int8x16_2Int16              , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Shuffle_I16     , Int8x16_3Int16              , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Neg_I16         , Int8x16_2                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Add_I16         , Int8x16_3                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Sub_I16         , Int8x16_3                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Mul_I16         , Int8x16_3                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Lt_I16          , Bool8x16_1Int8x16_2         , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_LtEq_I16        , Bool8x16_1Int8x16_2         , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Gt_I16          , Bool8x16_1Int8x16_2         , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_GtEq_I16        , Bool8x16_1Int8x16_2         , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Eq_I16          , Bool8x16_1Int8x16_2         , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Neq_I16         , Bool8x16_1Int8x16_2         , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Select_I16      , Int8x16_1Bool8x16_1Int8x16_2, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_And_I16         , Int8x16_3                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Or_I16          , Int8x16_3                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Xor_I16         , Int8x16_3                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Not_I16         , Int8x16_2                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_ShLtByScalar_I16, Int8x16_2Int1               , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_ShRtByScalar_I16, Int8x16_2Int1               , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_AddSaturate_I16 , Int8x16_3                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_SubSaturate_I16 , Int8x16_3                   , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_LdArr_I16       , AsmSimdTypedArr             , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_LdArrConst_I16  , AsmSimdTypedArr             , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_StArr_I16       , AsmSimdTypedArr             , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_StArrConst_I16  , AsmSimdTypedArr             , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_And_B16         , Bool8x16_3                  , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Or_B16          , Bool8x16_3                  , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Xor_B16         , Bool8x16_3                  , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Not_B16         , Bool8x16_2                  , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Neg_U4          , Uint32x4_2                  , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Neg_U8          , Uint16x8_2                  , None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Neg_U16         , Uint8x16_2                  , None, None, 0)
MACRO_SIMD_BACKEND_ONLY(Simd128_LdC           , Empty                       , None, OpCanCSE) // Load Simd128 const stack slot

#if 0
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS(Simd128_Ld_D2, Float64x2_2, None, None)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS(Simd128_LdSlot_D2, ElementSlot, None, None)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS(Simd128_StSlot_D2, ElementSlot, None, None)

MACRO_SIMD_EXTEND_WMS(Simd128_LdArr_D2, AsmSimdTypedArr, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_LdArrConst_D2, AsmSimdTypedArr, None, None, 0)

MACRO_SIMD_EXTEND_WMS(Simd128_Swizzle_D2, Float64x2_2Int2, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_Shuffle_D2, Float64x2_3Int2, None, None, 0)

MACRO_SIMD_EXTEND_WMS(Simd128_StArr_D2, AsmSimdTypedArr, None, None, 0)
MACRO_SIMD_EXTEND_WMS(Simd128_StArrConst_D2, AsmSimdTypedArr, None, None, 0)
#endif // 0



MACRO_SIMD_ASMJS_ONLY_WMS          ( Simd128_Ld_I8              , Int16x8_2                         , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_WMS          ( Simd128_LdSlot_I8          , ElementSlot                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_StSlot_I8          , ElementSlot                       , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_WMS          ( Simd128_Ld_U4              , Uint32x4_2                        , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_LdSlot_U4          , ElementSlot                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_StSlot_U4          , ElementSlot                       , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_WMS          ( Simd128_Ld_U8              , Uint16x8_2                        , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_LdSlot_U8          , ElementSlot                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_StSlot_U8          , ElementSlot                       , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_WMS          ( Simd128_Ld_U16             , Uint8x16_2                        , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_LdSlot_U16         , ElementSlot                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_StSlot_U16         , ElementSlot                       , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_ArgOut_I8        , Reg1Int16x8_1                     , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_Conv_VTI8        , Int16x8_2                         , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_Return_I8          , Int16x8_2                         , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_ArgOut_U4        , Reg1Uint32x4_1                    , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_Conv_VTU4        , Uint32x4_2                        , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_Return_U4          , Uint32x4_2                        , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_ArgOut_U8        , Reg1Uint16x8_1                    , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_Conv_VTU8        , Uint16x8_2                        , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_Return_U8          , Uint16x8_2                        , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_ArgOut_U16        , Reg1Uint8x16_1                   , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_Conv_VTU16        , Uint8x16_2                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_Return_U16          , Uint8x16_2                       , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_ArgOut_B4         , Reg1Bool32x4_1                   , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_Conv_VTB4         , Bool32x4_2                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_Return_B4           , Bool32x4_2                       , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_ArgOut_B8         , Reg1Bool16x8_1                   , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_Conv_VTB8         , Bool16x8_2                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_Return_B8           , Bool16x8_2                       , None           ,        None                      )

MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_ArgOut_B16        , Reg1Bool8x16_1                   , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_I_Conv_VTB16        , Bool8x16_2                       , None           ,        None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS   ( Simd128_Return_B16          , Bool8x16_2                       , None           ,        None                      )

MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArr_I4                    , AsmSimdTypedArr                   , None           ,        OpCanCSE          ,      4, &Js::SIMDInt32x4Lib::EntryInfo::Load, T_I4, ValueType::GetObject(ObjectType::Int8Array) /*dummy place-holder for any typed array*/, T_INT)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArrConst_I4               , AsmSimdTypedArr                   , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArr_F4                    , AsmSimdTypedArr                   , None           ,        OpCanCSE          ,      4, &Js::SIMDFloat32x4Lib::EntryInfo::Load, T_F4, ValueType::GetObject(ObjectType::Int8Array) /*dummy place-holder for any typed array*/, T_INT)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArrConst_F4               , AsmSimdTypedArr                   , None           ,        OpCanCSE          ,      0)

MACRO_SIMD_EXTEND_WMS     ( Simd128_StArr_I4                    , AsmSimdTypedArr                   , None           ,        OpCanCSE          ,      5, &Js::SIMDInt32x4Lib::EntryInfo::Store, ValueType::Undefined, ValueType::GetObject(ObjectType::Int8Array) /*dummy place-holder for any typed array*/, T_INT, T_I4)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArrConst_I4               , AsmSimdTypedArr                   , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArr_F4                    , AsmSimdTypedArr                   , None           ,        OpCanCSE          ,      5, &Js::SIMDFloat32x4Lib::EntryInfo::Store, ValueType::Undefined, ValueType::GetObject(ObjectType::Int8Array) /*dummy place-holder for any typed array*/, T_INT, T_F4)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArrConst_F4               , AsmSimdTypedArr                   , None           ,        OpCanCSE          ,      0)

MACRO_SIMD_EXTEND_WMS     ( Simd128_ExtractLane_I4              , Int1Int32x4_1Int1                 , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ReplaceLane_I4              , Int32x4_2Int2                     , None           ,        OpCanCSE          ,      0)

MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint32x4Bits_I4         , Int32x4_1Uint32x4_1               , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt16x8Bits_I4          , Int32x4_1Int16x8_1                , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint16x8Bits_I4         , Int32x4_1Uint16x8_1               , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt8x16Bits_I4          , Int32x4_1Int8x16_1                , None           ,        OpCanCSE         ,       0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint8x16Bits_I4         , Int32x4_1Uint8x16_1               , None           ,        OpCanCSE         ,       0)

MACRO_SIMD_EXTEND_WMS     ( Simd128_ExtractLane_F4              , Float1Float32x4_1Int1             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ReplaceLane_F4              , Float32x4_2Int1Float1             , None           ,        OpCanCSE          ,      0)

MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint32x4_F4             , Float32x4_1Uint32x4_1             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint32x4Bits_F4         , Float32x4_1Uint32x4_1             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt16x8Bits_F4          , Float32x4_1Int16x8_1              , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint16x8Bits_F4         , Float32x4_1Uint16x8_1             , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt8x16Bits_F4          , Float32x4_1Int8x16_1              , None           ,        OpCanCSE          ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint8x16Bits_F4         , Float32x4_1Uint8x16_1             , None           ,        OpCanCSE          ,      0)

MACRO_SIMD_EXTEND_WMS     ( Simd128_LtEq_I4                     , Bool32x4_1Int32x4_2              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_GtEq_I4                     , Bool32x4_1Int32x4_2              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Neq_I4                      , Bool32x4_1Int32x4_2              , None           ,        None               ,      0)
// Int16x8
MACRO_SIMD_EXTEND_WMS     ( Simd128_IntsToI8                    , Int16x8_1Int8                    , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ExtractLane_I8              , Int1Int16x8_1Int1                , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ReplaceLane_I8              , Int16x8_2Int2                    , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Swizzle_I8                  , Int16x8_2Int8                    , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Shuffle_I8                  , Int16x8_3Int8                    , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Splat_I8                    , Int16x8_1Int1                    , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_And_I8                      , Int16x8_3                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Or_I8                       , Int16x8_3                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Xor_I8                      , Int16x8_3                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Not_I8                      , Int16x8_2                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Add_I8                      , Int16x8_3                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Sub_I8                      , Int16x8_3                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Mul_I8                      , Int16x8_3                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Neg_I8                      , Int16x8_2                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Eq_I8                       , Bool16x8_1Int16x8_2              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Neq_I8                      , Bool16x8_1Int16x8_2              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Lt_I8                       , Bool16x8_1Int16x8_2              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LtEq_I8                     , Bool16x8_1Int16x8_2              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Gt_I8                       , Bool16x8_1Int16x8_2              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_GtEq_I8                     , Bool16x8_1Int16x8_2              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ShLtByScalar_I8             , Int16x8_2Int1                    , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ShRtByScalar_I8             , Int16x8_2Int1                    , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Select_I8                   , Int16x8_1Bool16x8_1Int16x8_2     , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_AddSaturate_I8              , Int16x8_3                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_SubSaturate_I8              , Int16x8_3                        , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArr_I8                    , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArrConst_I8               , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArr_I8                    , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArrConst_I8               , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromFloat32x4Bits_I8        , Int16x8_1Float32x4_1             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt32x4Bits_I8          , Int16x8_1Int32x4_1               , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt8x16Bits_I8          , Int16x8_1Int8x16_1               , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint32x4Bits_I8         , Int16x8_1Uint32x4_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint16x8Bits_I8         , Int16x8_1Uint16x8_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint8x16Bits_I8         , Int16x8_1Uint8x16_1              , None           ,        None               ,      0)

// int8x16
MACRO_SIMD_EXTEND_WMS     (Simd128_FromFloat32x4Bits_I16     , Int8x16_1Float32x4_1                 , None         ,          None              ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_FromInt32x4Bits_I16       , Int8x16_1Int32x4_1                   , None         ,          None              ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_FromInt16x8Bits_I16       , Int8x16_1Int16x8_1                   , None         ,          None              ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_FromUint32x4Bits_I16      , Int8x16_1Uint32x4_1                  , None         ,          None              ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_FromUint16x8Bits_I16      , Int8x16_1Uint16x8_1                  , None         ,          None              ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_FromUint8x16Bits_I16      , Int8x16_1Uint8x16_1                  , None         ,          None              ,      0)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS    (Simd128_Ld_I16          , Int8x16_2                            , None         ,          None                      )
MACRO_SIMD_ASMJS_ONLY_WMS           (Simd128_LdSlot_I16      , ElementSlot                          , None         ,          None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS    (Simd128_StSlot_I16      , ElementSlot                          , None         ,          None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS    (Simd128_Return_I16      , Int8x16_2                            , None         ,          None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS    (Simd128_I_ArgOut_I16    , Reg1Int8x16_1                        , None         ,          None                      )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS    (Simd128_I_Conv_VTI16    , Int8x16_2                            , None         ,          None                      )

// Uint32x4
MACRO_SIMD_EXTEND_WMS     ( Simd128_IntsToU4                    , Uint32x4_1Int4                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ExtractLane_U4              , Int1Uint32x4_1Int1               , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ReplaceLane_U4              , Uint32x4_2Int2                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Swizzle_U4                  , Uint32x4_2Int4                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Shuffle_U4                  , Uint32x4_3Int4                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Splat_U4                    , Uint32x4_1Int1                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_And_U4                      , Uint32x4_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Or_U4                       , Uint32x4_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Xor_U4                      , Uint32x4_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Not_U4                      , Uint32x4_2                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Add_U4                      , Uint32x4_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Sub_U4                      , Uint32x4_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Mul_U4                      , Uint32x4_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Eq_U4                       , Bool32x4_1Uint32x4_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Neq_U4                      , Bool32x4_1Uint32x4_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Lt_U4                       , Bool32x4_1Uint32x4_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LtEq_U4                     , Bool32x4_1Uint32x4_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Gt_U4                       , Bool32x4_1Uint32x4_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_GtEq_U4                     , Bool32x4_1Uint32x4_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ShLtByScalar_U4             , Uint32x4_2Int1                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ShRtByScalar_U4             , Uint32x4_2Int1                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Select_U4                   , Uint32x4_1Bool32x4_1Uint32x4_2   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArr_U4                    , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArrConst_U4               , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArr_U4                    , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArrConst_U4               , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromFloat32x4_U4            , Uint32x4_1Float32x4_1            , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromFloat32x4Bits_U4        , Uint32x4_1Float32x4_1            , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt32x4Bits_U4          , Uint32x4_1Int32x4_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt16x8Bits_U4          , Uint32x4_1Int16x8_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt8x16Bits_U4          , Uint32x4_1Int8x16_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint16x8Bits_U4         , Uint32x4_1Uint16x8_1             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint8x16Bits_U4         , Uint32x4_1Uint8x16_1             , None           ,        None               ,      0)

// Uint16x8
MACRO_SIMD_EXTEND_WMS     ( Simd128_IntsToU8                    , Uint16x8_1Int8                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ExtractLane_U8              , Int1Uint16x8_1Int1               , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ReplaceLane_U8              , Uint16x8_2Int2                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Swizzle_U8                  , Uint16x8_2Int8                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Shuffle_U8                  , Uint16x8_3Int8                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Splat_U8                    , Uint16x8_1Int1                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_And_U8                      , Uint16x8_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Or_U8                       , Uint16x8_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Xor_U8                      , Uint16x8_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Not_U8                      , Uint16x8_2                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Add_U8                      , Uint16x8_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Sub_U8                      , Uint16x8_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Mul_U8                      , Uint16x8_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ShLtByScalar_U8             , Uint16x8_2Int1                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ShRtByScalar_U8             , Uint16x8_2Int1                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_Lt_U8                        , Bool16x8_1Uint16x8_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_LtEq_U8                      , Bool16x8_1Uint16x8_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_Gt_U8                        , Bool16x8_1Uint16x8_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_GtEq_U8                      , Bool16x8_1Uint16x8_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_Eq_U8                        , Bool16x8_1Uint16x8_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_Neq_U8                       , Bool16x8_1Uint16x8_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     (Simd128_Select_U8                    , Uint16x8_1Bool16x8_1Uint16x8_2   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_AddSaturate_U8              , Uint16x8_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_SubSaturate_U8              , Uint16x8_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArr_U8                    , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArrConst_U8               , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArr_U8                    , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArrConst_U8               , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromFloat32x4Bits_U8        , Uint16x8_1Float32x4_1            , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt32x4Bits_U8          , Uint16x8_1Int32x4_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt16x8Bits_U8          , Uint16x8_1Int16x8_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt8x16Bits_U8          , Uint16x8_1Int8x16_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint32x4Bits_U8         , Uint16x8_1Uint32x4_1             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint8x16Bits_U8         , Uint16x8_1Uint8x16_1             , None           ,        None               ,      0)

// Uint8x16
MACRO_SIMD_EXTEND_WMS     ( Simd128_IntsToU16                   , Uint8x16_1Int16                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ExtractLane_U16             , Int1Uint8x16_1Int1               , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ReplaceLane_U16             , Uint8x16_2Int2                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Swizzle_U16                 , Uint8x16_2Int16                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Shuffle_U16                 , Uint8x16_3Int16                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Splat_U16                   , Uint8x16_1Int1                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_And_U16                     , Uint8x16_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Or_U16                      , Uint8x16_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Xor_U16                     , Uint8x16_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Not_U16                     , Uint8x16_2                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Add_U16                     , Uint8x16_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Sub_U16                     , Uint8x16_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Mul_U16                     , Uint8x16_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ShLtByScalar_U16            , Uint8x16_2Int1                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_ShRtByScalar_U16            , Uint8x16_2Int1                   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Lt_U16                      , Bool8x16_1Uint8x16_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LtEq_U16                    , Bool8x16_1Uint8x16_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Gt_U16                      , Bool8x16_1Uint8x16_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_GtEq_U16                    , Bool8x16_1Uint8x16_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Eq_U16                      , Bool8x16_1Uint8x16_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Neq_U16                     , Bool8x16_1Uint8x16_2             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_Select_U16                  , Uint8x16_1Bool8x16_1Uint8x16_2   , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_AddSaturate_U16             , Uint8x16_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_SubSaturate_U16             , Uint8x16_3                       , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArr_U16                   , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_LdArrConst_U16              , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArr_U16                   , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_StArrConst_U16              , AsmSimdTypedArr                  , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromFloat32x4Bits_U16       , Uint8x16_1Float32x4_1            , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt32x4Bits_U16         , Uint8x16_1Int32x4_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt16x8Bits_U16         , Uint8x16_1Int16x8_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromInt8x16Bits_U16         , Uint8x16_1Int8x16_1              , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint32x4Bits_U16        , Uint8x16_1Uint32x4_1             , None           ,        None               ,      0)
MACRO_SIMD_EXTEND_WMS     ( Simd128_FromUint16x8Bits_U16        , Uint8x16_1Uint16x8_1             , None           ,        None               ,      0)

//Bool32x4
MACRO_SIMD_EXTEND_WMS       ( Simd128_IntsToB4                  , Bool32x4_1Int4                     , None           ,        OpCanCSE         ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_Splat_B4                  , Bool32x4_1Int1                     , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_ExtractLane_B4            , Int1Bool32x4_1Int1                 , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_ReplaceLane_B4            , Bool32x4_2Int2                     , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_And_B4                    , Bool32x4_3                         , None           ,        OpCanCSE         ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_Or_B4                     , Bool32x4_3                         , None           ,        OpCanCSE         ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_Xor_B4                    , Bool32x4_3                         , None           ,        OpCanCSE         ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_Not_B4                    , Bool32x4_2                         , None           ,        OpCanCSE         ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_AnyTrue_B4                , Int1Bool32x4_1                     , None           ,        OpCanCSE         ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_AllTrue_B4                , Int1Bool32x4_1                     , None           ,        OpCanCSE         ,      0)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_Ld_B4                 , Bool32x4_2                         , None           ,        None                     )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_LdSlot_B4             , ElementSlot                        , None           ,        None                     )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_StSlot_B4             , ElementSlot                        , None           ,        None                     )

//Bool16x8
MACRO_SIMD_EXTEND_WMS       ( Simd128_IntsToB8                  , Bool16x8_1Int8                     , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_Splat_B8                  , Bool16x8_1Int1                     , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_ExtractLane_B8            , Int1Bool16x8_1Int1                 , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_ReplaceLane_B8            , Bool16x8_2Int2                     , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_And_B8                    , Bool16x8_3                         , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_Or_B8                     , Bool16x8_3                         , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_Xor_B8                    , Bool16x8_3                         , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_Not_B8                    , Bool16x8_2                         , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_AnyTrue_B8                , Int1Bool16x8_1                     , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_AllTrue_B8                , Int1Bool16x8_1                     , None           ,        None             ,      0)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_Ld_B8                 , Bool16x8_2                         , None           ,        None                     )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_LdSlot_B8             , ElementSlot                        , None           ,        None                     )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_StSlot_B8             , ElementSlot                        , None           ,        None                     )

//Bool8x16
MACRO_SIMD_EXTEND_WMS       ( Simd128_IntsToB16                 , Bool8x16_1Int16                    , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_Splat_B16                 , Bool8x16_1Int1                     , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_ExtractLane_B16           , Int1Bool8x16_1Int1                 , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_ReplaceLane_B16           , Bool8x16_2Int2                     , None           ,        None             ,      0)
//MACRO_SIMD_EXTEND_WMS     ( Simd128_And_B16                   , Bool8x16_3                         , None           ,        None             ,      0)
//MACRO_SIMD_EXTEND_WMS     ( Simd128_Or_B16                    , Bool8x16_3                         , None           ,        None             ,      0)
//MACRO_SIMD_EXTEND_WMS     ( Simd128_Xor_B16                   , Bool8x16_3                         , None           ,        None             ,      0)
//MACRO_SIMD_EXTEND_WMS     ( Simd128_Not_B16                   , Bool8x16_2                         , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_AnyTrue_B16               , Int1Bool8x16_1                     , None           ,        None             ,      0)
MACRO_SIMD_EXTEND_WMS       ( Simd128_AllTrue_B16               , Int1Bool8x16_1                     , None           ,        None             ,      0)
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_Ld_B16                , Bool8x16_2                         , None           ,        None                     )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_LdSlot_B16            , ElementSlot                        , None           ,        None                     )
MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS( Simd128_StSlot_B16            , ElementSlot                        , None           ,        None                     )

MACRO_SIMD_EXTEND         ( Simd128_End_Extend                  , Empty                            , None           ,        None               ,      0)   // Just a marker to indicate SIMD opcodes region
#undef T_F4
#undef T_I4
#undef T_INT
#undef T_FLT

#undef MACRO_SIMD
#undef MACRO_SIMD_WMS
#undef MACRO_SIMD_ASMJS_ONLY_WMS
#undef MACRO_SIMD_BACKEND_ONLY

#undef MACRO_SIMD_EXTEND
#undef MACRO_SIMD_EXTEND_WMS
#undef MACRO_SIMD_ASMJS_ONLY_EXTEND_WMS
#undef MACRO_SIMD_BACKEND_ONLY_EXTEND
