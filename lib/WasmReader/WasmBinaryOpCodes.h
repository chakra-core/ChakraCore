//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// define this to include all opcodes
#ifndef WASM_OPCODE
#define WASM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_SIGNATURE
#define WASM_SIGNATURE(id,...)
#endif

#ifndef WASM_CTRL_OPCODE
#define WASM_CTRL_OPCODE(opname, opcode, sig, nyi) WASM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_MISC_OPCODE
#define WASM_MISC_OPCODE(opname, opcode, sig, nyi) WASM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_MEM_OPCODE
#define WASM_MEM_OPCODE(opname, opcode, sig, nyi) WASM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_MEMREAD_OPCODE
#define WASM_MEMREAD_OPCODE(opname, opcode, sig, nyi, viewtype) WASM_MEM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_MEMSTORE_OPCODE
#define WASM_MEMSTORE_OPCODE(opname, opcode, sig, nyi, viewtype) WASM_MEM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_UNARY__OPCODE
#define WASM_UNARY__OPCODE(opname, opcode, sig, asmjop, nyi) WASM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_BINARY_OPCODE
#define WASM_BINARY_OPCODE(opname, opcode, sig, asmjop, nyi) WASM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_EMPTY__OPCODE
#define WASM_EMPTY__OPCODE(opname, opcode, asmjop, nyi) WASM_OPCODE(opname, opcode, Limit, nyi)
#endif

// built-in opcode signatures
//              id, retType, arg0, arg1, arg2
WASM_SIGNATURE(I_II,    3,   WasmTypes::I32, WasmTypes::I32, WasmTypes::I32)
WASM_SIGNATURE(I_I,     2,   WasmTypes::I32, WasmTypes::I32)
WASM_SIGNATURE(I_FF,    3,   WasmTypes::I32, WasmTypes::F32, WasmTypes::F32)
WASM_SIGNATURE(I_F,     2,   WasmTypes::I32, WasmTypes::F32)
WASM_SIGNATURE(I_DD,    3,   WasmTypes::I32, WasmTypes::F64, WasmTypes::F64)
WASM_SIGNATURE(I_D,     2,   WasmTypes::I32, WasmTypes::F64)
WASM_SIGNATURE(I_L,     2,   WasmTypes::I32, WasmTypes::I64)
WASM_SIGNATURE(L_LL,    3,   WasmTypes::I64, WasmTypes::I64, WasmTypes::I64)
WASM_SIGNATURE(I_LL,    3,   WasmTypes::I32, WasmTypes::I64, WasmTypes::I64)
WASM_SIGNATURE(L_L,     2,   WasmTypes::I64, WasmTypes::I64)
WASM_SIGNATURE(L_I,     2,   WasmTypes::I64, WasmTypes::I32)
WASM_SIGNATURE(L_F,     2,   WasmTypes::I64, WasmTypes::F32)
WASM_SIGNATURE(L_D,     2,   WasmTypes::I64, WasmTypes::F64)
WASM_SIGNATURE(F_FF,    3,   WasmTypes::F32, WasmTypes::F32, WasmTypes::F32)
WASM_SIGNATURE(F_F,     2,   WasmTypes::F32, WasmTypes::F32)
WASM_SIGNATURE(F_D,     2,   WasmTypes::F32, WasmTypes::F64)
WASM_SIGNATURE(F_I,     2,   WasmTypes::F32, WasmTypes::I32)
WASM_SIGNATURE(F_L,     2,   WasmTypes::F32, WasmTypes::I64)
WASM_SIGNATURE(D_DD,    3,   WasmTypes::F64, WasmTypes::F64, WasmTypes::F64)
WASM_SIGNATURE(D_D,     2,   WasmTypes::F64, WasmTypes::F64)
WASM_SIGNATURE(D_F,     2,   WasmTypes::F64, WasmTypes::F32)
WASM_SIGNATURE(D_I,     2,   WasmTypes::F64, WasmTypes::I32)
WASM_SIGNATURE(D_L,     2,   WasmTypes::F64, WasmTypes::I64)
WASM_SIGNATURE(D_ID,    3,   WasmTypes::F64, WasmTypes::I32, WasmTypes::F64)
WASM_SIGNATURE(F_IF,    3,   WasmTypes::F32, WasmTypes::I32, WasmTypes::F32)
WASM_SIGNATURE(L_IL,    3,   WasmTypes::I64, WasmTypes::I32, WasmTypes::I64)

WASM_SIGNATURE(V_I,     2,   WasmTypes::Void, WasmTypes::I32)
WASM_SIGNATURE(V_L,     2,   WasmTypes::Void, WasmTypes::I64)
WASM_SIGNATURE(V_F,     2,   WasmTypes::Void, WasmTypes::F32)
WASM_SIGNATURE(V_D,     2,   WasmTypes::Void, WasmTypes::I64)

WASM_SIGNATURE(I,       1,   WasmTypes::I32)
WASM_SIGNATURE(L,       1,   WasmTypes::I64)
WASM_SIGNATURE(F,       1,   WasmTypes::F32)
WASM_SIGNATURE(D,       1,   WasmTypes::I64)
WASM_SIGNATURE(V,       1,   WasmTypes::Void)
WASM_SIGNATURE(Limit,   1,   WasmTypes::Void)

// Control flow operators
WASM_CTRL_OPCODE(Unreachable, 0x00, Limit, false)
WASM_CTRL_OPCODE(Nop,         0x01, Limit, false)
WASM_CTRL_OPCODE(Block,       0x02, Limit, false)
WASM_CTRL_OPCODE(Loop,        0x03, Limit, false)
WASM_CTRL_OPCODE(If,          0x04, Limit, false)
WASM_CTRL_OPCODE(Else,        0x05, Limit, false)
WASM_CTRL_OPCODE(End,         0x0b, Limit, false)
WASM_CTRL_OPCODE(Br,          0x0c, Limit, false)
WASM_CTRL_OPCODE(BrIf,        0x0d, Limit, false)
WASM_CTRL_OPCODE(BrTable,     0x0e, Limit, false)
WASM_CTRL_OPCODE(Return,      0x0f, Limit, false)

// Call operators
WASM_MISC_OPCODE(Call,        0x10, Limit, false)
WASM_MISC_OPCODE(CallIndirect,0x11, Limit, false)

// Parametric operators
WASM_CTRL_OPCODE(Drop,        0x1a, Limit, false)
WASM_CTRL_OPCODE(Select,      0x1b, Limit, false)

// Variable access
WASM_MISC_OPCODE(GetLocal,     0x20, Limit, false)
WASM_MISC_OPCODE(SetLocal,     0x21, Limit, false)
WASM_MISC_OPCODE(TeeLocal,     0x22, Limit, false)
WASM_MISC_OPCODE(GetGlobal,    0x23, Limit, false)
WASM_MISC_OPCODE(SetGlobal,    0x24, Limit, false)

// Memory-related operators
// Load memory expressions.
WASM_MEMREAD_OPCODE(I32LoadMem,    0x28, I_I, false, Js::ArrayBufferView::TYPE_INT32)
WASM_MEMREAD_OPCODE(I64LoadMem,    0x29, L_I, false, Js::ArrayBufferView::TYPE_INT64)
WASM_MEMREAD_OPCODE(F32LoadMem,    0x2a, F_I, false, Js::ArrayBufferView::TYPE_FLOAT32)
WASM_MEMREAD_OPCODE(F64LoadMem,    0x2b, D_I, false, Js::ArrayBufferView::TYPE_FLOAT64)
WASM_MEMREAD_OPCODE(I32LoadMem8S,  0x2c, I_I, false, Js::ArrayBufferView::TYPE_INT8)
WASM_MEMREAD_OPCODE(I32LoadMem8U,  0x2d, I_I, false, Js::ArrayBufferView::TYPE_UINT8)
WASM_MEMREAD_OPCODE(I32LoadMem16S, 0x2e, I_I, false, Js::ArrayBufferView::TYPE_INT16)
WASM_MEMREAD_OPCODE(I32LoadMem16U, 0x2f, I_I, false, Js::ArrayBufferView::TYPE_UINT16)
WASM_MEMREAD_OPCODE(I64LoadMem8S,  0x30, L_I, false, Js::ArrayBufferView::TYPE_INT8_TO_INT64)
WASM_MEMREAD_OPCODE(I64LoadMem8U,  0x31, L_I, false, Js::ArrayBufferView::TYPE_UINT8_TO_INT64)
WASM_MEMREAD_OPCODE(I64LoadMem16S, 0x32, L_I, false, Js::ArrayBufferView::TYPE_INT16_TO_INT64)
WASM_MEMREAD_OPCODE(I64LoadMem16U, 0x33, L_I, false, Js::ArrayBufferView::TYPE_UINT16_TO_INT64)
WASM_MEMREAD_OPCODE(I64LoadMem32S, 0x34, L_I, false, Js::ArrayBufferView::TYPE_INT32_TO_INT64)
WASM_MEMREAD_OPCODE(I64LoadMem32U, 0x35, L_I, false, Js::ArrayBufferView::TYPE_UINT32_TO_INT64)

// Store memory expressions.
WASM_MEMSTORE_OPCODE(I32StoreMem,   0x36, I_II, false, Js::ArrayBufferView::TYPE_INT32)
WASM_MEMSTORE_OPCODE(I64StoreMem,   0x37, L_IL, false, Js::ArrayBufferView::TYPE_INT64)
WASM_MEMSTORE_OPCODE(F32StoreMem,   0x38, F_IF, false, Js::ArrayBufferView::TYPE_FLOAT32)
WASM_MEMSTORE_OPCODE(F64StoreMem,   0x39, D_ID, false, Js::ArrayBufferView::TYPE_FLOAT64)
WASM_MEMSTORE_OPCODE(I32StoreMem8,  0x3a, I_II, false, Js::ArrayBufferView::TYPE_INT8)
WASM_MEMSTORE_OPCODE(I32StoreMem16, 0x3b, I_II, false, Js::ArrayBufferView::TYPE_INT16)
WASM_MEMSTORE_OPCODE(I64StoreMem8,  0x3c, L_IL, false, Js::ArrayBufferView::TYPE_INT8_TO_INT64)
WASM_MEMSTORE_OPCODE(I64StoreMem16, 0x3d, L_IL, false, Js::ArrayBufferView::TYPE_INT16_TO_INT64)
WASM_MEMSTORE_OPCODE(I64StoreMem32, 0x3e, L_IL, false, Js::ArrayBufferView::TYPE_INT32_TO_INT64)

WASM_MISC_OPCODE(CurrentMemory, 0x3f, I_I, false)
WASM_MISC_OPCODE(GrowMemory,    0x40, I_I, false)

// Constants
WASM_MISC_OPCODE(I32Const,     0x41, Limit, false)
WASM_MISC_OPCODE(I64Const,     0x42, Limit, false)
WASM_MISC_OPCODE(F32Const,     0x43, Limit, false)
WASM_MISC_OPCODE(F64Const,     0x44, Limit, false)

////////////////////////////////////////////////////////////
// Comparison operators
WASM_UNARY__OPCODE(I32Eqz,            0x45, I_I , Eqz_Int        , false)
WASM_BINARY_OPCODE(I32Eq,             0x46, I_II, CmEq_Int       , false)
WASM_BINARY_OPCODE(I32Ne,             0x47, I_II, CmNe_Int       , false)
WASM_BINARY_OPCODE(I32LtS,            0x48, I_II, CmLt_Int       , false)
WASM_BINARY_OPCODE(I32LtU,            0x49, I_II, CmLt_UInt      , false)
WASM_BINARY_OPCODE(I32GtS,            0x4a, I_II, CmGt_Int       , false)
WASM_BINARY_OPCODE(I32GtU,            0x4b, I_II, CmGt_UInt      , false)
WASM_BINARY_OPCODE(I32LeS,            0x4c, I_II, CmLe_Int       , false)
WASM_BINARY_OPCODE(I32LeU,            0x4d, I_II, CmLe_UInt      , false)
WASM_BINARY_OPCODE(I32GeS,            0x4e, I_II, CmGe_Int       , false)
WASM_BINARY_OPCODE(I32GeU,            0x4f, I_II, CmGe_UInt      , false)

WASM_UNARY__OPCODE(I64Eqz,            0x50, I_L , Eqz_Long       , false)
WASM_BINARY_OPCODE(I64Eq,             0x51, I_LL, CmEq_Long      , false)
WASM_BINARY_OPCODE(I64Ne,             0x52, I_LL, CmNe_Long      , false)
WASM_BINARY_OPCODE(I64LtS,            0x53, I_LL, CmLt_Long      , false)
WASM_BINARY_OPCODE(I64LtU,            0x54, I_LL, CmLt_ULong     , false)
WASM_BINARY_OPCODE(I64GtS,            0x55, I_LL, CmGt_Long      , false)
WASM_BINARY_OPCODE(I64GtU,            0x56, I_LL, CmGt_ULong     , false)
WASM_BINARY_OPCODE(I64LeS,            0x57, I_LL, CmLe_Long      , false)
WASM_BINARY_OPCODE(I64LeU,            0x58, I_LL, CmLe_ULong     , false)
WASM_BINARY_OPCODE(I64GeS,            0x59, I_LL, CmGe_Long      , false)
WASM_BINARY_OPCODE(I64GeU,            0x5a, I_LL, CmGe_ULong     , false)

WASM_BINARY_OPCODE(F32Eq,             0x5b, I_FF, CmEq_Flt       , false)
WASM_BINARY_OPCODE(F32Ne,             0x5c, I_FF, CmNe_Flt       , false)
WASM_BINARY_OPCODE(F32Lt,             0x5d, I_FF, CmLt_Flt       , false)
WASM_BINARY_OPCODE(F32Le,             0x5f, I_FF, CmLe_Flt       , false)
WASM_BINARY_OPCODE(F32Gt,             0x5e, I_FF, CmGt_Flt       , false)
WASM_BINARY_OPCODE(F32Ge,             0x60, I_FF, CmGe_Flt       , false)

WASM_BINARY_OPCODE(F64Eq,             0x61, I_DD, CmEq_Db        , false)
WASM_BINARY_OPCODE(F64Ne,             0x62, I_DD, CmNe_Db        , false)
WASM_BINARY_OPCODE(F64Lt,             0x63, I_DD, CmLt_Db        , false)
WASM_BINARY_OPCODE(F64Le,             0x65, I_DD, CmLe_Db        , false)
WASM_BINARY_OPCODE(F64Gt,             0x64, I_DD, CmGt_Db        , false)
WASM_BINARY_OPCODE(F64Ge,             0x66, I_DD, CmGe_Db        , false)
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Numeric operators
WASM_UNARY__OPCODE(I32Clz,            0x67, I_I , Clz32_Int      , false)
WASM_UNARY__OPCODE(I32Ctz,            0x68, I_I , Ctz_Int        , false)
WASM_UNARY__OPCODE(I32Popcnt,         0x69, I_I , PopCnt_Int     , false)
WASM_BINARY_OPCODE(I32Add,            0x6a, I_II, Add_Int        , false)
WASM_BINARY_OPCODE(I32Sub,            0x6b, I_II, Sub_Int        , false)
WASM_BINARY_OPCODE(I32Mul,            0x6c, I_II, Mul_Int        , false)
WASM_BINARY_OPCODE(I32DivS,           0x6d, I_II, Div_Trap_Int   , false)
WASM_BINARY_OPCODE(I32DivU,           0x6e, I_II, Div_Trap_UInt  , false)
WASM_BINARY_OPCODE(I32RemS,           0x6f, I_II, Rem_Trap_Int   , false)
WASM_BINARY_OPCODE(I32RemU,           0x70, I_II, Rem_Trap_UInt  , false)
WASM_BINARY_OPCODE(I32And,            0x71, I_II, And_Int        , false)
WASM_BINARY_OPCODE(I32Or,             0x72, I_II, Or_Int         , false)
WASM_BINARY_OPCODE(I32Xor,            0x73, I_II, Xor_Int        , false)
WASM_BINARY_OPCODE(I32Shl,            0x74, I_II, Shl_Int        , false)
WASM_BINARY_OPCODE(I32ShrS,           0x75, I_II, Shr_Int        , false)
WASM_BINARY_OPCODE(I32ShrU,           0x76, I_II, Shr_UInt       , false)
WASM_BINARY_OPCODE(I32Rol,            0x77, I_II, Rol_Int        , false)
WASM_BINARY_OPCODE(I32Ror,            0x78, I_II, Ror_Int        , false)

WASM_UNARY__OPCODE(I64Clz,            0x79, L_L , Clz_Long       , false)
WASM_UNARY__OPCODE(I64Ctz,            0x7a, L_L , Ctz_Long       , false)
WASM_UNARY__OPCODE(I64Popcnt,         0x7b, L_L , PopCnt_Long    , false)
WASM_BINARY_OPCODE(I64Add,            0x7c, L_LL, Add_Long       , false)
WASM_BINARY_OPCODE(I64Sub,            0x7d, L_LL, Sub_Long       , false)
WASM_BINARY_OPCODE(I64Mul,            0x7e, L_LL, Mul_Long       , false)
WASM_BINARY_OPCODE(I64DivS,           0x7f, L_LL, Div_Trap_Long  , false)
WASM_BINARY_OPCODE(I64DivU,           0x80, L_LL, Div_Trap_ULong , false)
WASM_BINARY_OPCODE(I64RemS,           0x81, L_LL, Rem_Trap_Long  , false)
WASM_BINARY_OPCODE(I64RemU,           0x82, L_LL, Rem_Trap_ULong , false)
WASM_BINARY_OPCODE(I64And,            0x83, L_LL, And_Long       , false)
WASM_BINARY_OPCODE(I64Or,             0x84, L_LL, Or_Long        , false)
WASM_BINARY_OPCODE(I64Xor,            0x85, L_LL, Xor_Long       , false)
WASM_BINARY_OPCODE(I64Shl,            0x86, L_LL, Shl_Long       , false)
WASM_BINARY_OPCODE(I64ShrS,           0x87, L_LL, Shr_Long       , false)
WASM_BINARY_OPCODE(I64ShrU,           0x88, L_LL, Shr_ULong      , false)
WASM_BINARY_OPCODE(I64Rol,            0x89, L_LL, Rol_Long       , false)
WASM_BINARY_OPCODE(I64Ror,            0x8a, L_LL, Ror_Long       , false)

WASM_UNARY__OPCODE(F32Abs,            0x8b, F_F , Abs_Flt        , false)
WASM_UNARY__OPCODE(F32Neg,            0x8c, F_F , Neg_Flt        , false)
WASM_UNARY__OPCODE(F32Ceil,           0x8d, F_F , Ceil_Flt       , false)
WASM_UNARY__OPCODE(F32Floor,          0x8e, F_F , Floor_Flt      , false)
WASM_UNARY__OPCODE(F32Trunc,          0x8f, F_F , Trunc_Flt      , false)
WASM_UNARY__OPCODE(F32NearestInt,     0x90, F_F , Nearest_Flt    , false)
WASM_UNARY__OPCODE(F32Sqrt,           0x91, F_F , Sqrt_Flt       , false)
WASM_BINARY_OPCODE(F32Add,            0x92, F_FF, Add_Flt        , false)
WASM_BINARY_OPCODE(F32Sub,            0x93, F_FF, Sub_Flt        , false)
WASM_BINARY_OPCODE(F32Mul,            0x94, F_FF, Mul_Flt        , false)
WASM_BINARY_OPCODE(F32Div,            0x95, F_FF, Div_Flt        , false)
WASM_BINARY_OPCODE(F32Min,            0x96, F_FF, Min_Flt        , false)
WASM_BINARY_OPCODE(F32Max,            0x97, F_FF, Max_Flt        , false)
WASM_BINARY_OPCODE(F32CopySign,       0x98, F_FF, Copysign_Flt   , false)

WASM_UNARY__OPCODE(F64Abs,            0x99, D_D , Abs_Db         , false)
WASM_UNARY__OPCODE(F64Neg,            0x9a, D_D , Neg_Db         , false)
WASM_UNARY__OPCODE(F64Ceil,           0x9b, D_D , Ceil_Db        , false)
WASM_UNARY__OPCODE(F64Floor,          0x9c, D_D , Floor_Db       , false)
WASM_UNARY__OPCODE(F64Trunc,          0x9d, D_D , Trunc_Db       , false)
WASM_UNARY__OPCODE(F64NearestInt,     0x9e, D_D , Nearest_Db     , false)
WASM_UNARY__OPCODE(F64Sqrt,           0x9f, D_D , Sqrt_Db        , false)
WASM_BINARY_OPCODE(F64Add,            0xa0, D_DD, Add_Db         , false)
WASM_BINARY_OPCODE(F64Sub,            0xa1, D_DD, Sub_Db         , false)
WASM_BINARY_OPCODE(F64Mul,            0xa2, D_DD, Mul_Db         , false)
WASM_BINARY_OPCODE(F64Div,            0xa3, D_DD, Div_Db         , false)
WASM_BINARY_OPCODE(F64Min,            0xa4, D_DD, Min_Db         , false)
WASM_BINARY_OPCODE(F64Max,            0xa5, D_DD, Max_Db         , false)
WASM_BINARY_OPCODE(F64CopySign,       0xa6, D_DD, Copysign_Db    , false)
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Conversions
WASM_UNARY__OPCODE(I32Wrap_I64,       0xa7, I_L , Conv_LTI       , false)
WASM_UNARY__OPCODE(I32TruncS_F32,     0xa8, I_F , Conv_Check_FTI , false)
WASM_UNARY__OPCODE(I32TruncU_F32,     0xa9, I_F , Conv_Check_FTU , false)
WASM_UNARY__OPCODE(I32TruncS_F64,     0xaa, I_D , Conv_Check_DTI , false)
WASM_UNARY__OPCODE(I32TruncU_F64,     0xab, I_D , Conv_Check_DTU , false)

WASM_UNARY__OPCODE(I64ExtendS_I32,    0xac, L_I , Conv_ITL       , false)
WASM_UNARY__OPCODE(I64ExtendU_I32,    0xad, L_I , Conv_UTL       , false)
WASM_UNARY__OPCODE(I64TruncS_F32,     0xae, L_F , Conv_Check_FTL , false)
WASM_UNARY__OPCODE(I64TruncU_F32,     0xaf, L_F , Conv_Check_FTUL, false)
WASM_UNARY__OPCODE(I64TruncS_F64,     0xb0, L_D , Conv_Check_DTL , false)
WASM_UNARY__OPCODE(I64TruncU_F64,     0xb1, L_D , Conv_Check_DTUL, false)

WASM_UNARY__OPCODE(F32SConvertI32,    0xb2, F_I , Fround_Int     , false)
WASM_UNARY__OPCODE(F32UConvertI32,    0xb3, F_I , Conv_UTF       , false)
WASM_UNARY__OPCODE(F32SConvertI64,    0xb4, F_L , Conv_LTF       , false)
WASM_UNARY__OPCODE(F32UConvertI64,    0xb5, F_L , Conv_ULTF      , false)
WASM_UNARY__OPCODE(F32DemoteF64,      0xb6, F_D , Fround_Db      , false)

WASM_UNARY__OPCODE(F64SConvertI32,    0xb7, D_I , Conv_ITD       , false)
WASM_UNARY__OPCODE(F64UConvertI32,    0xb8, D_I , Conv_UTD       , false)
WASM_UNARY__OPCODE(F64SConvertI64,    0xb9, D_L , Conv_LTD       , false)
WASM_UNARY__OPCODE(F64UConvertI64,    0xba, D_L , Conv_ULTD      , false)
WASM_UNARY__OPCODE(F64PromoteF32,     0xbb, D_F , Conv_FTD       , false)
////////////////////////////////////////////////////////////

// Reinterpretations
WASM_UNARY__OPCODE(I32ReinterpretF32, 0xbc, I_F , Reinterpret_FTI, false)
WASM_UNARY__OPCODE(I64ReinterpretF64, 0xbd, L_D , Reinterpret_DTL, false)
WASM_UNARY__OPCODE(F32ReinterpretI32, 0xbe, F_I , Reinterpret_ITF, false)
WASM_UNARY__OPCODE(F64ReinterpretI64, 0xbf, D_L , Reinterpret_LTD, false)

#if ENABLE_DEBUG_CONFIG_OPTIONS
WASM_UNARY__OPCODE(PrintFuncName    , 0xf0, V_I , PrintFuncName    , false)
WASM_EMPTY__OPCODE(PrintArgSeparator, 0xf1,       PrintArgSeparator, false)
WASM_EMPTY__OPCODE(PrintBeginCall   , 0xf2,       PrintBeginCall   , false)
WASM_EMPTY__OPCODE(PrintNewLine     , 0xf3,       PrintNewLine     , false)
WASM_UNARY__OPCODE(PrintEndCall     , 0xf4, V_I , PrintEndCall     , false)
WASM_UNARY__OPCODE(PrintI32         , 0xfc, I_I , PrintI32         , false)
WASM_UNARY__OPCODE(PrintI64         , 0xfd, L_L , PrintI64         , false)
WASM_UNARY__OPCODE(PrintF32         , 0xfe, F_F , PrintF32         , false)
WASM_UNARY__OPCODE(PrintF64         , 0xff, D_D , PrintF64         , false)
#endif

#undef WASM_OPCODE
#undef WASM_SIGNATURE
#undef WASM_CTRL_OPCODE
#undef WASM_MISC_OPCODE
#undef WASM_MEM_OPCODE
#undef WASM_MEMREAD_OPCODE
#undef WASM_MEMSTORE_OPCODE
#undef WASM_UNARY__OPCODE
#undef WASM_BINARY_OPCODE
#undef WASM_EMPTY__OPCODE