//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifdef ASMJS_PLAT
// See  Lib\Runtime\Language\InterpreterProcessOpCodeAsmJs.h  for Handler Process
//         (   HandlerProcess , OpCodeAsmJs ,        HandlerFunction       , LayoutAsmJs , Type  )
//         (        |         ,     |       ,              |               ,      |      ,   |   )
//         (        |         ,     |       ,              |               ,      |      ,   |   )
//         (        V         ,     V       ,              V               ,      V      ,   V   )

  DEF2    ( NOPASMJS         , Nop          , Empty                                              )
EXDEF2    (NOPASMJS          , InvalidOpCode, Empty                                              )
  DEF2    ( NOPASMJS         , Label        , Empty                                              )
  DEF2    ( BR_ASM           , AsmBr        , OP_Br                                              )

  DEF2_WMS( FALLTHROUGH_ASM  , LdSlotArr    , /* Common case with LdSlot */                      )
  DEF3_WMS( GET_ELEM_SLOT_ASM, LdSlot       , OP_LdAsmJsSlot               , ElementSlot         )
  DEF2_WMS( FUNCtoA1Mem      , LdUndef      , JavascriptOperators::OP_LdUndef                    )

// Function Calls
  DEF2    ( FALLTHROUGH_ASM  , I_StartCall, /* Common case with StartCall */                     )
  DEF3    ( CUSTOM_ASMJS     , StartCall    , OP_AsmStartCall              , StartCall           )

  DEF3_WMS( CUSTOM_ASMJS     , I_Call       , OP_I_AsmCall                 , AsmCall             )
  DEF3_WMS( CUSTOM_ASMJS     , Call         , OP_AsmCall                   , AsmCall             )
  DEF2_WMS( D1toR1Out        , I_ArgOut_Db  , OP_I_SetOutAsmDb                                   ) // set double as internal outparam
  DEF2_WMS( D1toR1Out        , ArgOut_Db    , OP_SetOutAsmDb                                     ) // convert double to var and set it as outparam
  DEF2_WMS( I1toR1Out        , I_ArgOut_Int , OP_I_SetOutAsmInt                                  ) // set int as internal outparam
  DEF2_WMS( I1toR1Out        , ArgOut_Int   , OP_SetOutAsmInt                                    ) // convert int to var and set it as outparam
  DEF2_WMS( F1toR1Out        , I_ArgOut_Flt , OP_I_SetOutAsmFlt                                  ) // set float as internal outparam
  DEF2_WMS( F1toR1Out        , ArgOut_Flt   , OP_SetOutAsmFlt                                    ) // convert float to var and set as outparam
  DEF2_WMS( L1toR1Out        , I_ArgOut_Long, OP_I_SetOutAsmLong                                 ) // set int64 as internal outparam
  DEF2_WMS( D1toD1Mem        , I_Conv_VTD   , (double)                                           )
  DEF2_WMS( R1toD1Mem        , Conv_VTD     , JavascriptConversion::ToNumber                     ) // convert var to double
  DEF2_WMS( F1toF1Mem        , I_Conv_VTF   , (float)                                            )
  DEF2_WMS( R1toF1Mem        , Conv_VTF     , JavascriptConversion::ToNumber                     ) // convert var to float
  DEF2_WMS( I1toI1Mem        , I_Conv_VTI   , (int)                                              )
  DEF2_WMS( R1toI1Mem        , Conv_VTI     , JavascriptMath::ToInt32                            ) // convert var to int
  DEF3_WMS( CUSTOM_ASMJS     , ArgOut_Long  , OP_InvalidWasmTypeConversion<true>  , Reg1Long1    ) // convert int64 to Var
  DEF3_WMS( CUSTOM_ASMJS     , Conv_VTL     , OP_InvalidWasmTypeConversion<false> , Long1Reg1    ) // convert var to int64

  DEF3_WMS( CUSTOM_ASMJS     , LdArr_Func   , OP_LdArrFunc                 , ElementSlot         )
  DEF3_WMS( CUSTOM_ASMJS     , LdArr_WasmFunc,OP_LdArrWasmFunc             , ElementSlot         )
  DEF3_WMS( CUSTOM_ASMJS     , CheckSignature,OP_CheckSignature            , Reg1IntConst1       )
  DEF4_WMS( TEMPLATE_ASMJS   , LdSlot_Db    , OP_LdSlotPrimitive           , ElementSlot, double )
  DEF4_WMS( TEMPLATE_ASMJS   , LdSlot_Int   , OP_LdSlotPrimitive           , ElementSlot, int    )
  DEF4_WMS( TEMPLATE_ASMJS   , LdSlot_Long  , OP_LdSlotPrimitive           , ElementSlot, int64  )
  DEF4_WMS( TEMPLATE_ASMJS   , LdSlot_Flt   , OP_LdSlotPrimitive           , ElementSlot, float  )
  DEF4_WMS( TEMPLATE_ASMJS   , StSlot_Db    , OP_StSlotPrimitive           , ElementSlot, double )
  DEF4_WMS( TEMPLATE_ASMJS   , StSlot_Int   , OP_StSlotPrimitive           , ElementSlot, int    )
  DEF4_WMS( TEMPLATE_ASMJS   , StSlot_Long  , OP_StSlotPrimitive           , ElementSlot, int64  )
  DEF4_WMS( TEMPLATE_ASMJS   , StSlot_Flt   , OP_StSlotPrimitive           , ElementSlot, float  )
  DEF3_WMS( CUSTOM_ASMJS     , LdArr        , OP_LdArrGeneric              , AsmTypedArr         )
  DEF3_WMS( CUSTOM_ASMJS     , LdArrWasm    , OP_LdArrWasm                 , WasmMemAccess       )
  DEF3_WMS( CUSTOM_ASMJS     , StArrWasm    , OP_StArrWasm                 , WasmMemAccess       )
  DEF3_WMS( CUSTOM_ASMJS     , LdArrConst   , OP_LdArrConstIndex           , AsmTypedArr         )
  DEF3_WMS( CUSTOM_ASMJS     , StArr        , OP_StArrGeneric              , AsmTypedArr         )
  DEF3_WMS( CUSTOM_ASMJS     , StArrConst   , OP_StArrConstIndex           , AsmTypedArr         )

  DEF2_WMS( C1toI1           , Ld_IntConst  , None                                               )
  DEF2_WMS( C1toL1           , Ld_LongConst , None                                               )
  DEF2_WMS( C1toF1           , Ld_FltConst  , None                                               )
  DEF2_WMS( C1toD1           , Ld_DbConst   , None                                               )
  DEF2_WMS( BR_ASM_MemStack  , BrTrue_Int   , None                                               ) // Jumps to location if int reg is true
  DEF2_WMS( BR_ASM_MemStackF , BrFalse_Int   , None                                              ) // Jumps to location if int reg is false
  DEF2_WMS( BR_ASM_Mem       , BrEq_Int     , AsmJsMath::CmpEq<int>                              ) // Jumps to location if both int reg are equal
  DEF2_WMS( I1toF1Mem        , Reinterpret_ITF, NumberUtilities::ReinterpretBits                 ) // reinterpret bits of int to float
  DEF2_WMS( F1toI1Mem        , Reinterpret_FTI, NumberUtilities::ToSpecial                       ) // reinterpret bits of float to int
  DEF2_WMS( L1toD1Mem        , Reinterpret_LTD, NumberUtilities::ReinterpretBits                 ) // reinterpret bits of long to double
  DEF2_WMS( D1toL1Mem        , Reinterpret_DTL, NumberUtilities::ToSpecial                       ) // reinterpret bits of double to long
  DEF2_WMS( D1toI1Mem        , Conv_DTI     , JavascriptConversion::ToInt32                      ) // convert double to int
  DEF2_WMS( D1toI1Mem        , Conv_DTU     , JavascriptConversion::ToUInt32                     ) // convert double to unsigned int
  DEF2_WMS( F1toI1Mem        , Conv_FTI     , JavascriptConversion::ToInt32                      ) // convert float to int
  DEF2_WMS( F1toI1Mem        , Conv_FTU     , JavascriptConversion::ToUInt32                     ) // convert float to unsigned int
  DEF2_WMS( I1toD1Mem        , Conv_ITD     , (double)                                           ) // convert int to double
  DEF2_WMS( U1toD1Mem        , Conv_UTD     , (double)                                           ) // convert unsigned int to double
  DEF2_WMS( U1toF1Mem        , Conv_UTF     , (float)                                            ) // convert unsigned int to float
  DEF2_WMS( F1toD1Mem        , Conv_FTD     , (double)                                           ) // convert unsigned float to double
  DEF2_WMS( I1toL1Mem        , Conv_ITL     , (int64)                                            ) // extend signed int to int64
  DEF2_WMS( U1toL1Mem        , Conv_UTL     , (int64)                                            ) // extend unsigned int to int64
  DEF2_WMS( L1toD1Mem        , Conv_ULTD    , JavascriptConversion::ULongToDouble                )
  DEF2_WMS( L1toD1Mem        , Conv_LTD     , JavascriptConversion::LongToDouble                 )
  DEF2_WMS( L1toF1Mem        , Conv_ULTF    , JavascriptConversion::ULongToFloat                 )
  DEF2_WMS( L1toF1Mem        , Conv_LTF     , JavascriptConversion::LongToFloat                  )
  DEF2_WMS( L1toI1Mem        , Conv_LTI     , (int)                                              ) // wrap int64 to int
  DEF2_WMS( I1toI1Mem        , Ld_Int       , (int)                                              )
  DEF2_WMS( L1toL1Mem        , Ld_Long      , (int64)                                            )
  DEF2_WMS( D1toD1Mem        , Ld_Db        , (double)                                           )
  DEF2_WMS( F1toF1Mem        , Ld_Flt       , (float)                                            )
  DEF2_WMS( D1toD1Mem        , Return_Db    , (double)                                           )
  DEF2_WMS( F1toF1Mem        , Return_Flt   , (float)                                            )
  DEF2_WMS( I1toI1Mem        , Return_Int   , (int)                                              )
  DEF2_WMS( L1toL1Mem        , Return_Long  , (int64)                                            )

  DEF2_WMS( I1toI1Mem        , BeginSwitch_Int, (int)                                            )
  DEF2    ( BR_ASM           , EndSwitch_Int, OP_Br                                              )
  DEF2_WMS( BR_ASM_Mem       , Case_Int     , AsmJsMath::CmpEq<int>                              )
  DEF2_WMS( BR_ASM_Const     , Case_IntConst, AsmJsMath::CmpEq<int>                              )

// Int32 Math
  DEF2_WMS( I1toI1Mem        , Neg_Int      , AsmJsMath::Neg<int>                                ) // int unary '-'
  DEF2_WMS( I1toI1Mem        , Not_Int      , AsmJsMath::Not                                     ) // int unary '~'
  DEF2_WMS( I1toI1Mem        , LogNot_Int   , AsmJsMath::LogNot                                  ) // int unary '!'
  DEF2_WMS( I1toI1Mem        , Conv_ITB     , AsmJsMath::ToBool                                  ) // convert an int to a bool (0|1)
  DEF2_WMS( I2toI1Mem        , Add_Int      , AsmJsMath::Add<int>                                )
  DEF2_WMS( I2toI1Mem        , Sub_Int      , AsmJsMath::Sub<int>                                )
  DEF2_WMS( I2toI1Mem        , Mul_Int      , AsmJsMath::Mul<int>                                )
  DEF2_WMS( I2toI1Mem        , Div_Int      , AsmJsMath::DivChecked<int>                         )
  DEF2_WMS( I2toI1Mem        , Rem_Int      , AsmJsMath::RemChecked<int>                         )
  DEF2_WMS( I2toI1Ctx        , Div_Trap_Int, (OP_DivOverflow<int, &AsmJsMath::DivUnsafe<int>>)  )
  DEF2_WMS( I2toI1Ctx        , Rem_Trap_Int, (OP_RemOverflow<int, &AsmJsMath::RemUnsafe<int>>)  )
  DEF2_WMS( I2toI1Mem        , And_Int      , AsmJsMath::And                                     )
  DEF2_WMS( I2toI1Mem        , Or_Int       , AsmJsMath::Or                                      )
  DEF2_WMS( I2toI1Mem        , Xor_Int      , AsmJsMath::Xor                                     )
  DEF2_WMS( I2toI1Mem        , Shl_Int      , AsmJsMath::Shl                                     )
  DEF2_WMS( I2toI1Mem        , Shr_Int      , AsmJsMath::Shr                                     )
  DEF2_WMS( I2toI1Mem        , Shr_UInt     , AsmJsMath::ShrU<uint>                              )
  DEF2_WMS( I2toI1Mem        , Rol_Int      , Wasm::WasmMath::Rol                                )
  DEF2_WMS( I2toI1Mem        , Ror_Int      , Wasm::WasmMath::Ror                                )
  DEF2_WMS( I1toI1Mem        , PopCnt_Int   , ::Math::PopCnt32                                   )
  DEF2_WMS( I1toI1Mem        , Abs_Int      , ::abs                                              )
  DEF2_WMS( I2toI1Mem        , Min_Int      , min                                                )
  DEF2_WMS( I2toI1Mem        , Max_Int      , max                                                )
  DEF2_WMS( I1toI1Mem        , Clz32_Int    , AsmJsMath::Clz32                                   )
  DEF2_WMS( I1toI1Mem        , Ctz_Int      , Wasm::WasmMath::Ctz                                )

// Unsigned Int32 Math
  DEF2_WMS( I2toI1Mem        , Div_UInt     , AsmJsMath::DivChecked<uint32>                            )
  DEF2_WMS( I2toI1Mem        , Rem_UInt     , AsmJsMath::RemChecked<uint32>                            )
  DEF2_WMS( I2toI1Ctx        , Div_Trap_UInt, (OP_UnsignedDivRemCheck<uint32, &AsmJsMath::DivUnsafe<uint32>>) )
  DEF2_WMS( I2toI1Ctx        , Rem_Trap_UInt, (OP_UnsignedDivRemCheck<uint32, &AsmJsMath::RemUnsafe<uint32>>) )

// Int64 Math
  DEF2_WMS( L2toL1Mem        , Add_Long     , AsmJsMath::Add<int64>)
  DEF2_WMS( L2toL1Mem        , Sub_Long     , AsmJsMath::Sub<int64>)
  DEF2_WMS( L2toL1Mem        , Mul_Long     , AsmJsMath::Mul<int64>)
  DEF2_WMS( L2toL1Ctx        , Div_Trap_Long, (OP_DivOverflow<int64,  &AsmJsMath::DivUnsafe<int64>>) )
  DEF2_WMS( L2toL1Ctx        , Rem_Trap_Long, (OP_RemOverflow<int64,  &AsmJsMath::RemUnsafe<int64>>) )
  DEF2_WMS( L2toL1Mem        , And_Long     , AsmJsMath::And<int64>)
  DEF2_WMS( L2toL1Mem        , Or_Long      , AsmJsMath::Or<int64>)
  DEF2_WMS( L2toL1Mem        , Xor_Long     , AsmJsMath::Xor<int64>)
  DEF2_WMS( L2toL1Mem        , Shl_Long     , Wasm::WasmMath::Shl<int64>)
  DEF2_WMS( L2toL1Mem        , Shr_Long     , Wasm::WasmMath::Shr<int64>)
  DEF2_WMS( L2toL1Mem        , Shr_ULong    , Wasm::WasmMath::ShrU<uint64>)
  DEF2_WMS( L2toL1Mem        , Rol_Long     , Wasm::WasmMath::Rol<int64>)
  DEF2_WMS( L2toL1Mem        , Ror_Long     , Wasm::WasmMath::Ror<int64>)
  DEF2_WMS( L1toL1Mem        , Clz_Long     , Wasm::WasmMath::Clz<int64>)
  DEF2_WMS( L1toL1Mem        , Ctz_Long     , Wasm::WasmMath::Ctz<int64>)
  DEF2_WMS( L1toL1Mem        , PopCnt_Long  , Wasm::WasmMath::PopCnt<int64>)

// Unsigned Int64 Math
  DEF2_WMS( L2toL1Ctx        , Div_Trap_ULong, (OP_UnsignedDivRemCheck<uint64, &AsmJsMath::DivUnsafe<uint64>>) )
  DEF2_WMS( L2toL1Ctx        , Rem_Trap_ULong, (OP_UnsignedDivRemCheck<uint64, &AsmJsMath::RemUnsafe<uint64>>) )

// Double Math
  DEF2_WMS( D1toD1Mem        , Neg_Db       , AsmJsMath::Neg<double>                             )
  DEF2_WMS( D2toD1Mem        , Add_Db       , AsmJsMath::Add<double>                             )
  DEF2_WMS( D2toD1Mem        , Sub_Db       , AsmJsMath::Sub<double>                             )
  DEF2_WMS( D2toD1Mem        , Mul_Db       , AsmJsMath::Mul<double>                             )
  DEF2_WMS( D2toD1Mem        , Div_Db       , AsmJsMath::DivChecked<double>                      )
  DEF2_WMS( D2toD1Mem        , Rem_Db       , AsmJsMath::RemChecked<double>                      )

// Float Math
  DEF2_WMS( F1toF1Mem        , Neg_Flt      , AsmJsMath::Neg<float>                              )
  DEF2_WMS( F2toF1Mem        , Add_Flt      , AsmJsMath::Add<float>                              )
  DEF2_WMS( F2toF1Mem        , Sub_Flt      , AsmJsMath::Sub<float>                              )
  DEF2_WMS( F2toF1Mem        , Mul_Flt      , AsmJsMath::Mul<float>                              )
  DEF2_WMS( F2toF1Mem        , Div_Flt      , AsmJsMath::DivChecked<float>                       )

// Int32 comparisons
  DEF2_WMS( I2toI1Mem        , CmLt_Int     , AsmJsMath::CmpLt<int>                              )
  DEF2_WMS( I2toI1Mem        , CmLe_Int     , AsmJsMath::CmpLe<int>                              )
  DEF2_WMS( I2toI1Mem        , CmGt_Int     , AsmJsMath::CmpGt<int>                              )
  DEF2_WMS( I2toI1Mem        , CmGe_Int     , AsmJsMath::CmpGe<int>                              )
  DEF2_WMS( I2toI1Mem        , CmEq_Int     , AsmJsMath::CmpEq<int>                              )
  DEF2_WMS( I2toI1Mem        , CmNe_Int     , AsmJsMath::CmpNe<int>                              )
  DEF2_WMS( I1toI1Mem        , Eqz_Int      , Wasm::WasmMath::Eqz<int>                           )

// Unsigned Int32 comparisons
  DEF2_WMS( I2toI1Mem        , CmLt_UInt    , AsmJsMath::CmpLt<unsigned int>                     )
  DEF2_WMS( I2toI1Mem        , CmLe_UInt    , AsmJsMath::CmpLe<unsigned int>                     )
  DEF2_WMS( I2toI1Mem        , CmGt_UInt    , AsmJsMath::CmpGt<unsigned int>                     )
  DEF2_WMS( I2toI1Mem        , CmGe_UInt    , AsmJsMath::CmpGe<unsigned int>                     )

// Int64 comparisons
  DEF2_WMS( L2toI1Mem        , CmEq_Long    , AsmJsMath::CmpEq<int64>)
  DEF2_WMS( L2toI1Mem        , CmNe_Long    , AsmJsMath::CmpNe<int64>)
  DEF2_WMS( L2toI1Mem        , CmLt_Long    , AsmJsMath::CmpLt<int64>)
  DEF2_WMS( L2toI1Mem        , CmLe_Long    , AsmJsMath::CmpLe<int64>)
  DEF2_WMS( L2toI1Mem        , CmGt_Long    , AsmJsMath::CmpGt<int64>)
  DEF2_WMS( L2toI1Mem        , CmGe_Long    , AsmJsMath::CmpGe<int64>)
  DEF2_WMS( L1toI1Mem        , Eqz_Long     , Wasm::WasmMath::Eqz<int64>)

// Unsigned Int64 comparisons
  DEF2_WMS( L2toI1Mem        , CmLt_ULong   , AsmJsMath::CmpLt<uint64>)
  DEF2_WMS( L2toI1Mem        , CmLe_ULong   , AsmJsMath::CmpLe<uint64>)
  DEF2_WMS( L2toI1Mem        , CmGt_ULong   , AsmJsMath::CmpGt<uint64>)
  DEF2_WMS( L2toI1Mem        , CmGe_ULong   , AsmJsMath::CmpGe<uint64>)

// Double comparisons
  DEF2_WMS( D2toI1Mem        , CmLt_Db      , AsmJsMath::CmpLt<double>                           )
  DEF2_WMS( D2toI1Mem        , CmLe_Db      , AsmJsMath::CmpLe<double>                           )
  DEF2_WMS( D2toI1Mem        , CmGt_Db      , AsmJsMath::CmpGt<double>                           )
  DEF2_WMS( D2toI1Mem        , CmGe_Db      , AsmJsMath::CmpGe<double>                           )
  DEF2_WMS( D2toI1Mem        , CmEq_Db      , AsmJsMath::CmpEq<double>                           )
  DEF2_WMS( D2toI1Mem        , CmNe_Db      , AsmJsMath::CmpNe<double>                           )

// Float comparisons
  DEF2_WMS( F2toI1Mem        , CmLt_Flt     , AsmJsMath::CmpLt<float>                            )
  DEF2_WMS( F2toI1Mem        , CmLe_Flt     , AsmJsMath::CmpLe<float>                            )
  DEF2_WMS( F2toI1Mem        , CmGt_Flt     , AsmJsMath::CmpGt<float>                            )
  DEF2_WMS( F2toI1Mem        , CmGe_Flt     , AsmJsMath::CmpGe<float>                            )
  DEF2_WMS( F2toI1Mem        , CmEq_Flt     , AsmJsMath::CmpEq<float>                            )
  DEF2_WMS( F2toI1Mem        , CmNe_Flt     , AsmJsMath::CmpNe<float>                            )

// Standard Math lib
  DEF2_WMS( I2toI1Mem        , Imul_Int     , AsmJsMath::Mul<int>                                )
  DEF2_WMS( D1toD1Mem        , Sin_Db       , Math::Sin                                          )
  DEF2_WMS( D1toD1Mem        , Cos_Db       , Math::Cos                                          )
  DEF2_WMS( D1toD1Mem        , Tan_Db       , Math::Tan                                          )
  DEF2_WMS( D1toD1Mem        , Asin_Db      , Math::Asin                                         )
  DEF2_WMS( D1toD1Mem        , Acos_Db      , Math::Acos                                         )
  DEF2_WMS( D1toD1Mem        , Atan_Db      , Math::Atan                                         )
  DEF2_WMS( D1toD1Mem        , Ceil_Db      , ::ceil                                             )
  DEF2_WMS( F1toF1Mem        , Ceil_Flt     , ::ceilf                                            )
  DEF2_WMS( D1toD1Mem        , Floor_Db     , ::floor                                            )
  DEF2_WMS( F1toF1Mem        , Floor_Flt    , ::floorf                                           )
  DEF2_WMS( D1toD1Mem        , Exp_Db       , Math::Exp                                          )
  DEF2_WMS( D1toD1Mem        , Log_Db       , Math::Log                                          )
  DEF2_WMS( D2toD1Mem        , Pow_Db       , Math::Pow                                          )
  DEF2_WMS( D1toD1Mem        , Sqrt_Db      , ::sqrt                                             )
  DEF2_WMS( F1toF1Mem        , Sqrt_Flt     , ::sqrtf                                            )
  DEF2_WMS( D1toD1Mem        , Abs_Db       , AsmJsMath::Abs<double>                             )
  DEF2_WMS( F1toF1Mem        , Abs_Flt      , AsmJsMath::Abs<float>                              )
  DEF2_WMS( D2toD1Mem        , Atan2_Db     , Math::Atan2                                        )
  DEF2_WMS( D2toD1Mem        , Min_Db       , AsmJsMath::Min<double>                             )
  DEF2_WMS( F2toF1Mem        , Min_Flt      , AsmJsMath::Min<float>                              )
  DEF2_WMS( D2toD1Mem        , Max_Db       , AsmJsMath::Max<double>                             )
  DEF2_WMS( F2toF1Mem        , Max_Flt      , AsmJsMath::Max<float>                              )

  DEF2_WMS( F1toF1Mem        , Fround_Flt   , (float)                                            )
  DEF2_WMS( D1toF1Mem        , Fround_Db    , (float)                                            )
  DEF2_WMS( I1toF1Mem        , Fround_Int   , (float)                                            )
EXDEF2_WMS( F2toF1Mem        , Copysign_Flt , Wasm::WasmMath::Copysign<float>                    )
EXDEF2_WMS( D2toD1Mem        , Copysign_Db  , Wasm::WasmMath::Copysign<double>                   )

EXDEF2_WMS( F1toF1Mem        , Trunc_Flt        , Wasm::WasmMath::Trunc<float>                       )
EXDEF2_WMS( F1toF1Mem        , Nearest_Flt      , Wasm::WasmMath::Nearest<float>                     )
EXDEF2_WMS( D1toD1Mem        , Trunc_Db         , Wasm::WasmMath::Trunc<double>                      )
EXDEF2_WMS( D1toD1Mem        , Nearest_Db       , Wasm::WasmMath::Nearest<double>                    )
EXDEF2_WMS( VtoI1Mem         , CurrentMemory_Int, OP_GetMemorySize                                   )
EXDEF2_WMS( I1toI1Mem        , GrowMemory       , OP_GrowMemory                                      )
EXDEF2    ( EMPTYASMJS       , Unreachable_Void , OP_Unreachable                                     )
EXDEF2_WMS( D1toI1Ctx        , Conv_Check_DTI   , JavascriptConversion::F64TOI32                     )
EXDEF2_WMS( F1toI1Ctx        , Conv_Check_FTI   , JavascriptConversion::F32TOI32                     )
EXDEF2_WMS( D1toI1Ctx        , Conv_Check_DTU   , JavascriptConversion::F64TOU32                     )
EXDEF2_WMS( F1toI1Ctx        , Conv_Check_FTU   , JavascriptConversion::F32TOU32                     )
EXDEF2_WMS( F1toL1Ctx        , Conv_Check_FTL   , JavascriptConversion::F32TOI64                     )
EXDEF2_WMS( F1toL1Ctx        , Conv_Check_FTUL  , JavascriptConversion::F32TOU64                     )
EXDEF2_WMS( D1toL1Ctx        , Conv_Check_DTL   , JavascriptConversion::F64TOI64                     )
EXDEF2_WMS( D1toL1Ctx        , Conv_Check_DTUL  , JavascriptConversion::F64TOU64                     )

  DEF2_WMS( IP_TARG_ASM      , AsmJsLoopBodyStart, OP_ProfiledLoopBodyStart                      )

#if ENABLE_DEBUG_CONFIG_OPTIONS
EXDEF3_WMS( CUSTOM_ASMJS, PrintFuncName  , OP_WasmPrintFunc , Int2)
EXDEF2    ( EMPTYASMJS, PrintArgSeparator, WAsmJs::Tracing::PrintArgSeparator)
EXDEF2    ( EMPTYASMJS, PrintBeginCall, WAsmJs::Tracing::PrintBeginCall)
EXDEF2    ( EMPTYASMJS, PrintNewLine, WAsmJs::Tracing::PrintNewLine)
EXDEF3_WMS( CUSTOM_ASMJS, PrintEndCall, WAsmJs::Tracing::PrintEndCall, Int2)
EXDEF2_WMS( I1toI1Mem, PrintI32, WAsmJs::Tracing::PrintI32 )
EXDEF2_WMS( L1toL1Mem, PrintI64, WAsmJs::Tracing::PrintI64 )
EXDEF2_WMS( F1toF1Mem, PrintF32, WAsmJs::Tracing::PrintF32 )
EXDEF2_WMS( D1toD1Mem, PrintF64, WAsmJs::Tracing::PrintF64 )
#endif
  //unary ops

EXDEF2_WMS( SIMD_F4_1toF4_1  , Simd128_Ld_F4        , (AsmJsSIMDValue)                                   )
  DEF2_WMS( SIMD_I4_1toI4_1  , Simd128_Ld_I4        , (AsmJsSIMDValue)                                   )
EXDEF2_WMS( SIMD_B4_1toB4_1  , Simd128_Ld_B4        , (AsmJsSIMDValue)                                   )
EXDEF2_WMS( SIMD_B8_1toB8_1  , Simd128_Ld_B8        , (AsmJsSIMDValue)                                   )
EXDEF2_WMS( SIMD_B16_1toB16_1, Simd128_Ld_B16       , (AsmJsSIMDValue)                                   )

EXDEF2_WMS( SIMD_F4toF4_1    , Simd128_FloatsToF4   , SIMDFloat32x4Operation::OpFloat32x4                )
  DEF2_WMS( SIMD_I4toI4_1    , Simd128_IntsToI4     , SIMDInt32x4Operation::OpInt32x4                    )

EXDEF4_WMS( TEMPLATE_ASMJS   , Simd128_LdSlot_F4    , OP_LdSlotPrimitive          , ElementSlot, AsmJsSIMDValue)
  DEF4_WMS( TEMPLATE_ASMJS   , Simd128_LdSlot_I4    , OP_LdSlotPrimitive          , ElementSlot, AsmJsSIMDValue)

EXDEF4_WMS(TEMPLATE_ASMJS    , Simd128_StSlot_F4    , OP_StSlotPrimitive          , ElementSlot, AsmJsSIMDValue)
  DEF4_WMS(TEMPLATE_ASMJS    , Simd128_StSlot_I4    , OP_StSlotPrimitive          , ElementSlot, AsmJsSIMDValue)

EXDEF2_WMS( SIMD_F4_1toF4_1  , Simd128_Return_F4    , (AsmJsSIMDValue)                                   )
  DEF2_WMS( SIMD_I4_1toI4_1  , Simd128_Return_I4    , (AsmJsSIMDValue)                                   )

EXDEF2_WMS( SIMD_F1toF4_1    , Simd128_Splat_F4     ,Js::SIMDFloat32x4Operation::OpSplat                 )
  DEF2_WMS( SIMD_I1toI4_1    , Simd128_Splat_I4     ,Js::SIMDInt32x4Operation::OpSplat                   )

//EXDEF2_WMS( SIMD_D2_1toF4_1  , Simd128_FromFloat64x2_F4    ,SIMDFloat32x4Operation::OpFromFloat64x2      )
//EXDEF2_WMS( SIMD_D2_1toF4_1  , Simd128_FromFloat64x2Bits_F4,Js::FromSimdBits                             )
EXDEF2_WMS( SIMD_I4_1toF4_1  , Simd128_FromInt32x4_F4      ,SIMDFloat32x4Operation::OpFromInt32x4        )
EXDEF2_WMS( SIMD_I4_1toF4_1  , Simd128_FromInt32x4Bits_F4  ,Js::SIMDUtils::FromSimdBits                  )
  DEF3_WMS( CUSTOM_ASMJS     , Simd128_FromFloat32x4_I4    , OP_SimdInt32x4FromFloat32x4 ,  Int32x4_1Float32x4_1 )

  //DEF2_WMS( SIMD_D2_1toI4_1  , Simd128_FromFloat64x2_I4    ,SIMDInt32x4Operation::OpFromFloat64x2        )
  //DEF2_WMS( SIMD_D2_1toI4_1  , Simd128_FromFloat64x2Bits_I4,Js::FromSimdBits                             )
  DEF2_WMS( SIMD_F4_1toI4_1  , Simd128_FromFloat32x4Bits_I4,Js::SIMDUtils::FromSimdBits                  )

EXDEF2_WMS( SIMD_F4_1toF4_1  , Simd128_Abs_F4              ,SIMDFloat32x4Operation::OpAbs                )

EXDEF2_WMS( SIMD_F4_1toF4_1  , Simd128_Neg_F4              ,SIMDFloat32x4Operation::OpNeg                )
  DEF2_WMS( SIMD_I4_1toI4_1  , Simd128_Neg_I4              ,SIMDInt32x4Operation::OpNeg                  )

EXDEF2_WMS( SIMD_F4_1toF4_1  , Simd128_Rcp_F4              ,SIMDFloat32x4Operation::OpReciprocal         )
EXDEF2_WMS( SIMD_F4_1toF4_1  , Simd128_RcpSqrt_F4          ,SIMDFloat32x4Operation::OpReciprocalSqrt     )
EXDEF2_WMS( SIMD_F4_1toF4_1  , Simd128_Sqrt_F4             ,SIMDFloat32x4Operation::OpSqrt               )

  DEF2_WMS( SIMD_I4_1toI4_1  , Simd128_Not_I4              , Js::SIMDInt32x4Operation::OpNot             )
EXDEF2_WMS( SIMD_B4_1toB4_1  , Simd128_Not_B4              , Js::SIMDInt32x4Operation::OpNot             )
EXDEF2_WMS( SIMD_B8_1toB8_1  , Simd128_Not_B8              , Js::SIMDInt32x4Operation::OpNot             )
EXDEF2_WMS( SIMD_B16_1toB16_1, Simd128_Not_B16             , Js::SIMDInt32x4Operation::OpNot             )

EXDEF2_WMS( SIMD_B4_1toI1, Simd128_AllTrue_B4            , Js::SIMDBool32x4Operation::OpAllTrue        )
EXDEF2_WMS( SIMD_B8_1toI1, Simd128_AllTrue_B8            , Js::SIMDBool32x4Operation::OpAllTrue        )
EXDEF2_WMS( SIMD_B16_1toI1, Simd128_AllTrue_B16          , Js::SIMDBool32x4Operation::OpAllTrue        )

EXDEF2_WMS( SIMD_B4_1toI1, Simd128_AnyTrue_B4            , Js::SIMDBool32x4Operation::OpAnyTrue        )
EXDEF2_WMS( SIMD_B8_1toI1, Simd128_AnyTrue_B8            , Js::SIMDBool32x4Operation::OpAnyTrue        )
EXDEF2_WMS( SIMD_B16_1toI1, Simd128_AnyTrue_B16          , Js::SIMDBool32x4Operation::OpAnyTrue        )

  DEF2_WMS(SIMD_I4_1I1toI4_1 , Simd128_ShLtByScalar_I4     , Js::SIMDInt32x4Operation::OpShiftLeftByScalar)
  DEF2_WMS(SIMD_I4_1I1toI4_1 , Simd128_ShRtByScalar_I4     , Js::SIMDInt32x4Operation::OpShiftRightByScalar)

  // binary ops
EXDEF2_WMS( SIMD_F4_2toF4_1  , Simd128_Add_F4              , Js::SIMDFloat32x4Operation::OpAdd           )
  DEF2_WMS( SIMD_I4_2toI4_1  , Simd128_Add_I4              , Js::SIMDInt32x4Operation::OpAdd             )

EXDEF2_WMS( SIMD_F4_2toF4_1  , Simd128_Sub_F4              , Js::SIMDFloat32x4Operation::OpSub           )
  DEF2_WMS( SIMD_I4_2toI4_1  , Simd128_Sub_I4              , Js::SIMDInt32x4Operation::OpSub             )

EXDEF2_WMS( SIMD_F4_2toF4_1  , Simd128_Mul_F4              , Js::SIMDFloat32x4Operation::OpMul           )
  DEF2_WMS( SIMD_I4_2toI4_1  , Simd128_Mul_I4              , Js::SIMDInt32x4Operation::OpMul             )

EXDEF2_WMS( SIMD_F4_2toF4_1  , Simd128_Div_F4              , Js::SIMDFloat32x4Operation::OpDiv           )

EXDEF2_WMS( SIMD_F4_2toF4_1  , Simd128_Min_F4              , Js::SIMDFloat32x4Operation::OpMin           )

EXDEF2_WMS( SIMD_F4_2toF4_1  , Simd128_Max_F4              , Js::SIMDFloat32x4Operation::OpMax           )

EXDEF2_WMS( SIMD_F4_2toB4_1  , Simd128_Lt_F4               , Js::SIMDFloat32x4Operation::OpLessThan      )
  DEF2_WMS( SIMD_I4_2toB4_1  , Simd128_Lt_I4               , Js::SIMDInt32x4Operation::OpLessThan        )

EXDEF2_WMS( SIMD_F4_2toB4_1  , Simd128_LtEq_F4             , Js::SIMDFloat32x4Operation::OpLessThanOrEqual)

EXDEF2_WMS( SIMD_F4_2toB4_1  , Simd128_Eq_F4               , Js::SIMDFloat32x4Operation::OpEqual         )
  DEF2_WMS( SIMD_I4_2toB4_1  , Simd128_Eq_I4               , Js::SIMDInt32x4Operation::OpEqual           )

EXDEF2_WMS( SIMD_F4_2toB4_1  , Simd128_Neq_F4              , Js::SIMDFloat32x4Operation::OpNotEqual      )

EXDEF2_WMS( SIMD_F4_2toB4_1  , Simd128_GtEq_F4             , Js::SIMDFloat32x4Operation::OpGreaterThanOrEqual)

EXDEF2_WMS( SIMD_F4_2toB4_1  , Simd128_Gt_F4               , Js::SIMDFloat32x4Operation::OpGreaterThan   )
  DEF2_WMS( SIMD_I4_2toB4_1  , Simd128_Gt_I4               , Js::SIMDInt32x4Operation::OpGreaterThan     )

  DEF2_WMS( SIMD_I4_2toI4_1  , Simd128_And_I4              , Js::SIMDInt32x4Operation::OpAnd             )
EXDEF2_WMS( SIMD_B4_2toB4_1, Simd128_And_B4              , Js::SIMDInt32x4Operation::OpAnd            )
EXDEF2_WMS( SIMD_B8_2toB8_1, Simd128_And_B8              , Js::SIMDInt32x4Operation::OpAnd            )
EXDEF2_WMS( SIMD_B16_2toB16_1, Simd128_And_B16           , Js::SIMDInt32x4Operation::OpAnd            )

  DEF2_WMS( SIMD_I4_2toI4_1  , Simd128_Or_I4               , Js::SIMDInt32x4Operation::OpOr              )
EXDEF2_WMS( SIMD_B4_2toB4_1  , Simd128_Or_B4             , Js::SIMDInt32x4Operation::OpOr             )
EXDEF2_WMS( SIMD_B8_2toB8_1  , Simd128_Or_B8             , Js::SIMDInt32x4Operation::OpOr             )
EXDEF2_WMS( SIMD_B16_2toB16_1  , Simd128_Or_B16          , Js::SIMDInt32x4Operation::OpOr             )

  DEF2_WMS( SIMD_I4_2toI4_1  , Simd128_Xor_I4              , Js::SIMDInt32x4Operation::OpXor             )
EXDEF2_WMS( SIMD_B4_2toB4_1, Simd128_Xor_B4              , Js::SIMDInt32x4Operation::OpXor             )
EXDEF2_WMS( SIMD_B8_2toB8_1, Simd128_Xor_B8              , Js::SIMDInt32x4Operation::OpXor             )
EXDEF2_WMS( SIMD_B16_2toB16_1, Simd128_Xor_B16           , Js::SIMDInt32x4Operation::OpXor             )
  // ternary ops
EXDEF2_WMS( SIMD_F4_3toF4_1      , Simd128_Clamp_F4        , Js::SIMDFloat32x4Operation::OpClamp         )
EXDEF2_WMS( SIMD_B4_1F4_2toF4_1  , Simd128_Select_F4       , Js::SIMDFloat32x4Operation::OpSelect        )
  DEF2_WMS( SIMD_B4_1I4_2toI4_1  , Simd128_Select_I4       , Js::SIMDInt32x4Operation::OpSelect          )
EXDEF2_WMS( SIMD_B8_1I8_2toI8_1  , Simd128_Select_I8       , Js::SIMDInt32x4Operation::OpSelect          )
EXDEF2_WMS( SIMD_B16_1I16_2toI16_1  , Simd128_Select_I16   , Js::SIMDInt32x4Operation::OpSelect          )
EXDEF2_WMS( SIMD_B4_1U4_2toU4_1      , Simd128_Select_U4   , Js::SIMDInt32x4Operation::OpSelect         )
EXDEF2_WMS( SIMD_B8_1U8_2toU8_1     , Simd128_Select_U8   , Js::SIMDInt32x4Operation::OpSelect         )
EXDEF2_WMS( SIMD_B16_1U16_2toU16_1   , Simd128_Select_U16  , Js::SIMDInt32x4Operation::OpSelect         )

  // args out, copy value to outParams
EXDEF2_WMS   ( SIMD_F4_1toR1Mem  , Simd128_I_ArgOut_F4     , OP_I_SetOutAsmSimd                          )
  DEF2_WMS   ( SIMD_I4_1toR1Mem  , Simd128_I_ArgOut_I4     , OP_I_SetOutAsmSimd                          )

EXDEF2_WMS   ( SIMD_F4_1toF4_1   , Simd128_I_Conv_VTF4     , (AsmJsSIMDValue)                            )
  DEF2_WMS   ( SIMD_I4_1toI4_1   , Simd128_I_Conv_VTI4     , (AsmJsSIMDValue)                            )

EXDEF2_WMS   ( SIMD_F4_1I4toF4_1   , Simd128_Swizzle_F4      , SIMDUtils::SIMD128InnerShuffle            )
EXDEF2_WMS   ( SIMD_F4_2I4toF4_1   , Simd128_Shuffle_F4      , SIMDUtils::SIMD128InnerShuffle            )

  DEF2_WMS   ( SIMD_I4_1I4toI4_1   , Simd128_Swizzle_I4      , SIMDUtils::SIMD128InnerShuffle            )
  DEF2_WMS   ( SIMD_I4_2I4toI4_1   , Simd128_Shuffle_I4      , SIMDUtils::SIMD128InnerShuffle            )

  DEF2_WMS    ( SIMD_I8_1toI8_1      , Simd128_Ld_I8           , (AsmJsSIMDValue)                           )
  DEF2_WMS    ( SIMD_U4_1toU4_1      , Simd128_Ld_U4           , (AsmJsSIMDValue)                           )
  DEF2_WMS    ( SIMD_U8_1toU8_1      , Simd128_Ld_U8           , (AsmJsSIMDValue)                           )
  DEF2_WMS    ( SIMD_U16_1toU16_1    , Simd128_Ld_U16          , (AsmJsSIMDValue)                           )

  DEF4_WMS    ( TEMPLATE_ASMJS        , Simd128_LdSlot_I8      , OP_LdSlotPrimitive  , ElementSlot, AsmJsSIMDValue)
  DEF4_WMS    ( TEMPLATE_ASMJS        , Simd128_LdSlot_I16     , OP_LdSlotPrimitive  , ElementSlot, AsmJsSIMDValue)

  // Extended opcodes
EXDEF4_WMS    ( TEMPLATE_ASMJS        , Simd128_LdSlot_U4      , OP_LdSlotPrimitive  , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS        , Simd128_LdSlot_U8      , OP_LdSlotPrimitive  , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS        , Simd128_LdSlot_U16     , OP_LdSlotPrimitive  , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS        , Simd128_LdSlot_B4    , OP_LdSlotPrimitive      , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS        , Simd128_LdSlot_B8    , OP_LdSlotPrimitive      , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS        , Simd128_LdSlot_B16   , OP_LdSlotPrimitive      , ElementSlot, AsmJsSIMDValue)

EXDEF4_WMS    ( TEMPLATE_ASMJS         , Simd128_StSlot_I8    , OP_StSlotPrimitive    , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS(TEMPLATE_ASMJS         , Simd128_StSlot_I16   , OP_StSlotPrimitive    , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS         , Simd128_StSlot_U4    , OP_StSlotPrimitive    , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS         , Simd128_StSlot_U8    , OP_StSlotPrimitive    , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS         , Simd128_StSlot_U16   , OP_StSlotPrimitive    , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS         , Simd128_StSlot_B4    , OP_StSlotPrimitive      , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS         , Simd128_StSlot_B8    , OP_StSlotPrimitive      , ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS    ( TEMPLATE_ASMJS         , Simd128_StSlot_B16   , OP_StSlotPrimitive      , ElementSlot, AsmJsSIMDValue)

EXDEF3_WMS    ( CUSTOM_ASMJS     , Simd128_IntsToB4        , OP_SimdBool32x4      , Bool32x4_1Int4)
EXDEF3_WMS    ( CUSTOM_ASMJS     , Simd128_IntsToB8        , OP_SimdBool16x8      , Bool16x8_1Int8)
EXDEF3_WMS    ( CUSTOM_ASMJS     , Simd128_IntsToB16       , OP_SimdBool8x16      , Bool8x16_1Int16)

EXDEF2_WMS    ( SIMD_I1toB4_1    , Simd128_Splat_B4        , Js::SIMDInt32x4Operation::OpSplat         )
EXDEF2_WMS    ( SIMD_I1toB8_1    , Simd128_Splat_B8        , Js::SIMDInt32x4Operation::OpSplat         )
EXDEF2_WMS    ( SIMD_I1toB16_1   , Simd128_Splat_B16       , Js::SIMDInt32x4Operation::OpSplat         )

EXDEF2_WMS ( SIMD_I16_1toI16_1  , Simd128_Ld_I16           , (AsmJsSIMDValue)                           )
EXDEF2_WMS ( SIMD_I1toI16_1     , Simd128_Splat_I16        , Js::SIMDInt8x16Operation::OpSplat            )

EXDEF2_WMS ( SIMD_I16_2toI16_1  , Simd128_Add_I16           , Js::SIMDInt8x16Operation::OpAdd             )
EXDEF2_WMS ( SIMD_I16_2toI16_1  , Simd128_Sub_I16           , Js::SIMDInt8x16Operation::OpSub             )
EXDEF2_WMS ( SIMD_I16_2toI16_1  , Simd128_Mul_I16           , Js::SIMDInt8x16Operation::OpMul             )
EXDEF2_WMS ( SIMD_I16_1toI16_1  , Simd128_Neg_I16           , SIMDInt8x16Operation::OpNeg                 )
EXDEF2_WMS ( SIMD_I16_1toI16_1  , Simd128_Not_I16           , Js::SIMDInt32x4Operation::OpNot             )
EXDEF2_WMS ( SIMD_I16_2toB16_1  , Simd128_Lt_I16            , Js::SIMDInt8x16Operation::OpLessThan        )
EXDEF2_WMS ( SIMD_I16_2toB16_1  , Simd128_Eq_I16            , Js::SIMDInt8x16Operation::OpEqual           )
EXDEF2_WMS ( SIMD_I16_2toB16_1  , Simd128_Gt_I16            , Js::SIMDInt8x16Operation::OpGreaterThan     )
EXDEF2_WMS ( SIMD_I16_2toI16_1  , Simd128_And_I16           , Js::SIMDInt32x4Operation::OpAnd             )
EXDEF2_WMS ( SIMD_I16_2toI16_1  , Simd128_Or_I16            , Js::SIMDInt32x4Operation::OpOr              )
EXDEF2_WMS ( SIMD_I16_2toI16_1  , Simd128_Xor_I16           , Js::SIMDInt32x4Operation::OpXor             )

EXDEF2_WMS ( SIMD_I16_1toI16_1  , Simd128_Return_I16       , (AsmJsSIMDValue)                            )
EXDEF2_WMS ( SIMD_I16_1toR1Mem  , Simd128_I_ArgOut_I16     , OP_I_SetOutAsmSimd                          )
EXDEF2_WMS ( SIMD_I16_1toI16_1  , Simd128_I_Conv_VTI16     , (AsmJsSIMDValue)                            )

  //Lane Access
EXDEF2_WMS   ( SIMD_I16_1I1toI1    , Simd128_ExtractLane_I16 , SIMDUtils::SIMD128InnerExtractLaneI16       )
EXDEF2_WMS   ( SIMD_I16_1I2toI16_1 , Simd128_ReplaceLane_I16 , SIMDUtils::SIMD128InnerReplaceLaneI16       )
EXDEF2_WMS   ( SIMD_I4_1I1toI1   , Simd128_ExtractLane_I4  , SIMDUtils::SIMD128InnerExtractLaneI4        )
EXDEF2_WMS   ( SIMD_F4_1I1toF1   , Simd128_ExtractLane_F4  , SIMDUtils::SIMD128InnerExtractLaneF4        )
EXDEF2_WMS   ( SIMD_I8_1I1toI1   , Simd128_ExtractLane_I8  , SIMDUtils::SIMD128InnerExtractLaneI8        )
EXDEF2_WMS   ( SIMD_U4_1I1toI1   , Simd128_ExtractLane_U4  , SIMDUtils::SIMD128InnerExtractLaneI4        )
EXDEF2_WMS   ( SIMD_U8_1I1toI1   , Simd128_ExtractLane_U8  , SIMDUtils::SIMD128InnerExtractLaneI8        )
EXDEF2_WMS   ( SIMD_U16_1I1toI1  , Simd128_ExtractLane_U16 , SIMDUtils::SIMD128InnerExtractLaneI16       )
EXDEF2_WMS   ( SIMD_B4_1I1toI1   , Simd128_ExtractLane_B4  , SIMDUtils::SIMD128InnerExtractLaneI4        )
EXDEF2_WMS   ( SIMD_B8_1I1toI1   , Simd128_ExtractLane_B8  , SIMDUtils::SIMD128InnerExtractLaneI8        )
EXDEF2_WMS   ( SIMD_B16_1I1toI1  , Simd128_ExtractLane_B16 , SIMDUtils::SIMD128InnerExtractLaneI16       )

EXDEF2_WMS   ( SIMD_I4_1I2toI4_1   , Simd128_ReplaceLane_I4 , SIMDUtils::SIMD128InnerReplaceLaneI4       )
EXDEF2_WMS   ( SIMD_F4_1I1F1toF4_1 , Simd128_ReplaceLane_F4 , SIMDUtils::SIMD128InnerReplaceLaneF4       )
EXDEF2_WMS   ( SIMD_I8_1I2toI8_1   , Simd128_ReplaceLane_I8 , SIMDUtils::SIMD128InnerReplaceLaneI8       )
EXDEF2_WMS   ( SIMD_U4_1I2toU4_1   , Simd128_ReplaceLane_U4 , SIMDUtils::SIMD128InnerReplaceLaneI4       )
EXDEF2_WMS   ( SIMD_U8_1I2toU8_1   , Simd128_ReplaceLane_U8 , SIMDUtils::SIMD128InnerReplaceLaneI8       )
EXDEF2_WMS   ( SIMD_U16_1I2toU16_1 , Simd128_ReplaceLane_U16, SIMDUtils::SIMD128InnerReplaceLaneI16      )
EXDEF2_WMS   ( SIMD_B4_1I2toB4_1   , Simd128_ReplaceLane_B4 , SIMDUtils::SIMD128InnerReplaceLaneI4       )
EXDEF2_WMS   ( SIMD_B8_1I2toB8_1   , Simd128_ReplaceLane_B8 , SIMDUtils::SIMD128InnerReplaceLaneI8       )
EXDEF2_WMS   ( SIMD_B16_1I2toB16_1 , Simd128_ReplaceLane_B16, SIMDUtils::SIMD128InnerReplaceLaneI16      )

EXDEF3_WMS   ( CUSTOM_ASMJS      , Simd128_LdArr_F4        , OP_SimdLdArrGeneric , AsmSimdTypedArr       )
EXDEF3_WMS   ( CUSTOM_ASMJS      , Simd128_LdArr_I4        , OP_SimdLdArrGeneric , AsmSimdTypedArr       )
EXDEF3_WMS   ( CUSTOM_ASMJS      , Simd128_StArr_F4        , OP_SimdStArrGeneric , AsmSimdTypedArr       )
EXDEF3_WMS   ( CUSTOM_ASMJS      , Simd128_StArr_I4        , OP_SimdStArrGeneric , AsmSimdTypedArr       )

EXDEF3_WMS   ( CUSTOM_ASMJS      , Simd128_LdArrConst_F4   , OP_SimdLdArrConstIndex  , AsmSimdTypedArr   )
EXDEF3_WMS   ( CUSTOM_ASMJS      , Simd128_LdArrConst_I4   , OP_SimdLdArrConstIndex  , AsmSimdTypedArr   )
EXDEF3_WMS   ( CUSTOM_ASMJS      , Simd128_StArrConst_F4   , OP_SimdStArrConstIndex  , AsmSimdTypedArr   )
EXDEF3_WMS   ( CUSTOM_ASMJS      , Simd128_StArrConst_I4   , OP_SimdStArrConstIndex  , AsmSimdTypedArr   )

EXDEF2_WMS   ( SIMD_U4_1toF4_1    , Simd128_FromUint32x4_F4      , Js::SIMDFloat32x4Operation::OpFromUint32x4  )
EXDEF2_WMS   ( SIMD_I8_1toF4_1    , Simd128_FromInt16x8Bits_F4   , Js::SIMDUtils::FromSimdBits            )
EXDEF2_WMS   ( SIMD_I16_1toF4_1   , Simd128_FromInt8x16Bits_F4   , Js::SIMDUtils::FromSimdBits            )
EXDEF2_WMS   ( SIMD_U4_1toF4_1    , Simd128_FromUint32x4Bits_F4  , Js::SIMDUtils::FromSimdBits            )
EXDEF2_WMS   ( SIMD_U8_1toF4_1    , Simd128_FromUint16x8Bits_F4  , Js::SIMDUtils::FromSimdBits            )
EXDEF2_WMS   ( SIMD_U16_1toF4_1   , Simd128_FromUint8x16Bits_F4  , Js::SIMDUtils::FromSimdBits            )

EXDEF2_WMS   ( SIMD_I8_1toI4_1    , Simd128_FromInt16x8Bits_I4   , Js::SIMDUtils::FromSimdBits            )
EXDEF2_WMS   ( SIMD_I16_1toI4_1   , Simd128_FromInt8x16Bits_I4   , Js::SIMDUtils::FromSimdBits            )
EXDEF2_WMS   ( SIMD_U4_1toI4_1    , Simd128_FromUint32x4Bits_I4  , Js::SIMDUtils::FromSimdBits            )
EXDEF2_WMS   ( SIMD_U8_1toI4_1    , Simd128_FromUint16x8Bits_I4  , Js::SIMDUtils::FromSimdBits            )
EXDEF2_WMS   ( SIMD_U16_1toI4_1   , Simd128_FromUint8x16Bits_I4  , Js::SIMDUtils::FromSimdBits            )

EXDEF3_WMS   ( CUSTOM_ASMJS       , Simd128_IntsToI8                 , OP_SimdInt16x8     , Int16x8_1Int8 )
EXDEF3_WMS   ( CUSTOM_ASMJS       , Simd128_IntsToI16                , OP_SimdInt8x16     , Int8x16_1Int16)
EXDEF2_WMS   ( SIMD_I4toU4_1      , Simd128_IntsToU4                 , SIMDUint32x4Operation::OpUint32x4  )
EXDEF3_WMS   ( CUSTOM_ASMJS       , Simd128_IntsToU8                 , OP_SimdUint16x8     , Uint16x8_1Int8)
EXDEF3_WMS   ( CUSTOM_ASMJS       , Simd128_IntsToU16                , OP_SimdUint8x16     , Uint8x16_1Int16)

EXDEF2_WMS   ( SIMD_I8_1toR1Mem  , Simd128_I_ArgOut_I8     , OP_I_SetOutAsmSimd                          )
EXDEF2_WMS   ( SIMD_U4_1toR1Mem  , Simd128_I_ArgOut_U4     , OP_I_SetOutAsmSimd                          )
EXDEF2_WMS   ( SIMD_U8_1toR1Mem  , Simd128_I_ArgOut_U8     , OP_I_SetOutAsmSimd                          )
EXDEF2_WMS   ( SIMD_U16_1toR1Mem , Simd128_I_ArgOut_U16    , OP_I_SetOutAsmSimd                          )
EXDEF2_WMS   ( SIMD_B4_1toR1Mem  , Simd128_I_ArgOut_B4     , OP_I_SetOutAsmSimd                          )
EXDEF2_WMS   ( SIMD_B8_1toR1Mem  , Simd128_I_ArgOut_B8     , OP_I_SetOutAsmSimd                          )
EXDEF2_WMS   ( SIMD_B16_1toR1Mem , Simd128_I_ArgOut_B16    , OP_I_SetOutAsmSimd                          )

EXDEF2_WMS   ( SIMD_I8_1toI8_1   , Simd128_I_Conv_VTI8     , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_U4_1toU4_1   , Simd128_I_Conv_VTU4     , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_U8_1toU8_1   , Simd128_I_Conv_VTU8     , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_U16_1toU16_1 , Simd128_I_Conv_VTU16    , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_B4_1toB4_1   , Simd128_I_Conv_VTB4     , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_B8_1toB8_1   , Simd128_I_Conv_VTB8     , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_B16_1toB16_1 , Simd128_I_Conv_VTB16    , (AsmJsSIMDValue)                            )

EXDEF2_WMS   ( SIMD_I8_1toI8_1   , Simd128_Return_I8       , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_U4_1toU4_1   , Simd128_Return_U4       , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_U8_1toU8_1   , Simd128_Return_U8       , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_U16_1toU16_1 , Simd128_Return_U16      , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_B4_1toB4_1   , Simd128_Return_B4       , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_B8_1toB8_1   , Simd128_Return_B8       , (AsmJsSIMDValue)                            )
EXDEF2_WMS   ( SIMD_B16_1toB16_1 , Simd128_Return_B16      , (AsmJsSIMDValue)                            )

EXDEF2_WMS   ( SIMD_I8_1I8toI8_1    , Simd128_Swizzle_I8   , SIMDUtils::SIMD128InnerShuffle              )
EXDEF2_WMS   ( SIMD_I8_2I8toI8_1    , Simd128_Shuffle_I8   , SIMDUtils::SIMD128InnerShuffle              )
EXDEF2_WMS     ( SIMD_I16_1I16toI16_1 , Simd128_Swizzle_I16  , SIMDUtils::SIMD128InnerShuffle              )
EXDEF2_WMS     ( SIMD_I16_2I16toI16_1 , Simd128_Shuffle_I16  , SIMDUtils::SIMD128InnerShuffle              )
EXDEF2_WMS   ( SIMD_U4_1I4toU4_1    , Simd128_Swizzle_U4   , SIMDUtils::SIMD128InnerShuffle              )
EXDEF2_WMS   ( SIMD_U4_2I4toU4_1    , Simd128_Shuffle_U4   , SIMDUtils::SIMD128InnerShuffle              )
EXDEF2_WMS   ( SIMD_U8_1I8toU8_1    , Simd128_Swizzle_U8   , SIMDUtils::SIMD128InnerShuffle              )
EXDEF2_WMS   ( SIMD_U8_2I8toU8_1    , Simd128_Shuffle_U8   , SIMDUtils::SIMD128InnerShuffle              )
EXDEF2_WMS   ( SIMD_U16_1I16toU16_1 , Simd128_Swizzle_U16  , SIMDUtils::SIMD128InnerShuffle              )
EXDEF2_WMS   ( SIMD_U16_2I16toU16_1 , Simd128_Shuffle_U16  , SIMDUtils::SIMD128InnerShuffle              )

EXDEF2_WMS( SIMD_I1toI8_1    , Simd128_Splat_I8     ,Js::SIMDInt16x8Operation::OpSplat                   )
EXDEF2_WMS( SIMD_I1toU4_1    , Simd128_Splat_U4     ,Js::SIMDUint32x4Operation::OpSplat                  )
EXDEF2_WMS( SIMD_I1toU8_1    , Simd128_Splat_U8     ,Js::SIMDInt16x8Operation::OpSplat                   )
EXDEF2_WMS( SIMD_I1toU16_1   , Simd128_Splat_U16    ,Js::SIMDInt8x16Operation::OpSplat                   )

EXDEF2_WMS( SIMD_I8_2toI8_1    , Simd128_And_I8              , Js::SIMDInt16x8Operation::OpAnd           )
EXDEF2_WMS( SIMD_U4_2toU4_1    , Simd128_And_U4              , Js::SIMDInt32x4Operation::OpAnd           )
EXDEF2_WMS( SIMD_U8_2toU8_1    , Simd128_And_U8              , Js::SIMDInt32x4Operation::OpAnd           )
EXDEF2_WMS( SIMD_U16_2toU16_1  , Simd128_And_U16             , Js::SIMDInt32x4Operation::OpAnd           )

EXDEF2_WMS( SIMD_I8_2toI8_1    , Simd128_Or_I8              , Js::SIMDInt16x8Operation::OpOr             )
EXDEF2_WMS( SIMD_U4_2toU4_1    , Simd128_Or_U4              , Js::SIMDInt32x4Operation::OpOr             )
EXDEF2_WMS( SIMD_U8_2toU8_1    , Simd128_Or_U8              , Js::SIMDInt32x4Operation::OpOr             )
EXDEF2_WMS( SIMD_U16_2toU16_1  , Simd128_Or_U16             , Js::SIMDInt32x4Operation::OpOr             )

EXDEF2_WMS( SIMD_I8_2toI8_1    , Simd128_Xor_I8              , Js::SIMDInt16x8Operation::OpXor             )
EXDEF2_WMS( SIMD_U4_2toU4_1    , Simd128_Xor_U4              , Js::SIMDInt32x4Operation::OpXor             )
EXDEF2_WMS( SIMD_U8_2toU8_1    , Simd128_Xor_U8              , Js::SIMDInt32x4Operation::OpXor             )
EXDEF2_WMS( SIMD_U16_2toU16_1  , Simd128_Xor_U16             , Js::SIMDInt32x4Operation::OpXor             )

EXDEF2_WMS( SIMD_I8_1toI8_1    , Simd128_Not_I8              , Js::SIMDInt16x8Operation::OpNot             )
EXDEF2_WMS( SIMD_U4_1toU4_1    , Simd128_Not_U4              , Js::SIMDInt32x4Operation::OpNot             )
EXDEF2_WMS( SIMD_U8_1toU8_1    , Simd128_Not_U8              , Js::SIMDInt32x4Operation::OpNot             )
EXDEF2_WMS( SIMD_U16_1toU16_1  , Simd128_Not_U16             , Js::SIMDInt32x4Operation::OpNot             )

EXDEF2_WMS( SIMD_I8_2toI8_1    , Simd128_Add_I8              , Js::SIMDInt16x8Operation::OpAdd             )
EXDEF2_WMS( SIMD_U4_2toU4_1    , Simd128_Add_U4              , Js::SIMDInt32x4Operation::OpAdd             )
EXDEF2_WMS( SIMD_U8_2toU8_1    , Simd128_Add_U8              , Js::SIMDInt16x8Operation::OpAdd             )
EXDEF2_WMS( SIMD_U16_2toU16_1  , Simd128_Add_U16             , Js::SIMDInt8x16Operation::OpAdd             )

EXDEF2_WMS( SIMD_I8_2toI8_1    , Simd128_Sub_I8              , Js::SIMDInt16x8Operation::OpSub             )
EXDEF2_WMS( SIMD_U4_2toU4_1    , Simd128_Sub_U4              , Js::SIMDInt32x4Operation::OpSub             )
EXDEF2_WMS( SIMD_U8_2toU8_1    , Simd128_Sub_U8              , Js::SIMDInt16x8Operation::OpSub             )
EXDEF2_WMS( SIMD_U16_2toU16_1  , Simd128_Sub_U16             , Js::SIMDInt8x16Operation::OpSub             )

EXDEF2_WMS( SIMD_I8_1toI8_1    , Simd128_Neg_I8              , SIMDInt16x8Operation::OpNeg                 )
EXDEF2_WMS( SIMD_I8_2toI8_1    , Simd128_Mul_I8              , Js::SIMDInt16x8Operation::OpMul             )
EXDEF2_WMS( SIMD_U4_2toU4_1    , Simd128_Mul_U4              , Js::SIMDInt32x4Operation::OpMul             )
EXDEF2_WMS( SIMD_U8_2toU8_1    , Simd128_Mul_U8              , Js::SIMDInt16x8Operation::OpMul             )
EXDEF2_WMS( SIMD_U16_2toU16_1  , Simd128_Mul_U16             , Js::SIMDInt8x16Operation::OpMul             )

EXDEF2_WMS(SIMD_I8_1I1toI8_1   , Simd128_ShLtByScalar_I8     , Js::SIMDInt16x8Operation::OpShiftLeftByScalar  )
EXDEF2_WMS(SIMD_I8_1I1toI8_1   , Simd128_ShRtByScalar_I8     , Js::SIMDInt16x8Operation::OpShiftRightByScalar )

EXDEF2_WMS(SIMD_I16_1I1toI16_1 , Simd128_ShLtByScalar_I16    , Js::SIMDInt8x16Operation::OpShiftLeftByScalar    )
EXDEF2_WMS(SIMD_I16_1I1toI16_1 , Simd128_ShRtByScalar_I16    , Js::SIMDInt8x16Operation::OpShiftRightByScalar   )

EXDEF2_WMS(SIMD_U4_1I1toU4_1   , Simd128_ShLtByScalar_U4     , Js::SIMDInt32x4Operation::OpShiftLeftByScalar  )
EXDEF2_WMS(SIMD_U4_1I1toU4_1   , Simd128_ShRtByScalar_U4     , Js::SIMDUint32x4Operation::OpShiftRightByScalar )

EXDEF2_WMS(SIMD_U8_1I1toU8_1   , Simd128_ShLtByScalar_U8     , Js::SIMDInt16x8Operation::OpShiftLeftByScalar  )
EXDEF2_WMS(SIMD_U8_1I1toU8_1   , Simd128_ShRtByScalar_U8     , Js::SIMDUint16x8Operation::OpShiftRightByScalar )

EXDEF2_WMS(SIMD_U16_1I1toU16_1 , Simd128_ShLtByScalar_U16    , Js::SIMDInt8x16Operation::OpShiftLeftByScalar  )
EXDEF2_WMS(SIMD_U16_1I1toU16_1 , Simd128_ShRtByScalar_U16    , Js::SIMDUint8x16Operation::OpShiftRightByScalar )

EXDEF2_WMS( SIMD_I8_2toI8_1    , Simd128_AddSaturate_I8      , Js::SIMDInt16x8Operation::OpAddSaturate        )
EXDEF2_WMS( SIMD_I8_2toI8_1    , Simd128_SubSaturate_I8      , Js::SIMDInt16x8Operation::OpSubSaturate        )

EXDEF2_WMS( SIMD_I16_2toI16_1  , Simd128_AddSaturate_I16    , Js::SIMDInt8x16Operation::OpAddSaturate           )
EXDEF2_WMS( SIMD_I16_2toI16_1  , Simd128_SubSaturate_I16    , Js::SIMDInt8x16Operation::OpSubSaturate           )

EXDEF2_WMS( SIMD_U8_2toU8_1    , Simd128_AddSaturate_U8      , Js::SIMDUint16x8Operation::OpAddSaturate       )
EXDEF2_WMS( SIMD_U8_2toU8_1    , Simd128_SubSaturate_U8      , Js::SIMDUint16x8Operation::OpSubSaturate       )

EXDEF2_WMS( SIMD_U16_2toU16_1  , Simd128_AddSaturate_U16     , Js::SIMDUint8x16Operation::OpAddSaturate       )
EXDEF2_WMS( SIMD_U16_2toU16_1  , Simd128_SubSaturate_U16     , Js::SIMDUint8x16Operation::OpSubSaturate       )

EXDEF3_WMS( CUSTOM_ASMJS       , Simd128_LdArr_I8            , OP_SimdLdArrGeneric , AsmSimdTypedArr          )
EXDEF3_WMS( CUSTOM_ASMJS       , Simd128_LdArr_I16           , OP_SimdLdArrGeneric , AsmSimdTypedArr            )
EXDEF3_WMS( CUSTOM_ASMJS       , Simd128_LdArr_U4            , OP_SimdLdArrGeneric , AsmSimdTypedArr          )
EXDEF3_WMS( CUSTOM_ASMJS       , Simd128_LdArr_U8            , OP_SimdLdArrGeneric , AsmSimdTypedArr          )
EXDEF3_WMS( CUSTOM_ASMJS       , Simd128_LdArr_U16           , OP_SimdLdArrGeneric , AsmSimdTypedArr          )

EXDEF3_WMS( CUSTOM_ASMJS      , Simd128_StArr_I8              , OP_SimdStArrGeneric , AsmSimdTypedArr         )
EXDEF3_WMS( CUSTOM_ASMJS      , Simd128_StArr_I16             , OP_SimdStArrGeneric , AsmSimdTypedArr           )
EXDEF3_WMS( CUSTOM_ASMJS      , Simd128_StArr_U4              , OP_SimdStArrGeneric , AsmSimdTypedArr         )
EXDEF3_WMS( CUSTOM_ASMJS      , Simd128_StArr_U8              , OP_SimdStArrGeneric , AsmSimdTypedArr         )
EXDEF3_WMS( CUSTOM_ASMJS      , Simd128_StArr_U16             , OP_SimdStArrGeneric , AsmSimdTypedArr         )

EXDEF2_WMS( SIMD_F4_1toI8_1  , Simd128_FromFloat32x4Bits_I8   , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I4_1toI8_1  , Simd128_FromInt32x4Bits_I8     , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I16_1toI8_1 , Simd128_FromInt8x16Bits_I8     , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U4_1toI8_1  , Simd128_FromUint32x4Bits_I8    , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U8_1toI8_1  , Simd128_FromUint16x8Bits_I8    , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U16_1toI8_1 , Simd128_FromUint8x16Bits_I8    , Js::SIMDUtils::FromSimdBits                   )

EXDEF2_WMS( SIMD_F4_1toI16_1 , Simd128_FromFloat32x4Bits_I16  , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I4_1toI16_1 , Simd128_FromInt32x4Bits_I16    , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I8_1toI16_1 , Simd128_FromInt16x8Bits_I16    , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U4_1toI16_1 , Simd128_FromUint32x4Bits_I16   , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U8_1toI16_1 , Simd128_FromUint16x8Bits_I16   , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U16_1toI16_1, Simd128_FromUint8x16Bits_I16   , Js::SIMDUtils::FromSimdBits                   )

EXDEF3_WMS( CUSTOM_ASMJS     , Simd128_FromFloat32x4_U4       , OP_SimdUint32x4FromFloat32x4 , Uint32x4_1Float32x4_1)
EXDEF2_WMS( SIMD_F4_1toU4_1  , Simd128_FromFloat32x4Bits_U4   , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I4_1toU4_1  , Simd128_FromInt32x4Bits_U4     , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I8_1toU4_1  , Simd128_FromInt16x8Bits_U4     , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I16_1toU4_1 , Simd128_FromInt8x16Bits_U4     , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U8_1toU4_1  , Simd128_FromUint16x8Bits_U4    , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U16_1toU4_1 , Simd128_FromUint8x16Bits_U4    , Js::SIMDUtils::FromSimdBits                   )

EXDEF2_WMS( SIMD_F4_1toU8_1  , Simd128_FromFloat32x4Bits_U8   , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I4_1toU8_1  , Simd128_FromInt32x4Bits_U8     , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I8_1toU8_1  , Simd128_FromInt16x8Bits_U8     , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I16_1toU8_1 , Simd128_FromInt8x16Bits_U8     , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U4_1toU8_1  , Simd128_FromUint32x4Bits_U8    , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U16_1toU8_1 , Simd128_FromUint8x16Bits_U8    , Js::SIMDUtils::FromSimdBits                   )

EXDEF2_WMS( SIMD_F4_1toU16_1  , Simd128_FromFloat32x4Bits_U16 , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I4_1toU16_1  , Simd128_FromInt32x4Bits_U16   , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I8_1toU16_1  , Simd128_FromInt16x8Bits_U16   , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_I16_1toU16_1 , Simd128_FromInt8x16Bits_U16   , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U4_1toU16_1  , Simd128_FromUint32x4Bits_U16  , Js::SIMDUtils::FromSimdBits                   )
EXDEF2_WMS( SIMD_U8_1toU16_1  , Simd128_FromUint16x8Bits_U16  , Js::SIMDUtils::FromSimdBits                   )

EXDEF2_WMS( SIMD_I8_2toB8_1  , Simd128_Lt_I8               , Js::SIMDInt16x8Operation::OpLessThan             )
EXDEF2_WMS( SIMD_U4_2toB4_1  , Simd128_Lt_U4               , Js::SIMDUint32x4Operation::OpLessThan            )
EXDEF2_WMS( SIMD_U8_2toB8_1  , Simd128_Lt_U8               , Js::SIMDUint16x8Operation::OpLessThan            )
EXDEF2_WMS( SIMD_U16_2toB16_1 , Simd128_Lt_U16             , Js::SIMDUint8x16Operation::OpLessThan            )

EXDEF2_WMS( SIMD_I4_2toB4_1   , Simd128_LtEq_I4             , Js::SIMDInt32x4Operation::OpLessThanOrEqual     )
EXDEF2_WMS( SIMD_I8_2toB8_1   , Simd128_LtEq_I8             , Js::SIMDInt16x8Operation::OpLessThanOrEqual     )
EXDEF2_WMS( SIMD_I16_2toB16_1   , Simd128_LtEq_I16            , Js::SIMDInt8x16Operation::OpLessThanOrEqual     )
EXDEF2_WMS( SIMD_U4_2toB4_1   , Simd128_LtEq_U4             , Js::SIMDUint32x4Operation::OpLessThanOrEqual    )
EXDEF2_WMS( SIMD_U8_2toB8_1   , Simd128_LtEq_U8             , Js::SIMDUint16x8Operation::OpLessThanOrEqual    )
EXDEF2_WMS( SIMD_U16_2toB16_1 , Simd128_LtEq_U16            , Js::SIMDUint8x16Operation::OpLessThanOrEqual    )

EXDEF2_WMS( SIMD_I8_2toB8_1   , Simd128_Eq_I8               , Js::SIMDInt16x8Operation::OpEqual               )
EXDEF2_WMS( SIMD_U4_2toB4_1   , Simd128_Eq_U4               , Js::SIMDInt32x4Operation::OpEqual               )
EXDEF2_WMS( SIMD_U8_2toB8_1   , Simd128_Eq_U8               , Js::SIMDInt16x8Operation::OpEqual               )
EXDEF2_WMS( SIMD_U16_2toB16_1 , Simd128_Eq_U16              , Js::SIMDInt8x16Operation::OpEqual               )

EXDEF2_WMS( SIMD_I4_2toB4_1   , Simd128_Neq_I4              , Js::SIMDInt32x4Operation::OpNotEqual            )
EXDEF2_WMS( SIMD_I8_2toB8_1   , Simd128_Neq_I8              , Js::SIMDInt16x8Operation::OpNotEqual            )
EXDEF2_WMS( SIMD_I16_2toB16_1   , Simd128_Neq_I16             , Js::SIMDInt8x16Operation::OpNotEqual            )
EXDEF2_WMS( SIMD_U4_2toB4_1   , Simd128_Neq_U4              , Js::SIMDInt32x4Operation::OpNotEqual            )
EXDEF2_WMS( SIMD_U8_2toB8_1   , Simd128_Neq_U8              , Js::SIMDInt16x8Operation::OpNotEqual            )
EXDEF2_WMS( SIMD_U16_2toB16_1 , Simd128_Neq_U16             , Js::SIMDInt8x16Operation::OpNotEqual            )

EXDEF2_WMS( SIMD_I4_2toB4_1   , Simd128_GtEq_I4             , Js::SIMDInt32x4Operation::OpGreaterThanOrEqual  )
EXDEF2_WMS( SIMD_I8_2toB8_1   , Simd128_GtEq_I8             , Js::SIMDInt16x8Operation::OpGreaterThanOrEqual  )
EXDEF2_WMS( SIMD_I16_2toB16_1   , Simd128_GtEq_I16            , Js::SIMDInt8x16Operation::OpGreaterThanOrEqual  )
EXDEF2_WMS( SIMD_U4_2toB4_1   , Simd128_GtEq_U4             , Js::SIMDUint32x4Operation::OpGreaterThanOrEqual )
EXDEF2_WMS( SIMD_U8_2toB8_1   , Simd128_GtEq_U8             , Js::SIMDUint16x8Operation::OpGreaterThanOrEqual )
EXDEF2_WMS( SIMD_U16_2toB16_1 , Simd128_GtEq_U16            , Js::SIMDUint8x16Operation::OpGreaterThanOrEqual )

EXDEF2_WMS( SIMD_I8_2toB8_1   , Simd128_Gt_I8               , Js::SIMDInt16x8Operation::OpGreaterThan         )
EXDEF2_WMS( SIMD_U4_2toB4_1   , Simd128_Gt_U4               , Js::SIMDUint32x4Operation::OpGreaterThan        )
EXDEF2_WMS( SIMD_U8_2toB8_1   , Simd128_Gt_U8               , Js::SIMDUint16x8Operation::OpGreaterThan        )
EXDEF2_WMS( SIMD_U16_2toB16_1 , Simd128_Gt_U16              , Js::SIMDUint8x16Operation::OpGreaterThan        )

EXDEF2_WMS( SIMD_U4_1toU4_1     , Simd128_Neg_U4              , SIMDInt32x4Operation::OpNeg                     )
EXDEF2_WMS( SIMD_U8_1toU8_1     , Simd128_Neg_U8              , SIMDInt16x8Operation::OpNeg                     )
EXDEF2_WMS( SIMD_U16_1toU16_1   , Simd128_Neg_U16             , SIMDInt8x16Operation::OpNeg                     )

#if 0
EXDEF2_WMS(SIMD_D2toD2_1, Simd128_DoublesToD2, SIMDFloat64x2Operation::OpFloat64x2)
EXDEF4_WMS(TEMPLATE_ASMJS, Simd128_LdSlot_D2, OP_LdSlotPrimitive, ElementSlot, AsmJsSIMDValue)
EXDEF4_WMS(TEMPLATE_ASMJS, Simd128_StSlot_D2, OP_StSlotPrimitive, ElementSlot, AsmJsSIMDValue)
EXDEF2_WMS(SIMD_D2_1toD2_1, Simd128_Return_D2, (AsmJsSIMDValue))
EXDEF2_WMS(SIMD_D1toD2_1, Simd128_Splat_D2, Js::SIMDFloat64x2Operation::OpSplat)
EXDEF2_WMS(SIMD_F4_1toD2_1, Simd128_FromFloat32x4_D2, SIMDFloat64x2Operation::OpFromFloat32x4)
EXDEF2_WMS(SIMD_F4_1toD2_1, Simd128_FromFloat32x4Bits_D2, Js::FromSimdBits)
EXDEF2_WMS(SIMD_I4_1toD2_1, Simd128_FromInt32x4_D2, SIMDFloat64x2Operation::OpFromInt32x4)
EXDEF2_WMS(SIMD_I4_1toD2_1, Simd128_FromInt32x4Bits_D2, Js::FromSimdBits)
EXDEF2_WMS(SIMD_D2_1toD2_1, Simd128_Abs_D2, SIMDFloat64x2Operation::OpAbs)
EXDEF2_WMS(SIMD_D2_1toD2_1, Simd128_Neg_D2, SIMDFloat64x2Operation::OpNeg)
EXDEF2_WMS(SIMD_D2_1toD2_1, Simd128_Rcp_D2, SIMDFloat64x2Operation::OpReciprocal)
EXDEF2_WMS(SIMD_D2_1toD2_1, Simd128_RcpSqrt_D2, SIMDFloat64x2Operation::OpReciprocalSqrt)
EXDEF2_WMS(SIMD_D2_1toD2_1, Simd128_Sqrt_D2, SIMDFloat64x2Operation::OpSqrt)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Add_D2, Js::SIMDFloat64x2Operation::OpAdd)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Sub_D2, Js::SIMDFloat64x2Operation::OpSub)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Mul_D2, Js::SIMDFloat64x2Operation::OpMul)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Div_D2, Js::SIMDFloat64x2Operation::OpDiv)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Min_D2, Js::SIMDFloat64x2Operation::OpMin)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Max_D2, Js::SIMDFloat64x2Operation::OpMax)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Lt_D2, Js::SIMDFloat64x2Operation::OpLessThan)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_LtEq_D2, Js::SIMDFloat64x2Operation::OpLessThanOrEqual)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Eq_D2, Js::SIMDFloat64x2Operation::OpEqual)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Neq_D2, Js::SIMDFloat64x2Operation::OpNotEqual)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_GtEq_D2, Js::SIMDFloat64x2Operation::OpGreaterThanOrEqual)
EXDEF2_WMS(SIMD_D2_2toD2_1, Simd128_Gt_D2, Js::SIMDFloat64x2Operation::OpGreaterThan)
EXDEF2_WMS(SIMD_I4_1D2_2toD2_1, Simd128_Select_D2, Js::SIMDFloat64x2Operation::OpSelect)
EXDEF2_WMS(SIMD_D2_3toD2_1, Simd128_Clamp_D2, Js::SIMDFloat64x2Operation::OpClamp)
EXDEF2_WMS(SIMD_D2_1toI1, Simd128_LdSignMask_D2, Js::SIMDFloat64x2Operation::OpGetSignMask)
EXDEF2_WMS(SIMD_D2_1toR1Mem, Simd128_I_ArgOut_D2, OP_I_SetOutAsmSimd)
EXDEF2_WMS(SIMD_D2_1toD2_1, Simd128_I_Conv_VTD2, (AsmJsSIMDValue))
EXDEF2_WMS(SIMD_D2_1toD2_1, Simd128_Ld_D2, (AsmJsSIMDValue)
EXDEF2_WMS(SIMD_D2_1I2toD2_1, Simd128_Swizzle_D2, SIMD128InnerShuffle)
EXDE2_WMS(SIMD_D2_2I2toD2_1, Simd128_Shuffle_D2, SIMD128InnerShuffle)
EXDEF3_WMS(CUSTOM_ASMJS, Simd128_StArr_D2, OP_SimdStArrGeneric, AsmSimdTypedArr)
EXDEF3_WMS(CUSTOM_ASMJS, Simd128_LdArr_D2, OP_SimdLdArrGeneric, AsmSimdTypedArr)
EXDEF3_WMS(CUSTOM_ASMJS, Simd128_LdArrConst_D2, OP_SimdLdArrConstIndex, AsmSimdTypedArr)
EXDEF3_WMS(CUSTOM_ASMJS, Simd128_StArrConst_D2, OP_SimdStArrConstIndex, AsmSimdTypedArr)
#endif // 0

#endif
