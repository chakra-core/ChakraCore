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
#define WASM_MEMREAD_OPCODE(opname, opcode, sig, nyi) WASM_MEM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_MEMSTORE_OPCODE
#define WASM_MEMSTORE_OPCODE(opname, opcode, sig, nyi) WASM_MEM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_UNARY__OPCODE
#define WASM_UNARY__OPCODE(opname, opcode, sig, asmjop, nyi) WASM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_BINARY_OPCODE
#define WASM_BINARY_OPCODE(opname, opcode, sig, asmjop, nyi) WASM_OPCODE(opname, opcode, sig, nyi)
#endif

// built-in opcode signatures
//              id, retType, arg0, arg1, arg2
WASM_SIGNATURE(I_II,    3,   WasmTypes::I32, WasmTypes::I32, WasmTypes::I32)
WASM_SIGNATURE(I_I,     2,   WasmTypes::I32, WasmTypes::I32)
WASM_SIGNATURE(I_V,     1,   WasmTypes::I32)
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

//               OpCode Name,    Encoding,    Signature, NYI
WASM_CTRL_OPCODE(Nop,            0x00,        Limit, false)
WASM_CTRL_OPCODE(Block,          0x01,        Limit, false)
WASM_CTRL_OPCODE(Loop,           0x02,        Limit, false)
WASM_CTRL_OPCODE(If,             0x03,        Limit, false)
WASM_CTRL_OPCODE(Else,           0x04,        Limit, false)
WASM_CTRL_OPCODE(Br,             0x06,        Limit, false)
WASM_CTRL_OPCODE(BrIf,           0x07,        Limit, false)
WASM_CTRL_OPCODE(BrTable,        0x08,        Limit, false)
WASM_CTRL_OPCODE(Return,         0x09,        Limit, false)
WASM_CTRL_OPCODE(Unreachable,    0x0a,        Limit, true)
WASM_CTRL_OPCODE(Drop,           0x0b,        Limit, false)
WASM_CTRL_OPCODE(End,            0x0f,        Limit, false)
WASM_CTRL_OPCODE(Select,         0x05,        Limit, false)

// Constants, locals, globals, and calls.
WASM_MISC_OPCODE(I32Const,       0x10,        Limit, false)
WASM_MISC_OPCODE(I64Const,       0x11,        Limit, true)
WASM_MISC_OPCODE(F64Const,       0x12,        Limit, false)
WASM_MISC_OPCODE(F32Const,       0x13,        Limit, false)
WASM_MISC_OPCODE(GetLocal,       0x14,        Limit, false)
WASM_MISC_OPCODE(SetLocal,       0x15,        Limit, false)
WASM_MISC_OPCODE(Call,           0x16,        Limit, false)
WASM_MISC_OPCODE(CallIndirect,   0x17,        Limit, false)
WASM_MISC_OPCODE(CallImport,     0x18,        Limit, false)
WASM_MISC_OPCODE(TeeLocal,       0x19,        Limit, false)

// Load memory expressions.
WASM_MEMREAD_OPCODE(I32LoadMem8S,    0x20,        I_I, false)
WASM_MEMREAD_OPCODE(I32LoadMem8U,    0x21,        I_I, false)
WASM_MEMREAD_OPCODE(I32LoadMem16S,   0x22,        I_I, false)
WASM_MEMREAD_OPCODE(I32LoadMem16U,   0x23,        I_I, false)
WASM_MEMREAD_OPCODE(I64LoadMem8S,    0x24,        L_I, true)
WASM_MEMREAD_OPCODE(I64LoadMem8U,    0x25,        L_I, true)
WASM_MEMREAD_OPCODE(I64LoadMem16S,   0x26,        L_I, true)
WASM_MEMREAD_OPCODE(I64LoadMem16U,   0x27,        L_I, true)
WASM_MEMREAD_OPCODE(I64LoadMem32S,   0x28,        L_I, true)
WASM_MEMREAD_OPCODE(I64LoadMem32U,   0x29,        L_I, true)
WASM_MEMREAD_OPCODE(I32LoadMem,      0x2a,        I_I, false)
WASM_MEMREAD_OPCODE(I64LoadMem,      0x2b,        L_I, true)
WASM_MEMREAD_OPCODE(F32LoadMem,      0x2c,        F_I, false)
WASM_MEMREAD_OPCODE(F64LoadMem,      0x2d,        D_I, false)

// Store memory expressions.
WASM_MEMSTORE_OPCODE(I32StoreMem8,    0x2e,        I_II, false)
WASM_MEMSTORE_OPCODE(I32StoreMem16,   0x2f,        I_II, false)
WASM_MEMSTORE_OPCODE(I64StoreMem8,    0x30,        L_IL, true)
WASM_MEMSTORE_OPCODE(I64StoreMem16,   0x31,        L_IL, true)
WASM_MEMSTORE_OPCODE(I64StoreMem32,   0x32,        L_IL, true)
WASM_MEMSTORE_OPCODE(I32StoreMem,     0x33,        I_II, false)
WASM_MEMSTORE_OPCODE(I64StoreMem,     0x34,        L_IL, true)
WASM_MEMSTORE_OPCODE(F32StoreMem,     0x35,        F_IF, false)
WASM_MEMSTORE_OPCODE(F64StoreMem,     0x36,        D_ID, false)

// Memory operator
WASM_MISC_OPCODE(MemorySize,      0x3b,        I_V, true)
WASM_MISC_OPCODE(GrowMemory,      0x39,        I_I, true)

// Expressions
WASM_BINARY_OPCODE(I32Add,            0x40, I_II, Add_Int        , false)
WASM_BINARY_OPCODE(I32Sub,            0x41, I_II, Sub_Int        , false)
WASM_BINARY_OPCODE(I32Mul,            0x42, I_II, Mul_Int        , false)
WASM_BINARY_OPCODE(I32DivS,           0x43, I_II, Div_Int        , false)
WASM_BINARY_OPCODE(I32DivU,           0x44, I_II, Div_UInt       , false)
WASM_BINARY_OPCODE(I32RemS,           0x45, I_II, Rem_Int        , false)
WASM_BINARY_OPCODE(I32RemU,           0x46, I_II, Rem_UInt       , false)
WASM_BINARY_OPCODE(I32And,            0x47, I_II, And_Int        , false)
WASM_BINARY_OPCODE(I32Or,             0x48, I_II, Or_Int        , false)
WASM_BINARY_OPCODE(I32Xor,            0x49, I_II, Xor_Int        , false)
WASM_BINARY_OPCODE(I32Shl,            0x4a, I_II, Shl_Int        , false)
WASM_BINARY_OPCODE(I32ShrU,           0x4b, I_II, Shr_UInt       , false)
WASM_BINARY_OPCODE(I32ShrS,           0x4c, I_II, Shr_Int        , false)
WASM_BINARY_OPCODE(I32Eq,             0x4d, I_II, CmEq_Int       , false)
WASM_BINARY_OPCODE(I32Ne,             0x4e, I_II, CmNe_Int       , false)
WASM_BINARY_OPCODE(I32LtS,            0x4f, I_II, CmLt_Int       , false)
WASM_BINARY_OPCODE(I32LeS,            0x50, I_II, CmLe_Int       , false)
WASM_BINARY_OPCODE(I32LtU,            0x51, I_II, CmLt_UInt     , false)
WASM_BINARY_OPCODE(I32LeU,            0x52, I_II, CmLe_UInt     , false)
WASM_BINARY_OPCODE(I32GtS,            0x53, I_II, CmGt_Int       , false)
WASM_BINARY_OPCODE(I32GeS,            0x54, I_II, CmGe_Int       , false)
WASM_BINARY_OPCODE(I32GtU,            0x55, I_II, CmGt_UInt     , false)
WASM_BINARY_OPCODE(I32GeU,            0x56, I_II, CmGe_UInt     , false)
WASM_UNARY__OPCODE(I32Clz,            0x57, I_I , Clz32_Int      , false)
WASM_UNARY__OPCODE(I32Ctz,            0x58, I_I , Ctz_Int        , false)
WASM_UNARY__OPCODE(I32Popcnt,         0x59, I_I , PopCnt_Int     , false)
WASM_UNARY__OPCODE(I32Eqz,            0x5a, I_I , Eqz_Int        , false)
WASM_BINARY_OPCODE(I64Add,            0x5b, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64Sub,            0x5c, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64Mul,            0x5d, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64DivS,           0x5e, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64DivU,           0x5f, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64RemS,           0x60, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64RemU,           0x61, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64And,            0x62, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64Ior,            0x63, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64Xor,            0x64, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64Shl,            0x65, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64ShrU,           0x66, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64ShrS,           0x67, L_LL, Nop            , true)
WASM_BINARY_OPCODE(I64Eq,             0x68, I_LL, Nop            , true)
WASM_BINARY_OPCODE(I64Ne,             0x69, I_LL, Nop            , true)
WASM_BINARY_OPCODE(I64LtS,            0x6a, I_LL, Nop            , true)
WASM_BINARY_OPCODE(I64LeS,            0x6b, I_LL, Nop            , true)
WASM_BINARY_OPCODE(I64LtU,            0x6c, I_LL, Nop            , true)
WASM_BINARY_OPCODE(I64LeU,            0x6d, I_LL, Nop            , true)
WASM_BINARY_OPCODE(I64GtS,            0x6e, I_LL, Nop            , true)
WASM_BINARY_OPCODE(I64GeS,            0x6f, I_LL, Nop            , true)
WASM_BINARY_OPCODE(I64GtU,            0x70, I_LL, Nop            , true)
WASM_BINARY_OPCODE(I64GeU,            0x71, I_LL, Nop            , true)
WASM_UNARY__OPCODE(I64Clz,            0x72, L_L , Nop            , true)
WASM_UNARY__OPCODE(I64Ctz,            0x73, L_L , Nop            , true)
WASM_UNARY__OPCODE(I64Popcnt,         0x74, L_L , Nop            , true)
WASM_BINARY_OPCODE(F32Add,            0x75, F_FF, Add_Flt        , false)
WASM_BINARY_OPCODE(F32Sub,            0x76, F_FF, Sub_Flt        , false)
WASM_BINARY_OPCODE(F32Mul,            0x77, F_FF, Mul_Flt        , false)
WASM_BINARY_OPCODE(F32Div,            0x78, F_FF, Div_Flt        , false)
WASM_BINARY_OPCODE(F32Min,            0x79, F_FF, Min_Flt        , false)
WASM_BINARY_OPCODE(F32Max,            0x7a, F_FF, Max_Flt        , false)
WASM_UNARY__OPCODE(F32Abs,            0x7b, F_F , Abs_Flt        , false)
WASM_UNARY__OPCODE(F32Neg,            0x7c, F_F , Neg_Flt        , false)
WASM_BINARY_OPCODE(F32CopySign,       0x7d, F_FF, Copysign_Flt   , false)
WASM_UNARY__OPCODE(F32Ceil,           0x7e, F_F , Ceil_Flt       , false)
WASM_UNARY__OPCODE(F32Floor,          0x7f, F_F , Floor_Flt      , false)
WASM_UNARY__OPCODE(F32Trunc,          0x80, F_F , Trunc_Flt      , true)
WASM_UNARY__OPCODE(F32NearestInt,     0x81, F_F , Nearest_Flt    , true)
WASM_UNARY__OPCODE(F32Sqrt,           0x82, F_F , Sqrt_Flt       , false)
WASM_BINARY_OPCODE(F32Eq,             0x83, I_FF, CmEq_Flt       , false)
WASM_BINARY_OPCODE(F32Ne,             0x84, I_FF, CmNe_Flt       , false)
WASM_BINARY_OPCODE(F32Lt,             0x85, I_FF, CmLt_Flt       , false)
WASM_BINARY_OPCODE(F32Le,             0x86, I_FF, CmLe_Flt       , false)
WASM_BINARY_OPCODE(F32Gt,             0x87, I_FF, CmGt_Flt       , false)
WASM_BINARY_OPCODE(F32Ge,             0x88, I_FF, CmGe_Flt       , false)
WASM_BINARY_OPCODE(F64Add,            0x89, D_DD, Add_Db         , false)
WASM_BINARY_OPCODE(F64Sub,            0x8a, D_DD, Sub_Db         , false)
WASM_BINARY_OPCODE(F64Mul,            0x8b, D_DD, Mul_Db         , false)
WASM_BINARY_OPCODE(F64Div,            0x8c, D_DD, Div_Db         , false)
WASM_BINARY_OPCODE(F64Min,            0x8d, D_DD, Min_Db         , false)
WASM_BINARY_OPCODE(F64Max,            0x8e, D_DD, Max_Db         , false)
WASM_UNARY__OPCODE(F64Abs,            0x8f, D_D , Abs_Db         , false)
WASM_UNARY__OPCODE(F64Neg,            0x90, D_D , Neg_Db         , false)
WASM_BINARY_OPCODE(F64CopySign,       0x91, D_DD, Copysign_Db    , true)
WASM_UNARY__OPCODE(F64Ceil,           0x92, D_D , Ceil_Db        , false)
WASM_UNARY__OPCODE(F64Floor,          0x93, D_D , Floor_Db       , false)
WASM_UNARY__OPCODE(F64Trunc,          0x94, D_D , Trunc_Db       , true)
WASM_UNARY__OPCODE(F64NearestInt,     0x95, D_D , Nearest_Db     , true)
WASM_UNARY__OPCODE(F64Sqrt,           0x96, D_D , Sqrt_Db        , false)
WASM_BINARY_OPCODE(F64Eq,             0x97, I_DD, CmEq_Db        , false)
WASM_BINARY_OPCODE(F64Ne,             0x98, I_DD, CmNe_Db        , false)
WASM_BINARY_OPCODE(F64Lt,             0x99, I_DD, CmLt_Db        , false)
WASM_BINARY_OPCODE(F64Le,             0x9a, I_DD, CmLe_Db        , false)
WASM_BINARY_OPCODE(F64Gt,             0x9b, I_DD, CmGt_Db        , false)
WASM_BINARY_OPCODE(F64Ge,             0x9c, I_DD, CmGe_Db        , false)
WASM_UNARY__OPCODE(I32SConvertF32,    0x9d, I_F , Conv_FTI       , false)
WASM_UNARY__OPCODE(I32SConvertF64,    0x9e, I_D , Conv_DTI       , false)
WASM_UNARY__OPCODE(I32UConvertF32,    0x9f, I_F , Conv_FTU       , true)
WASM_UNARY__OPCODE(I32UConvertF64,    0xa0, I_D , Conv_DTU       , true)
WASM_UNARY__OPCODE(I32ConvertI64,     0xa1, I_L , Nop            , true)
WASM_UNARY__OPCODE(I64SConvertF32,    0xa2, L_F , Nop            , true)
WASM_UNARY__OPCODE(I64SConvertF64,    0xa3, L_D , Nop            , true)
WASM_UNARY__OPCODE(I64UConvertF32,    0xa4, L_F , Nop            , true)
WASM_UNARY__OPCODE(I64UConvertF64,    0xa5, L_D , Nop            , true)
WASM_UNARY__OPCODE(I64SConvertI32,    0xa6, L_I , Nop            , true)
WASM_UNARY__OPCODE(I64UConvertI32,    0xa7, L_I , Nop            , true)
WASM_UNARY__OPCODE(F32SConvertI32,    0xa8, F_I , Fround_Int     , false)
WASM_UNARY__OPCODE(F32UConvertI32,    0xa9, F_I , Conv_UTF       , true)
WASM_UNARY__OPCODE(F32SConvertI64,    0xaa, F_L , Nop            , true)
WASM_UNARY__OPCODE(F32UConvertI64,    0xab, F_L , Nop            , true)
WASM_UNARY__OPCODE(F32ConvertF64,     0xac, F_D , Fround_Db      , false)
WASM_UNARY__OPCODE(F32ReinterpretI32, 0xad, F_I , Reinterpret_ITF, true)
WASM_UNARY__OPCODE(F64SConvertI32,    0xae, D_I , Conv_ITD       , false)
WASM_UNARY__OPCODE(F64UConvertI32,    0xaf, D_I , Conv_UTD       , false)
WASM_UNARY__OPCODE(F64SConvertI64,    0xb0, D_L , Nop            , true)
WASM_UNARY__OPCODE(F64UConvertI64,    0xb1, D_L , Nop            , true)
WASM_UNARY__OPCODE(F64ConvertF32,     0xb2, D_F , Conv_FTD       , false)
WASM_UNARY__OPCODE(F64ReinterpretI64, 0xb3, D_L , Nop            , true)
WASM_UNARY__OPCODE(I32ReinterpretF32, 0xb4, I_F , Reinterpret_FTI, false)
WASM_UNARY__OPCODE(I64ReinterpretF64, 0xb5, L_D , Nop            , true)
WASM_BINARY_OPCODE(I32Ror,            0xb6, I_II, Ror_Int        , false)
WASM_BINARY_OPCODE(I32Rol,            0xb7, I_II, Rol_Int        , false)

#undef WASM_OPCODE
#undef WASM_SIGNATURE
#undef WASM_CTRL_OPCODE
#undef WASM_MISC_OPCODE
#undef WASM_MEM_OPCODE
#undef WASM_MEMREAD_OPCODE
#undef WASM_MEMSTORE_OPCODE
#undef WASM_UNARY__OPCODE
#undef WASM_BINARY_OPCODE
