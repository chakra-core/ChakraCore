//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// NOTE: This file is intended to be "#include" multiple times. The call site must define the macros
// "LAYOUT_TYPE", etc., to be executed for each entry.
//

#ifndef TEMP_DISABLE_ASMJS
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
#define LAYOUT_PREFIX_Float() F
#define LAYOUT_PREFIX_Double() D
#define LAYOUT_PREFIX_IntConst() C
#define LAYOUT_PREFIX_FloatConst() C
#define LAYOUT_PREFIX_DoubleConst() C

// These layout are already defined in LayoutTypes.h
// We redeclare them here to keep the same layout and use them
// in other contexts.
LAYOUT_TYPE_WMS_DUP ( ElementSlot      )
LAYOUT_TYPE_DUP     ( StartCall        )
LAYOUT_TYPE_DUP     ( Empty            )

LAYOUT_TYPE_WMS     ( AsmTypedArr      )
LAYOUT_TYPE_WMS     ( AsmCall          )
LAYOUT_TYPE         ( AsmBr            )
LAYOUT_TYPE_WMS     ( AsmReg1          ) // Generic layout with 1 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg2          ) // Generic layout with 2 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg3          ) // Generic layout with 3 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg4          ) // Generic layout with 4 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg5          ) // Generic layout with 5 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg6          ) // Generic layout with 6 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg7          ) // Generic layout with 7 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg9          ) // Generic layout with 9 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg10         ) // Generic layout with 10 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg11         ) // Generic layout with 11 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg17         ) // Generic layout with 17 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg18         ) // Generic layout with 18 RegSlot
LAYOUT_TYPE_WMS_FE  ( AsmReg19         ) // Generic layout with 19 RegSlot
LAYOUT_TYPE_WMS_REG2( Int1Double1      , Int, Double) // 1 int register and 1 double register
LAYOUT_TYPE_WMS_REG2( Int1Float1       , Int, Float) // 1 int register and 1 float register
LAYOUT_TYPE_WMS_REG2( Double1Const1    , Double, DoubleConst) // 1 double register and 1 const double value
LAYOUT_TYPE_WMS_REG2( Double1Int1      , Double, Int) // 1 double register and 1 int register
LAYOUT_TYPE_WMS_REG2( Double1Float1    , Double, Float) // 1 double register and 1 float register
LAYOUT_TYPE_WMS_REG2( Double1Reg1      , Double, Reg) // 1 double register and 1 var register
LAYOUT_TYPE_WMS_REG2( Float1Reg1       , Float, Reg) // 1 double register and 1 var register
LAYOUT_TYPE_WMS_REG2( Int1Reg1         , Int, Reg) // 1 int register and 1 var register
LAYOUT_TYPE_WMS_REG2( Reg1Double1      , Reg, Double) // 1 var register and 1 double register
LAYOUT_TYPE_WMS_REG2( Reg1Float1       , Reg, Float) // 1 var register and 1 Float register
LAYOUT_TYPE_WMS_REG2( Reg1Int1         , Reg, Int) // 1 var register and 1 int register
LAYOUT_TYPE_WMS_REG2( Int1Const1       , Int, IntConst) // 1 int register and 1 const int value
LAYOUT_TYPE_WMS_REG3( Int1Double2      , Int, Double, Double) // 1 int register and 2 double register ( double comparisons )
LAYOUT_TYPE_WMS_REG3( Int1Float2       , Int, Float, Float) // 1 int register and 2 float register ( float comparisons )
LAYOUT_TYPE_WMS_REG2( Int2             , Int, Int) // 2 int register
LAYOUT_TYPE_WMS_REG3( Int3             , Int, Int, Int) // 3 int register
LAYOUT_TYPE_WMS_REG2( Double2          , Double, Double) // 2 double register
LAYOUT_TYPE_WMS_REG2( Float2           , Float, Float) // 2 float register
LAYOUT_TYPE_WMS_REG3( Float3           , Float, Float, Float) // 3 float register
LAYOUT_TYPE_WMS_REG2( Float1Const1     , Float, FloatConst) // 1 float register and 1 const float value
LAYOUT_TYPE_WMS_REG2( Float1Double1    , Float, Double) // 1 float register and 1 double register
LAYOUT_TYPE_WMS_REG2( Float1Int1       , Float, Int) // 2 double register
LAYOUT_TYPE_WMS_REG3( Double3          , Double, Double, Double) // 3 double register
LAYOUT_TYPE_WMS     ( BrInt1           ) // Conditional branching with 1 int
LAYOUT_TYPE_WMS     ( BrInt2           ) // Conditional branching with 2 int
LAYOUT_TYPE_WMS     ( BrInt1Const1     ) // Conditional branching with 1 int and 1 const
LAYOUT_TYPE_WMS     ( AsmUnsigned1     ) // 1 unsigned int register

// Float32x4
LAYOUT_TYPE_WMS     ( Float32x4_2                       )
LAYOUT_TYPE_WMS     ( Float32x4_3                       )
LAYOUT_TYPE_WMS     ( Float32x4_4                       )
LAYOUT_TYPE_WMS     ( Bool32x4_1Float32x4_2             )
LAYOUT_TYPE_WMS     ( Float32x4_1Bool32x4_1Float32x4_2  )
LAYOUT_TYPE_WMS     ( Float32x4_1Float4                 )
LAYOUT_TYPE_WMS     ( Float32x4_2Int4                   )
LAYOUT_TYPE_WMS     ( Float32x4_3Int4                   )
LAYOUT_TYPE_WMS     ( Float32x4_1Float1                 )
LAYOUT_TYPE_WMS     ( Float32x4_2Float1                 )
//LAYOUT_TYPE_WMS     ( Float32x4_1Float64x2_1            )
LAYOUT_TYPE_WMS     ( Float32x4_1Int32x4_1              )
LAYOUT_TYPE_WMS     ( Float32x4_1Uint32x4_1             )
LAYOUT_TYPE_WMS     ( Float32x4_1Int16x8_1              )
LAYOUT_TYPE_WMS     ( Float32x4_1Uint16x8_1             )
LAYOUT_TYPE_WMS     ( Float32x4_1Int8x16_1              )
LAYOUT_TYPE_WMS     ( Float32x4_1Uint8x16_1             )
LAYOUT_TYPE_WMS     ( Reg1Float32x4_1                   )
LAYOUT_TYPE_WMS     ( Float1Float32x4_1Int1             )
LAYOUT_TYPE_WMS     ( Float32x4_2Int1Float1             )
// Int32x4
LAYOUT_TYPE_WMS     ( Int32x4_2                         )
LAYOUT_TYPE_WMS     ( Int32x4_3                         )
LAYOUT_TYPE_WMS     ( Bool32x4_1Int32x4_2               )
LAYOUT_TYPE_WMS     ( Int32x4_1Bool32x4_1Int32x4_2      )
LAYOUT_TYPE_WMS     ( Int32x4_1Int4                     )
LAYOUT_TYPE_WMS     ( Int32x4_2Int4                     )
LAYOUT_TYPE_WMS     ( Int32x4_3Int4                     )
LAYOUT_TYPE_WMS     ( Int32x4_1Int1                     )
LAYOUT_TYPE_WMS     ( Int32x4_2Int1                     )
LAYOUT_TYPE_WMS     ( Reg1Int32x4_1                     )
LAYOUT_TYPE_WMS     ( Int32x4_1Float32x4_1              )
//LAYOUT_TYPE_WMS     ( Int32x4_1Float64x2_1              )
LAYOUT_TYPE_WMS     ( Int32x4_1Uint32x4_1               )
LAYOUT_TYPE_WMS     ( Int32x4_1Int16x8_1                )
LAYOUT_TYPE_WMS     ( Int32x4_1Uint16x8_1               )
LAYOUT_TYPE_WMS     ( Int32x4_1Int8x16_1                )
LAYOUT_TYPE_WMS     ( Int32x4_1Uint8x16_1               )
LAYOUT_TYPE_WMS     ( Int1Int32x4_1Int1                 )
LAYOUT_TYPE_WMS     ( Int32x4_2Int2                     )

// Float64x2
// Disabled for now
#if 0 
LAYOUT_TYPE_WMS     ( Float64x2_2 )
LAYOUT_TYPE_WMS     ( Float64x2_3 )
LAYOUT_TYPE_WMS     ( Float64x2_4 )
LAYOUT_TYPE_WMS     ( Float64x2_1Double2 )
LAYOUT_TYPE_WMS     ( Float64x2_1Double1 )
LAYOUT_TYPE_WMS     ( Float64x2_2Double1 )
LAYOUT_TYPE_WMS     ( Float64x2_2Int2 )
LAYOUT_TYPE_WMS     ( Float64x2_3Int2 )
LAYOUT_TYPE_WMS     ( Float64x2_1Float32x4_1 )
LAYOUT_TYPE_WMS     ( Float64x2_1Int32x4_1 )
LAYOUT_TYPE_WMS     ( Float64x2_1Int32x4_1Float64x2_2 )
LAYOUT_TYPE_WMS     ( Reg1Float64x2_1 )

#endif //0
// Int16x8
LAYOUT_TYPE_WMS     ( Int16x8_1Int8                 )
LAYOUT_TYPE_WMS     ( Reg1Int16x8_1                 )
LAYOUT_TYPE_WMS     ( Int16x8_2                     )
LAYOUT_TYPE_WMS     ( Int1Int16x8_1Int1             )
LAYOUT_TYPE_WMS     ( Int16x8_2Int8                 )
LAYOUT_TYPE_WMS     ( Int16x8_3Int8                 )
LAYOUT_TYPE_WMS     ( Int16x8_1Int1                 )
LAYOUT_TYPE_WMS     ( Int16x8_2Int2                 )
LAYOUT_TYPE_WMS     ( Int16x8_3                     )
LAYOUT_TYPE_WMS     ( Bool16x8_1Int16x8_2           )
LAYOUT_TYPE_WMS     ( Int16x8_1Bool16x8_1Int16x8_2  )
LAYOUT_TYPE_WMS     ( Int16x8_2Int1                 )
LAYOUT_TYPE_WMS     ( Int16x8_1Float32x4_1          )
LAYOUT_TYPE_WMS     ( Int16x8_1Int32x4_1            )
LAYOUT_TYPE_WMS     ( Int16x8_1Int8x16_1            )
LAYOUT_TYPE_WMS     ( Int16x8_1Uint32x4_1           )
LAYOUT_TYPE_WMS     ( Int16x8_1Uint16x8_1           )
LAYOUT_TYPE_WMS     ( Int16x8_1Uint8x16_1           )
// Int8x16
LAYOUT_TYPE_WMS     ( Int8x16_2                     )
LAYOUT_TYPE_WMS     ( Int8x16_3                     )
LAYOUT_TYPE_WMS     ( Int8x16_1Int16                )
LAYOUT_TYPE_WMS     ( Int8x16_2Int16                )
LAYOUT_TYPE_WMS     ( Int8x16_3Int16                )
LAYOUT_TYPE_WMS     ( Int8x16_1Int1                 )
LAYOUT_TYPE_WMS     ( Int8x16_2Int1                 )
LAYOUT_TYPE_WMS     ( Reg1Int8x16_1                 )
LAYOUT_TYPE_WMS     ( Bool8x16_1Int8x16_2           )
LAYOUT_TYPE_WMS     ( Int8x16_1Bool8x16_1Int8x16_2  )
LAYOUT_TYPE_WMS     ( Int8x16_1Float32x4_1          )
LAYOUT_TYPE_WMS     ( Int8x16_1Int32x4_1            )
LAYOUT_TYPE_WMS     ( Int8x16_1Int16x8_1            )
LAYOUT_TYPE_WMS     ( Int8x16_1Uint32x4_1           )
LAYOUT_TYPE_WMS     ( Int8x16_1Uint16x8_1           )
LAYOUT_TYPE_WMS     ( Int8x16_1Uint8x16_1           )
LAYOUT_TYPE_WMS     ( Int1Int8x16_1Int1             )
LAYOUT_TYPE_WMS     ( Int8x16_2Int2                 )
// Uint32x4
LAYOUT_TYPE_WMS     ( Uint32x4_1Int4                )
LAYOUT_TYPE_WMS     ( Reg1Uint32x4_1                )
LAYOUT_TYPE_WMS     ( Uint32x4_2                    )
LAYOUT_TYPE_WMS     ( Int1Uint32x4_1Int1            )
LAYOUT_TYPE_WMS     ( Uint32x4_2Int4                )
LAYOUT_TYPE_WMS     ( Uint32x4_3Int4                )
LAYOUT_TYPE_WMS     ( Uint32x4_1Int1                )
LAYOUT_TYPE_WMS     ( Uint32x4_2Int2                )
LAYOUT_TYPE_WMS     ( Uint32x4_3                    )
LAYOUT_TYPE_WMS     ( Bool32x4_1Uint32x4_2          )
LAYOUT_TYPE_WMS     ( Uint32x4_1Bool32x4_1Uint32x4_2)
LAYOUT_TYPE_WMS     ( Uint32x4_2Int1                )
LAYOUT_TYPE_WMS     ( Uint32x4_1Float32x4_1         )
LAYOUT_TYPE_WMS     ( Uint32x4_1Int32x4_1           )
LAYOUT_TYPE_WMS     ( Uint32x4_1Int16x8_1           )
LAYOUT_TYPE_WMS     ( Uint32x4_1Int8x16_1           )
LAYOUT_TYPE_WMS     ( Uint32x4_1Uint16x8_1          )
LAYOUT_TYPE_WMS     ( Uint32x4_1Uint8x16_1          )

// Uint16x8
LAYOUT_TYPE_WMS     ( Uint16x8_1Int8                )
LAYOUT_TYPE_WMS     ( Reg1Uint16x8_1                )
LAYOUT_TYPE_WMS     ( Uint16x8_2                    )
LAYOUT_TYPE_WMS     ( Int1Uint16x8_1Int1            )
LAYOUT_TYPE_WMS     ( Uint16x8_2Int8                )
LAYOUT_TYPE_WMS     ( Uint16x8_3Int8                )
LAYOUT_TYPE_WMS     ( Uint16x8_1Int1                )
LAYOUT_TYPE_WMS     ( Uint16x8_2Int2                )
LAYOUT_TYPE_WMS     ( Uint16x8_3                    )
LAYOUT_TYPE_WMS     ( Bool16x8_1Uint16x8_2          )
LAYOUT_TYPE_WMS     ( Uint16x8_1Bool16x8_1Uint16x8_2)
LAYOUT_TYPE_WMS     ( Uint16x8_2Int1                )
LAYOUT_TYPE_WMS     ( Uint16x8_1Float32x4_1         )
LAYOUT_TYPE_WMS     ( Uint16x8_1Int32x4_1           )
LAYOUT_TYPE_WMS     ( Uint16x8_1Int16x8_1           )
LAYOUT_TYPE_WMS     ( Uint16x8_1Int8x16_1           )
LAYOUT_TYPE_WMS     ( Uint16x8_1Uint32x4_1          )
LAYOUT_TYPE_WMS     ( Uint16x8_1Uint8x16_1          )

// Uint8x16
LAYOUT_TYPE_WMS     ( Uint8x16_1Int16               )
LAYOUT_TYPE_WMS     ( Reg1Uint8x16_1                )
LAYOUT_TYPE_WMS     ( Uint8x16_2                    )
LAYOUT_TYPE_WMS     ( Int1Uint8x16_1Int1            )
LAYOUT_TYPE_WMS     ( Uint8x16_2Int16               )
LAYOUT_TYPE_WMS     ( Uint8x16_3Int16               )
LAYOUT_TYPE_WMS     ( Uint8x16_1Int1                )
LAYOUT_TYPE_WMS     ( Uint8x16_2Int2                )
LAYOUT_TYPE_WMS     ( Uint8x16_3                    )
LAYOUT_TYPE_WMS     ( Bool8x16_1Uint8x16_2          )
LAYOUT_TYPE_WMS     ( Uint8x16_1Bool8x16_1Uint8x16_2)
LAYOUT_TYPE_WMS     ( Uint8x16_2Int1                )
LAYOUT_TYPE_WMS     ( Uint8x16_1Float32x4_1         )
LAYOUT_TYPE_WMS     ( Uint8x16_1Int32x4_1           )
LAYOUT_TYPE_WMS     ( Uint8x16_1Int16x8_1           )
LAYOUT_TYPE_WMS     ( Uint8x16_1Int8x16_1           )
LAYOUT_TYPE_WMS     ( Uint8x16_1Uint32x4_1          )
LAYOUT_TYPE_WMS     ( Uint8x16_1Uint16x8_1          )

//Bool32x4
LAYOUT_TYPE_WMS     ( Bool32x4_1Int1                )
LAYOUT_TYPE_WMS     ( Bool32x4_1Int4                )
LAYOUT_TYPE_WMS     ( Int1Bool32x4_1Int1            )
LAYOUT_TYPE_WMS     ( Bool32x4_2Int2                )
LAYOUT_TYPE_WMS     ( Int1Bool32x4_1                )
LAYOUT_TYPE_WMS     ( Bool32x4_2                    )
LAYOUT_TYPE_WMS     ( Bool32x4_3                    )
LAYOUT_TYPE_WMS     ( Reg1Bool32x4_1                )
//Bool16x8
LAYOUT_TYPE_WMS     ( Bool16x8_1Int1                )
LAYOUT_TYPE_WMS     ( Bool16x8_1Int8                )
LAYOUT_TYPE_WMS     ( Int1Bool16x8_1Int1            )
LAYOUT_TYPE_WMS     ( Bool16x8_2Int2                )
LAYOUT_TYPE_WMS     ( Int1Bool16x8_1                )
LAYOUT_TYPE_WMS     ( Bool16x8_2                    )
LAYOUT_TYPE_WMS     ( Bool16x8_3                    )
LAYOUT_TYPE_WMS     ( Reg1Bool16x8_1                )
//Bool8x16
LAYOUT_TYPE_WMS     ( Bool8x16_1Int1                )
LAYOUT_TYPE_WMS     ( Bool8x16_1Int16               )
LAYOUT_TYPE_WMS     ( Int1Bool8x16_1                )
LAYOUT_TYPE_WMS     ( Int1Bool8x16_1Int1            )
LAYOUT_TYPE_WMS     ( Bool8x16_2Int2                )
LAYOUT_TYPE_WMS     ( Bool8x16_2                    )
LAYOUT_TYPE_WMS     ( Bool8x16_3                    )
LAYOUT_TYPE_WMS     ( Reg1Bool8x16_1                )

LAYOUT_TYPE_WMS     ( AsmSimdTypedArr               )

#undef LAYOUT_TYPE_DUP
#undef LAYOUT_TYPE_WMS_DUP
#undef LAYOUT_TYPE
#undef LAYOUT_TYPE_WMS
#undef LAYOUT_TYPE_WMS_REG2
#undef LAYOUT_TYPE_WMS_REG3
#undef EXCLUDE_DUP_LAYOUT
#undef LAYOUT_TYPE_WMS_FE
#undef EXCLUDE_FRONTEND_LAYOUT

#undef LAYOUT_PREFIX_Int
#undef LAYOUT_PREFIX_IntConst
#endif
