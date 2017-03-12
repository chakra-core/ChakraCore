//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// NOTE: This file is intended to be "#include" multiple times. The call site must define the macros
// "LAYOUT_TYPE", etc., to be executed for each entry.
//

#ifndef LAYOUT_TYPE
#define LAYOUT_TYPE(layout)
#endif

#ifndef LAYOUT_TYPE_WMS
#define LAYOUT_TYPE_WMS(layout) \
    LAYOUT_TYPE(layout##_Small) \
    LAYOUT_TYPE(layout##_Medium) \
    LAYOUT_TYPE(layout##_Large)
#endif

#ifndef LAYOUT_TYPE_WMS_REG2
#define LAYOUT_TYPE_WMS_REG2(layout, t0, t1) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG3
#define LAYOUT_TYPE_WMS_REG3(layout, t0, t1, t2) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG4
#define LAYOUT_TYPE_WMS_REG4(layout, t0, t1, t2, t3) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG5
#define LAYOUT_TYPE_WMS_REG5(layout, t0, t1, t2, t3, t4) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG6
#define LAYOUT_TYPE_WMS_REG6(layout, t0, t1, t2, t3, t4, t5) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG7
#define LAYOUT_TYPE_WMS_REG7(layout, t0, t1, t2, t3, t4, t5, t6) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG9
#define LAYOUT_TYPE_WMS_REG9(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG10
#define LAYOUT_TYPE_WMS_REG10(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG11
#define LAYOUT_TYPE_WMS_REG11(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG17
#define LAYOUT_TYPE_WMS_REG17(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG18
#define LAYOUT_TYPE_WMS_REG18(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17) LAYOUT_TYPE_WMS(layout)
#endif

#ifndef LAYOUT_TYPE_WMS_REG19
#define LAYOUT_TYPE_WMS_REG19(layout, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18) LAYOUT_TYPE_WMS(layout)
#endif

#ifdef ASMJS_PLAT

// FE for frontend only layout
#ifdef EXCLUDE_FRONTEND_LAYOUT
#ifndef LAYOUT_TYPE_WMS_FE
#define LAYOUT_TYPE_WMS_FE(...)
#endif
#else
#ifndef LAYOUT_TYPE_WMS_FE
#define LAYOUT_TYPE_WMS_FE LAYOUT_TYPE_WMS
#endif
#endif

// For duplicates layout from LayoutTypes.h
#ifdef EXCLUDE_DUP_LAYOUT
#define LAYOUT_TYPE_DUP(...)
#define LAYOUT_TYPE_WMS_DUP(...)
#else
#define LAYOUT_TYPE_DUP LAYOUT_TYPE
#define LAYOUT_TYPE_WMS_DUP LAYOUT_TYPE_WMS
#endif

#define LAYOUT_PREFIX_Reg() R
#define LAYOUT_PREFIX_Int() I
#define LAYOUT_PREFIX_Long() L
#define LAYOUT_PREFIX_Float() F
#define LAYOUT_PREFIX_Double() D
#define LAYOUT_PREFIX_IntConst() C
#define LAYOUT_PREFIX_LongConst() C
#define LAYOUT_PREFIX_FloatConst() C
#define LAYOUT_PREFIX_DoubleConst() C
#define LAYOUT_PREFIX_Float32x4() F4_
#define LAYOUT_PREFIX_Bool32x4() B4_
#define LAYOUT_PREFIX_Int32x4() I4_
#define LAYOUT_PREFIX_Float64x2() F2_
#define LAYOUT_PREFIX_Int16x8() I8_
#define LAYOUT_PREFIX_Bool16x8() B8_
#define LAYOUT_PREFIX_Int8x16() I16_
#define LAYOUT_PREFIX_Bool8x16() B16_
#define LAYOUT_PREFIX_Uint32x4() U4_
#define LAYOUT_PREFIX_Uint16x8() U8_
#define LAYOUT_PREFIX_Uint8x16() U16_

// These layout are already defined in LayoutTypes.h
// We redeclare them here to keep the same layout and use them
// in other contexts.
LAYOUT_TYPE_WMS_DUP   ( ElementSlot   )
LAYOUT_TYPE_DUP       ( StartCall     )
LAYOUT_TYPE_DUP       ( Empty         )

LAYOUT_TYPE_WMS       ( AsmTypedArr   )
LAYOUT_TYPE_WMS       ( WasmMemAccess )
LAYOUT_TYPE_WMS       ( AsmCall       )
LAYOUT_TYPE           ( AsmBr         )
LAYOUT_TYPE_WMS       ( AsmReg1       ) // Generic layout with 1 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg2       ) // Generic layout with 2 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg3       ) // Generic layout with 3 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg4       ) // Generic layout with 4 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg5       ) // Generic layout with 5 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg6       ) // Generic layout with 6 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg7       ) // Generic layout with 7 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg9       ) // Generic layout with 9 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg10      ) // Generic layout with 10 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg11      ) // Generic layout with 11 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg17      ) // Generic layout with 17 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg18      ) // Generic layout with 18 RegSlot
LAYOUT_TYPE_WMS_FE    ( AsmReg19      ) // Generic layout with 19 RegSlot
LAYOUT_TYPE_WMS_REG2  ( Int1Double1   , Int, Double) // 1 int register and 1 double register
LAYOUT_TYPE_WMS_REG2  ( Int1Float1    , Int, Float) // 1 int register and 1 float register
LAYOUT_TYPE_WMS_REG2  ( Double1Const1 , Double, DoubleConst) // 1 double register and 1 const double value
LAYOUT_TYPE_WMS_REG2  ( Double1Int1   , Double, Int) // 1 double register and 1 int register
LAYOUT_TYPE_WMS_REG2  ( Double1Float1 , Double, Float) // 1 double register and 1 float register
LAYOUT_TYPE_WMS_REG2  ( Double1Reg1   , Double, Reg) // 1 double register and 1 var register
LAYOUT_TYPE_WMS_REG2  ( Float1Reg1    , Float, Reg) // 1 double register and 1 var register
LAYOUT_TYPE_WMS_REG2  ( Int1Reg1      , Int, Reg) // 1 int register and 1 var register
LAYOUT_TYPE_WMS_REG2  ( Reg1Double1   , Reg, Double) // 1 var register and 1 double register
LAYOUT_TYPE_WMS_REG2  ( Reg1Float1    , Reg, Float) // 1 var register and 1 Float register
LAYOUT_TYPE_WMS_REG2  ( Reg1Int1      , Reg, Int) // 1 var register and 1 int register
LAYOUT_TYPE_WMS_REG2  ( Int1Const1    , Int, IntConst) // 1 int register and 1 const int value
LAYOUT_TYPE_WMS_REG2  ( Reg1IntConst1 , Reg, IntConst) // 1 int register and 1 const int value
LAYOUT_TYPE_WMS_REG3  ( Int1Double2   , Int, Double, Double) // 1 int register and 2 double register ( double comparisons )
LAYOUT_TYPE_WMS_REG3  ( Int1Float2    , Int, Float, Float) // 1 int register and 2 float register ( float comparisons )
LAYOUT_TYPE_WMS_REG2  ( Int2          , Int, Int) // 2 int register
LAYOUT_TYPE_WMS_REG3  ( Int3          , Int, Int, Int) // 3 int register
LAYOUT_TYPE_WMS_REG2  ( Double2       , Double, Double) // 2 double register
LAYOUT_TYPE_WMS_REG2  ( Float2        , Float, Float) // 2 float register
LAYOUT_TYPE_WMS_REG3  ( Float3        , Float, Float, Float) // 3 float register
LAYOUT_TYPE_WMS_REG2  ( Float1Const1  , Float, FloatConst) // 1 float register and 1 const float value
LAYOUT_TYPE_WMS_REG2  ( Float1Double1 , Float, Double) // 1 float register and 1 double register
LAYOUT_TYPE_WMS_REG2  ( Float1Int1    , Float, Int) // 2 double register
LAYOUT_TYPE_WMS_REG3  ( Double3       , Double, Double, Double) // 3 double register
LAYOUT_TYPE_WMS       ( BrInt1        ) // Conditional branching with 1 int
LAYOUT_TYPE_WMS       ( BrInt2        ) // Conditional branching with 2 int
LAYOUT_TYPE_WMS       ( BrInt1Const1  ) // Conditional branching with 1 int and 1 const
LAYOUT_TYPE_WMS       ( AsmUnsigned1  ) // 1 unsigned int register

// Int64
LAYOUT_TYPE_WMS_REG2  ( Long1Reg1   , Long, Reg)
LAYOUT_TYPE_WMS_REG2  ( Reg1Long1   , Reg, Long)
LAYOUT_TYPE_WMS_REG2  ( Long1Const1 , Long, LongConst)
LAYOUT_TYPE_WMS_REG2  ( Long2       , Long, Long)
LAYOUT_TYPE_WMS_REG3  ( Long3       , Long, Long, Long)
LAYOUT_TYPE_WMS_REG3  ( Int1Long2   , Int, Long, Long)
LAYOUT_TYPE_WMS_REG2  ( Long1Int1   , Long, Int)
LAYOUT_TYPE_WMS_REG2  ( Int1Long1   , Int, Long)
LAYOUT_TYPE_WMS_REG2  ( Long1Float1 , Long, Float)
LAYOUT_TYPE_WMS_REG2  ( Float1Long1 , Float, Long)
LAYOUT_TYPE_WMS_REG2  ( Long1Double1, Long, Double)
LAYOUT_TYPE_WMS_REG2  ( Double1Long1, Double, Long)

// Float32x4
LAYOUT_TYPE_WMS_REG2  ( Float32x4_2                      , Float32x4, Float32x4)
LAYOUT_TYPE_WMS_REG3  ( Float32x4_3                      , Float32x4, Float32x4, Float32x4)
LAYOUT_TYPE_WMS_REG4  ( Float32x4_4                      , Float32x4, Float32x4, Float32x4, Float32x4)
LAYOUT_TYPE_WMS_REG3  ( Bool32x4_1Float32x4_2            , Bool32x4, Float32x4, Float32x4)
LAYOUT_TYPE_WMS_REG4  ( Float32x4_1Bool32x4_1Float32x4_2 , Float32x4, Bool32x4, Float32x4, Float32x4)
LAYOUT_TYPE_WMS_REG5  ( Float32x4_1Float4                , Float32x4, Float, Float, Float, Float)
LAYOUT_TYPE_WMS_REG6  ( Float32x4_2Int4                  , Float32x4, Float32x4, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG7  ( Float32x4_3Int4                  , Float32x4, Float32x4, Float32x4, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Float32x4_1Float1                , Float32x4, Float)
LAYOUT_TYPE_WMS_REG3  ( Float32x4_2Float1                , Float32x4, Float32x4, Float)
//LAYOUT_TYPE_WMS_REG2( Float32x4_1Float64x2_1           , Float32x4, Float64x2)
LAYOUT_TYPE_WMS_REG2  ( Float32x4_1Int32x4_1             , Float32x4, Int32x4)
LAYOUT_TYPE_WMS_REG2  ( Float32x4_1Uint32x4_1            , Float32x4, Uint32x4)
LAYOUT_TYPE_WMS_REG2  ( Float32x4_1Int16x8_1             , Float32x4, Int16x8)
LAYOUT_TYPE_WMS_REG2  ( Float32x4_1Uint16x8_1            , Float32x4, Uint16x8)
LAYOUT_TYPE_WMS_REG2  ( Float32x4_1Int8x16_1             , Float32x4, Int8x16)
LAYOUT_TYPE_WMS_REG2  ( Float32x4_1Uint8x16_1            , Float32x4, Uint8x16)
LAYOUT_TYPE_WMS_REG2  ( Reg1Float32x4_1                  , Reg, Float32x4)
LAYOUT_TYPE_WMS_REG3  ( Float1Float32x4_1Int1            , Float, Float32x4, Int)
LAYOUT_TYPE_WMS_REG4  ( Float32x4_2Int1Float1            , Float32x4, Float32x4, Int, Float)
// Int32x4_2
LAYOUT_TYPE_WMS_REG2  ( Int32x4_2                        , Int32x4, Int32x4)
LAYOUT_TYPE_WMS_REG3  ( Int32x4_3                        , Int32x4, Int32x4, Int32x4)
LAYOUT_TYPE_WMS_REG3  ( Bool32x4_1Int32x4_2              , Bool32x4, Int32x4, Int32x4)
LAYOUT_TYPE_WMS_REG4  ( Int32x4_1Bool32x4_1Int32x4_2     , Int32x4, Bool32x4, Int32x4, Int32x4)
LAYOUT_TYPE_WMS_REG5  ( Int32x4_1Int4                    , Int32x4, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG6  ( Int32x4_2Int4                    , Int32x4, Int32x4, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG7  ( Int32x4_3Int4                    , Int32x4, Int32x4, Int32x4, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Int32x4_1Int1                    , Int32x4, Int)
LAYOUT_TYPE_WMS_REG3  ( Int32x4_2Int1                    , Int32x4, Int32x4, Int)
LAYOUT_TYPE_WMS_REG2  ( Reg1Int32x4_1                    , Reg, Int32x4)
LAYOUT_TYPE_WMS_REG2  ( Int32x4_1Float32x4_1             , Int32x4, Float32x4)
//LAYOUT_TYPE_WMS_REG2( Int32x4_1Float64x2_1             , Int32x4, Float64x2)
LAYOUT_TYPE_WMS_REG2  ( Int32x4_1Uint32x4_1              , Int32x4, Uint32x4)
LAYOUT_TYPE_WMS_REG2  ( Int32x4_1Int16x8_1               , Int32x4, Int16x8)
LAYOUT_TYPE_WMS_REG2  ( Int32x4_1Uint16x8_1              , Int32x4, Uint16x8)
LAYOUT_TYPE_WMS_REG2  ( Int32x4_1Int8x16_1               , Int32x4, Int8x16)
LAYOUT_TYPE_WMS_REG2  ( Int32x4_1Uint8x16_1              , Int32x4, Uint8x16)
LAYOUT_TYPE_WMS_REG3  ( Int1Int32x4_1Int1                , Int, Int32x4, Int)
LAYOUT_TYPE_WMS_REG4  ( Int32x4_2Int2                    , Int32x4, Int32x4, Int, Int)
// Float64x2
// Disabled for now
#if 0
LAYOUT_TYPE_WMS_REG2  ( Float64x2_2                      , Float64x2, Float64x2)
LAYOUT_TYPE_WMS_REG3  ( Float64x2_3                      , Float64x2, Float64x2, Float64x2)
LAYOUT_TYPE_WMS_REG4  ( Float64x2_4                      , Float64x2, Float64x2, Float64x2, Float64x2)
LAYOUT_TYPE_WMS_REG3  ( Float64x2_1Double2               , Float64x2, Double, Double)
LAYOUT_TYPE_WMS_REG2  ( Float64x2_1Double1               , Float64x2, Double)
LAYOUT_TYPE_WMS_REG3  ( Float64x2_2Double1               , Float64x2, Float64x2, Double)
LAYOUT_TYPE_WMS_REG4  ( Float64x2_2Int2                  , Float64x2, Float64x2, Int, Int)
LAYOUT_TYPE_WMS_REG5  ( Float64x2_3Int2                  , Float64x2, Float64x2, Float64x2, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Float64x2_1Float32x4_1           , Float64x2, Float32x4)
LAYOUT_TYPE_WMS_REG2  ( Float64x2_1Int32x4_1             , Float64x2, Int32x4)
LAYOUT_TYPE_WMS_REG4  ( Float64x2_1Int32x4_1Float64x2_2  , Float64x2, Int32x4, Float64x2, Float64x2)
LAYOUT_TYPE_WMS_REG2  ( Reg1Float64x2_1                  , Reg, Float64x2)
#endif //0

// Int16x8
LAYOUT_TYPE_WMS_REG9  ( Int16x8_1Int8                    , Int16x8, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Reg1Int16x8_1                    , Reg, Int16x8)
LAYOUT_TYPE_WMS_REG2  ( Int16x8_2                        , Int16x8, Int16x8)
LAYOUT_TYPE_WMS_REG3  ( Int1Int16x8_1Int1                , Int, Int16x8, Int)
LAYOUT_TYPE_WMS_REG10 ( Int16x8_2Int8                    , Int16x8, Int16x8, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG11 ( Int16x8_3Int8                    , Int16x8, Int16x8, Int16x8, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Int16x8_1Int1                    , Int16x8, Int)
LAYOUT_TYPE_WMS_REG4  ( Int16x8_2Int2                    , Int16x8, Int16x8, Int, Int)
LAYOUT_TYPE_WMS_REG3  ( Int16x8_3                        , Int16x8, Int16x8, Int16x8)
LAYOUT_TYPE_WMS_REG3  ( Bool16x8_1Int16x8_2              , Bool16x8, Int16x8, Int16x8)
LAYOUT_TYPE_WMS_REG4  ( Int16x8_1Bool16x8_1Int16x8_2     , Int16x8, Bool16x8, Int16x8, Int16x8)
LAYOUT_TYPE_WMS_REG3  ( Int16x8_2Int1                    , Int16x8, Int16x8, Int)
LAYOUT_TYPE_WMS_REG2  ( Int16x8_1Float32x4_1             , Int16x8, Float32x4)
LAYOUT_TYPE_WMS_REG2  ( Int16x8_1Int32x4_1               , Int16x8, Int32x4)
LAYOUT_TYPE_WMS_REG2  ( Int16x8_1Int8x16_1               , Int16x8, Int8x16)
LAYOUT_TYPE_WMS_REG2  ( Int16x8_1Uint32x4_1              , Int16x8, Uint32x4)
LAYOUT_TYPE_WMS_REG2  ( Int16x8_1Uint16x8_1              , Int16x8, Uint16x8)
LAYOUT_TYPE_WMS_REG2  ( Int16x8_1Uint8x16_1              , Int16x8, Uint8x16)
// Int8x16
LAYOUT_TYPE_WMS_REG2  ( Int8x16_2                        , Int8x16, Int8x16)
LAYOUT_TYPE_WMS_REG3  ( Int8x16_3                        , Int8x16, Int8x16, Int8x16)
LAYOUT_TYPE_WMS_REG17 ( Int8x16_1Int16                   , Int8x16, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG18 ( Int8x16_2Int16                   , Int8x16, Int8x16, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG19 ( Int8x16_3Int16                   , Int8x16, Int8x16, Int8x16, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Int8x16_1Int1                    , Int8x16, Int)
LAYOUT_TYPE_WMS_REG3  ( Int8x16_2Int1                    , Int8x16, Int8x16, Int)
LAYOUT_TYPE_WMS_REG2  ( Reg1Int8x16_1                    , Reg, Int8x16)
LAYOUT_TYPE_WMS_REG3  ( Bool8x16_1Int8x16_2              , Bool8x16, Int8x16, Int8x16)
LAYOUT_TYPE_WMS_REG4  ( Int8x16_1Bool8x16_1Int8x16_2     , Int8x16, Bool8x16, Int8x16, Int8x16)
LAYOUT_TYPE_WMS_REG2  ( Int8x16_1Float32x4_1             , Int8x16, Float32x4)
LAYOUT_TYPE_WMS_REG2  ( Int8x16_1Int32x4_1               , Int8x16, Int32x4)
LAYOUT_TYPE_WMS_REG2  ( Int8x16_1Int16x8_1               , Int8x16, Int16x8)
LAYOUT_TYPE_WMS_REG2  ( Int8x16_1Uint32x4_1              , Int8x16, Uint32x4)
LAYOUT_TYPE_WMS_REG2  ( Int8x16_1Uint16x8_1              , Int8x16, Uint16x8)
LAYOUT_TYPE_WMS_REG2  ( Int8x16_1Uint8x16_1              , Int8x16, Uint8x16)
LAYOUT_TYPE_WMS_REG3  ( Int1Int8x16_1Int1                , Int, Int8x16, Int)
LAYOUT_TYPE_WMS_REG4  ( Int8x16_2Int2                    , Int8x16, Int8x16, Int, Int)
// Uint32x4
LAYOUT_TYPE_WMS_REG5  ( Uint32x4_1Int4                   , Uint32x4, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Reg1Uint32x4_1                   , Reg, Uint32x4)
LAYOUT_TYPE_WMS_REG2  ( Uint32x4_2                       , Uint32x4, Uint32x4)
LAYOUT_TYPE_WMS_REG3  ( Int1Uint32x4_1Int1               , Int, Uint32x4, Int)
LAYOUT_TYPE_WMS_REG6  ( Uint32x4_2Int4                   , Uint32x4, Uint32x4, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG7  ( Uint32x4_3Int4                   , Uint32x4, Uint32x4, Uint32x4, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Uint32x4_1Int1                   , Uint32x4, Int)
LAYOUT_TYPE_WMS_REG4  ( Uint32x4_2Int2                   , Uint32x4, Uint32x4, Int, Int)
LAYOUT_TYPE_WMS_REG3  ( Uint32x4_3                       , Uint32x4, Uint32x4, Uint32x4)
LAYOUT_TYPE_WMS_REG3  ( Bool32x4_1Uint32x4_2             , Bool32x4, Uint32x4, Uint32x4)
LAYOUT_TYPE_WMS_REG4  ( Uint32x4_1Bool32x4_1Uint32x4_2   , Uint32x4, Bool32x4, Uint32x4, Uint32x4)
LAYOUT_TYPE_WMS_REG3  ( Uint32x4_2Int1                   , Uint32x4, Uint32x4, Int)
LAYOUT_TYPE_WMS_REG2  ( Uint32x4_1Float32x4_1            , Uint32x4, Float32x4)
LAYOUT_TYPE_WMS_REG2  ( Uint32x4_1Int32x4_1              , Uint32x4, Int32x4)
LAYOUT_TYPE_WMS_REG2  ( Uint32x4_1Int16x8_1              , Uint32x4, Int16x8)
LAYOUT_TYPE_WMS_REG2  ( Uint32x4_1Int8x16_1              , Uint32x4, Int8x16)
LAYOUT_TYPE_WMS_REG2  ( Uint32x4_1Uint16x8_1             , Uint32x4, Uint16x8)
LAYOUT_TYPE_WMS_REG2  ( Uint32x4_1Uint8x16_1             , Uint32x4, Uint8x16)

// Uint16x8
LAYOUT_TYPE_WMS_REG9  ( Uint16x8_1Int8                   , Uint16x8, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Reg1Uint16x8_1                   , Reg, Uint16x8)
LAYOUT_TYPE_WMS_REG2  ( Uint16x8_2                       , Uint16x8, Uint16x8)
LAYOUT_TYPE_WMS_REG3  ( Int1Uint16x8_1Int1               , Int, Uint16x8, Int)
LAYOUT_TYPE_WMS_REG10 ( Uint16x8_2Int8                   , Uint16x8, Uint16x8, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG11 ( Uint16x8_3Int8                   , Uint16x8, Uint16x8, Uint16x8, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Uint16x8_1Int1                   , Uint16x8, Int)
LAYOUT_TYPE_WMS_REG4  ( Uint16x8_2Int2                   , Uint16x8, Uint16x8, Int, Int)
LAYOUT_TYPE_WMS_REG3  ( Uint16x8_3                       , Uint16x8, Uint16x8, Uint16x8)
LAYOUT_TYPE_WMS_REG3  ( Bool16x8_1Uint16x8_2             , Bool16x8, Uint16x8, Uint16x8)
LAYOUT_TYPE_WMS_REG4  ( Uint16x8_1Bool16x8_1Uint16x8_2   , Uint16x8, Bool16x8, Uint16x8, Uint16x8)
LAYOUT_TYPE_WMS_REG3  ( Uint16x8_2Int1                   , Uint16x8, Uint16x8, Int)
LAYOUT_TYPE_WMS_REG2  ( Uint16x8_1Float32x4_1            , Uint16x8, Float32x4)
LAYOUT_TYPE_WMS_REG2  ( Uint16x8_1Int32x4_1              , Uint16x8, Int32x4)
LAYOUT_TYPE_WMS_REG2  ( Uint16x8_1Int16x8_1              , Uint16x8, Int16x8)
LAYOUT_TYPE_WMS_REG2  ( Uint16x8_1Int8x16_1              , Uint16x8, Int8x16)
LAYOUT_TYPE_WMS_REG2  ( Uint16x8_1Uint32x4_1             , Uint16x8, Uint32x4)
LAYOUT_TYPE_WMS_REG2  ( Uint16x8_1Uint8x16_1             , Uint16x8, Uint8x16)

// Uint8x16
LAYOUT_TYPE_WMS_REG17 ( Uint8x16_1Int16                  , Uint8x16, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Reg1Uint8x16_1                   , Reg, Uint8x16)
LAYOUT_TYPE_WMS_REG2  ( Uint8x16_2                       , Uint8x16, Uint8x16)
LAYOUT_TYPE_WMS_REG3  ( Int1Uint8x16_1Int1               , Int, Uint8x16, Int)
LAYOUT_TYPE_WMS_REG18 ( Uint8x16_2Int16                  , Uint8x16, Uint8x16, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG19 ( Uint8x16_3Int16                  , Uint8x16, Uint8x16, Uint8x16, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Uint8x16_1Int1                   , Uint8x16, Int)
LAYOUT_TYPE_WMS_REG4  ( Uint8x16_2Int2                   , Uint8x16, Uint8x16, Int, Int)
LAYOUT_TYPE_WMS_REG3  ( Uint8x16_3                       , Uint8x16, Uint8x16, Uint8x16)
LAYOUT_TYPE_WMS_REG3  ( Bool8x16_1Uint8x16_2             , Bool8x16, Uint8x16, Uint8x16)
LAYOUT_TYPE_WMS_REG4  ( Uint8x16_1Bool8x16_1Uint8x16_2   , Uint8x16, Bool8x16, Uint8x16, Uint8x16)
LAYOUT_TYPE_WMS_REG3  ( Uint8x16_2Int1                   , Uint8x16, Uint8x16, Int)
LAYOUT_TYPE_WMS_REG2  ( Uint8x16_1Float32x4_1            , Uint8x16, Float32x4)
LAYOUT_TYPE_WMS_REG2  ( Uint8x16_1Int32x4_1              , Uint8x16, Int32x4)
LAYOUT_TYPE_WMS_REG2  ( Uint8x16_1Int16x8_1              , Uint8x16, Int16x8)
LAYOUT_TYPE_WMS_REG2  ( Uint8x16_1Int8x16_1              , Uint8x16, Int8x16)
LAYOUT_TYPE_WMS_REG2  ( Uint8x16_1Uint32x4_1             , Uint8x16, Uint32x4)
LAYOUT_TYPE_WMS_REG2  ( Uint8x16_1Uint16x8_1             , Uint8x16, Uint16x8)

// Bool32x4
LAYOUT_TYPE_WMS_REG2  ( Bool32x4_1Int1                   , Bool32x4, Int)
LAYOUT_TYPE_WMS_REG5  ( Bool32x4_1Int4                   , Bool32x4, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG3  ( Int1Bool32x4_1Int1               , Int, Bool32x4, Int)
LAYOUT_TYPE_WMS_REG4  ( Bool32x4_2Int2                   , Bool32x4, Bool32x4, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Int1Bool32x4_1                   , Int, Bool32x4)
LAYOUT_TYPE_WMS_REG2  ( Bool32x4_2                       , Bool32x4, Bool32x4)
LAYOUT_TYPE_WMS_REG3  ( Bool32x4_3                       , Bool32x4, Bool32x4, Bool32x4)
LAYOUT_TYPE_WMS_REG2  ( Reg1Bool32x4_1                   , Reg, Bool32x4)

// Bool16x8
LAYOUT_TYPE_WMS_REG2  ( Bool16x8_1Int1                   , Bool16x8, Int)
LAYOUT_TYPE_WMS_REG9  ( Bool16x8_1Int8                   , Bool16x8, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG3  ( Int1Bool16x8_1Int1               , Int, Bool16x8, Int)
LAYOUT_TYPE_WMS_REG4  ( Bool16x8_2Int2                   , Bool16x8, Bool16x8, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Int1Bool16x8_1                   , Int, Bool16x8)
LAYOUT_TYPE_WMS_REG2  ( Bool16x8_2                       , Bool16x8, Bool16x8)
LAYOUT_TYPE_WMS_REG3  ( Bool16x8_3                       , Bool16x8, Bool16x8, Bool16x8)
LAYOUT_TYPE_WMS_REG2  ( Reg1Bool16x8_1                   , Reg, Bool16x8)

//Bool8x16
LAYOUT_TYPE_WMS_REG2  ( Bool8x16_1Int1                   , Bool8x16, Int)
LAYOUT_TYPE_WMS_REG17 ( Bool8x16_1Int16                  , Bool8x16, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Int1Bool8x16_1                   , Int, Bool8x16)
LAYOUT_TYPE_WMS_REG3  ( Int1Bool8x16_1Int1               , Int, Bool8x16, Int)
LAYOUT_TYPE_WMS_REG4  ( Bool8x16_2Int2                   , Bool8x16, Bool8x16, Int, Int)
LAYOUT_TYPE_WMS_REG2  ( Bool8x16_2                       , Bool8x16, Bool8x16)
LAYOUT_TYPE_WMS_REG3  ( Bool8x16_3                       , Bool8x16, Bool8x16, Bool8x16)
LAYOUT_TYPE_WMS_REG2  ( Reg1Bool8x16_1                   , Reg, Bool8x16)


LAYOUT_TYPE_WMS       ( AsmSimdTypedArr                  )
#endif

#undef LAYOUT_TYPE_DUP
#undef LAYOUT_TYPE_WMS_DUP
#undef LAYOUT_TYPE
#undef LAYOUT_TYPE_WMS
#undef LAYOUT_TYPE_WMS_REG2
#undef LAYOUT_TYPE_WMS_REG3
#undef LAYOUT_TYPE_WMS_REG4
#undef LAYOUT_TYPE_WMS_REG5
#undef LAYOUT_TYPE_WMS_REG6
#undef LAYOUT_TYPE_WMS_REG7
#undef LAYOUT_TYPE_WMS_REG9
#undef LAYOUT_TYPE_WMS_REG10
#undef LAYOUT_TYPE_WMS_REG11
#undef LAYOUT_TYPE_WMS_REG17
#undef LAYOUT_TYPE_WMS_REG18
#undef LAYOUT_TYPE_WMS_REG19
#undef EXCLUDE_DUP_LAYOUT
#undef LAYOUT_TYPE_WMS_FE
#undef EXCLUDE_FRONTEND_LAYOUT

#undef LAYOUT_PREFIX_Int
#undef LAYOUT_PREFIX_IntConst
