//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// define this to include all opcodes
#ifndef WASM_OPCODE
#define WASM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_SIGNATURE
#define WASM_SIGNATURE(id,...)
#endif

#ifndef WASM_CTRL_OPCODE
#define WASM_CTRL_OPCODE(opname, opcode, sig, imp, wat) WASM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_MISC_OPCODE
#define WASM_MISC_OPCODE(opname, opcode, sig, imp, wat) WASM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_MEM_OPCODE
#define WASM_MEM_OPCODE(opname, opcode, sig, imp, wat) WASM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_ATOMIC_OPCODE
#define WASM_ATOMIC_OPCODE(opname, opcode, sig, imp, viewtype, wat) WASM_MEM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_MEMREAD_OPCODE
#define WASM_MEMREAD_OPCODE(opname, opcode, sig, imp, viewtype, wat) WASM_MEM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_MEMSTORE_OPCODE
#define WASM_MEMSTORE_OPCODE(opname, opcode, sig, imp, viewtype, wat) WASM_MEM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_ATOMICREAD_OPCODE
#define WASM_ATOMICREAD_OPCODE(opname, opcode, sig, imp, viewtype, wat) WASM_ATOMIC_OPCODE(opname, opcode, sig, imp, viewtype, wat)
#endif

#ifndef WASM_ATOMICSTORE_OPCODE
#define WASM_ATOMICSTORE_OPCODE(opname, opcode, sig, imp, viewtype, wat) WASM_ATOMIC_OPCODE(opname, opcode, sig, imp, viewtype, wat)
#endif

#ifndef WASM_UNARY__OPCODE
#define WASM_UNARY__OPCODE(opname, opcode, sig, asmjop, imp, wat) WASM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_BINARY_OPCODE
#define WASM_BINARY_OPCODE(opname, opcode, sig, asmjop, imp, wat) WASM_OPCODE(opname, opcode, sig, imp, wat)
#endif

#ifndef WASM_EMPTY__OPCODE
#define WASM_EMPTY__OPCODE(opname, opcode, asmjop, imp, wat) WASM_OPCODE(opname, opcode, Limit, imp, wat)
#endif

#ifndef WASM_PREFIX
#define WASM_PREFIX(prefixname, op, imp, errorMsg)
#endif

#define WASM_PREFIX_TRACING 0xf0
#define WASM_PREFIX_NUMERIC 0xfc
#define WASM_PREFIX_THREADS 0xfe

WASM_PREFIX(Numeric, WASM_PREFIX_NUMERIC, Wasm::WasmNontrapping::IsEnabled(), "WebAssembly nontrapping float-to-int conversion support is not enabled")
WASM_PREFIX(Threads, WASM_PREFIX_THREADS, Wasm::Threads::IsEnabled(), "WebAssembly Threads support is not enabled")
#if ENABLE_DEBUG_CONFIG_OPTIONS
// We won't even look at that prefix in release builds
// Mark the prefix as not implemented so we don't allow it in the binary buffer
WASM_PREFIX(Tracing, WASM_PREFIX_TRACING, false, "Tracing opcodes not allowed")
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
WASM_CTRL_OPCODE(Unreachable, 0x00, Limit, true, "unreachable")
WASM_CTRL_OPCODE(Nop,         0x01, Limit, true, "nop")
WASM_CTRL_OPCODE(Block,       0x02, Limit, true, "block")
WASM_CTRL_OPCODE(Loop,        0x03, Limit, true, "loop")
WASM_CTRL_OPCODE(If,          0x04, Limit, true, "if")
WASM_CTRL_OPCODE(Else,        0x05, Limit, true, "else")
WASM_CTRL_OPCODE(End,         0x0b, Limit, true, "end")
WASM_CTRL_OPCODE(Br,          0x0c, Limit, true, "br")
WASM_CTRL_OPCODE(BrIf,        0x0d, Limit, true, "br_if")
WASM_CTRL_OPCODE(BrTable,     0x0e, Limit, true, "br_table")
WASM_CTRL_OPCODE(Return,      0x0f, Limit, true, "return")

// Call operators
WASM_MISC_OPCODE(Call,        0x10, Limit, true, "call")
WASM_MISC_OPCODE(CallIndirect,0x11, Limit, true, "call_indirect")

// Parametric operators
WASM_CTRL_OPCODE(Drop,        0x1a, Limit, true, "drop")
WASM_CTRL_OPCODE(Select,      0x1b, Limit, true, "select")

// Variable access
WASM_MISC_OPCODE(GetLocal,     0x20, Limit, true, "get_local")
WASM_MISC_OPCODE(SetLocal,     0x21, Limit, true, "set_local")
WASM_MISC_OPCODE(TeeLocal,     0x22, Limit, true, "tee_local")
WASM_MISC_OPCODE(GetGlobal,    0x23, Limit, true, "get_global")
WASM_MISC_OPCODE(SetGlobal,    0x24, Limit, true, "set_global")

// Memory-related operators
// Load memory expressions.
WASM_MEMREAD_OPCODE(I32LoadMem,    0x28, I_I, true, Js::ArrayBufferView::TYPE_INT32, "i32.load")
WASM_MEMREAD_OPCODE(I64LoadMem,    0x29, L_I, true, Js::ArrayBufferView::TYPE_INT64, "i64.load")
WASM_MEMREAD_OPCODE(F32LoadMem,    0x2a, F_I, true, Js::ArrayBufferView::TYPE_FLOAT32, "f32.load")
WASM_MEMREAD_OPCODE(F64LoadMem,    0x2b, D_I, true, Js::ArrayBufferView::TYPE_FLOAT64, "f64.load")
WASM_MEMREAD_OPCODE(I32LoadMem8S,  0x2c, I_I, true, Js::ArrayBufferView::TYPE_INT8, "i32.load8_s")
WASM_MEMREAD_OPCODE(I32LoadMem8U,  0x2d, I_I, true, Js::ArrayBufferView::TYPE_UINT8, "i32.load8_u")
WASM_MEMREAD_OPCODE(I32LoadMem16S, 0x2e, I_I, true, Js::ArrayBufferView::TYPE_INT16, "i32.load16_s")
WASM_MEMREAD_OPCODE(I32LoadMem16U, 0x2f, I_I, true, Js::ArrayBufferView::TYPE_UINT16, "i32.load16_u")
WASM_MEMREAD_OPCODE(I64LoadMem8S,  0x30, L_I, true, Js::ArrayBufferView::TYPE_INT8_TO_INT64, "i64.load8_s")
WASM_MEMREAD_OPCODE(I64LoadMem8U,  0x31, L_I, true, Js::ArrayBufferView::TYPE_UINT8_TO_INT64, "i64.load8_u")
WASM_MEMREAD_OPCODE(I64LoadMem16S, 0x32, L_I, true, Js::ArrayBufferView::TYPE_INT16_TO_INT64, "i64.load16_s")
WASM_MEMREAD_OPCODE(I64LoadMem16U, 0x33, L_I, true, Js::ArrayBufferView::TYPE_UINT16_TO_INT64, "i64.load16_u")
WASM_MEMREAD_OPCODE(I64LoadMem32S, 0x34, L_I, true, Js::ArrayBufferView::TYPE_INT32_TO_INT64, "i64.load32_s")
WASM_MEMREAD_OPCODE(I64LoadMem32U, 0x35, L_I, true, Js::ArrayBufferView::TYPE_UINT32_TO_INT64, "i64.load32_u")

// Store memory expressions.
WASM_MEMSTORE_OPCODE(I32StoreMem,   0x36, I_II, true, Js::ArrayBufferView::TYPE_INT32, "i32.store")
WASM_MEMSTORE_OPCODE(I64StoreMem,   0x37, L_IL, true, Js::ArrayBufferView::TYPE_INT64, "i64.store")
WASM_MEMSTORE_OPCODE(F32StoreMem,   0x38, F_IF, true, Js::ArrayBufferView::TYPE_FLOAT32, "f32.store")
WASM_MEMSTORE_OPCODE(F64StoreMem,   0x39, D_ID, true, Js::ArrayBufferView::TYPE_FLOAT64, "f64.store")
WASM_MEMSTORE_OPCODE(I32StoreMem8,  0x3a, I_II, true, Js::ArrayBufferView::TYPE_INT8, "i32.store8")
WASM_MEMSTORE_OPCODE(I32StoreMem16, 0x3b, I_II, true, Js::ArrayBufferView::TYPE_INT16, "i32.store16")
WASM_MEMSTORE_OPCODE(I64StoreMem8,  0x3c, L_IL, true, Js::ArrayBufferView::TYPE_INT8_TO_INT64, "i64.store8")
WASM_MEMSTORE_OPCODE(I64StoreMem16, 0x3d, L_IL, true, Js::ArrayBufferView::TYPE_INT16_TO_INT64, "i64.store16")
WASM_MEMSTORE_OPCODE(I64StoreMem32, 0x3e, L_IL, true, Js::ArrayBufferView::TYPE_INT32_TO_INT64, "i64.store32")

WASM_MISC_OPCODE(MemorySize, 0x3f, I_I, true, "memory.size")
WASM_MISC_OPCODE(MemoryGrow,    0x40, I_I, true, "memory.grow")

// Constants
WASM_MISC_OPCODE(I32Const,     0x41, Limit, true, "i32.const")
WASM_MISC_OPCODE(I64Const,     0x42, Limit, true, "i64.const")
WASM_MISC_OPCODE(F32Const,     0x43, Limit, true, "f32.const")
WASM_MISC_OPCODE(F64Const,     0x44, Limit, true, "f64.const")

////////////////////////////////////////////////////////////
// Comparison operators
WASM_UNARY__OPCODE(I32Eqz,            0x45, I_I , Eqz_Int        , true, "i32.eqz")
WASM_BINARY_OPCODE(I32Eq,             0x46, I_II, CmEq_Int       , true, "i32.eq")
WASM_BINARY_OPCODE(I32Ne,             0x47, I_II, CmNe_Int       , true, "i32.ne")
WASM_BINARY_OPCODE(I32LtS,            0x48, I_II, CmLt_Int       , true, "i32.lt_s")
WASM_BINARY_OPCODE(I32LtU,            0x49, I_II, CmLt_UInt      , true, "i32.lt_u")
WASM_BINARY_OPCODE(I32GtS,            0x4a, I_II, CmGt_Int       , true, "i32.gt_s")
WASM_BINARY_OPCODE(I32GtU,            0x4b, I_II, CmGt_UInt      , true, "i32.gt_u")
WASM_BINARY_OPCODE(I32LeS,            0x4c, I_II, CmLe_Int       , true, "i32.le_s")
WASM_BINARY_OPCODE(I32LeU,            0x4d, I_II, CmLe_UInt      , true, "i32.le_u")
WASM_BINARY_OPCODE(I32GeS,            0x4e, I_II, CmGe_Int       , true, "i32.ge_s")
WASM_BINARY_OPCODE(I32GeU,            0x4f, I_II, CmGe_UInt      , true, "i32.ge_u")

WASM_UNARY__OPCODE(I64Eqz,            0x50, I_L , Eqz_Long       , true, "i64.eqz")
WASM_BINARY_OPCODE(I64Eq,             0x51, I_LL, CmEq_Long      , true, "i64.eq")
WASM_BINARY_OPCODE(I64Ne,             0x52, I_LL, CmNe_Long      , true, "i64.ne")
WASM_BINARY_OPCODE(I64LtS,            0x53, I_LL, CmLt_Long      , true, "i64.lt_s")
WASM_BINARY_OPCODE(I64LtU,            0x54, I_LL, CmLt_ULong     , true, "i64.lt_u")
WASM_BINARY_OPCODE(I64GtS,            0x55, I_LL, CmGt_Long      , true, "i64.gt_s")
WASM_BINARY_OPCODE(I64GtU,            0x56, I_LL, CmGt_ULong     , true, "i64.gt_u")
WASM_BINARY_OPCODE(I64LeS,            0x57, I_LL, CmLe_Long      , true, "i64.le_s")
WASM_BINARY_OPCODE(I64LeU,            0x58, I_LL, CmLe_ULong     , true, "i64.le_u")
WASM_BINARY_OPCODE(I64GeS,            0x59, I_LL, CmGe_Long      , true, "i64.ge_s")
WASM_BINARY_OPCODE(I64GeU,            0x5a, I_LL, CmGe_ULong     , true, "i64.ge_u")

WASM_BINARY_OPCODE(F32Eq,             0x5b, I_FF, CmEq_Flt       , true, "f32.eq")
WASM_BINARY_OPCODE(F32Ne,             0x5c, I_FF, CmNe_Flt       , true, "f32.ne")
WASM_BINARY_OPCODE(F32Lt,             0x5d, I_FF, CmLt_Flt       , true, "f32.lt")
WASM_BINARY_OPCODE(F32Gt,             0x5e, I_FF, CmGt_Flt       , true, "f32.gt")
WASM_BINARY_OPCODE(F32Le,             0x5f, I_FF, CmLe_Flt       , true, "f32.le")
WASM_BINARY_OPCODE(F32Ge,             0x60, I_FF, CmGe_Flt       , true, "f32.ge")

WASM_BINARY_OPCODE(F64Eq,             0x61, I_DD, CmEq_Db        , true, "f64.eq")
WASM_BINARY_OPCODE(F64Ne,             0x62, I_DD, CmNe_Db        , true, "f64.ne")
WASM_BINARY_OPCODE(F64Lt,             0x63, I_DD, CmLt_Db        , true, "f64.lt")
WASM_BINARY_OPCODE(F64Gt,             0x64, I_DD, CmGt_Db        , true, "f64.gt")
WASM_BINARY_OPCODE(F64Le,             0x65, I_DD, CmLe_Db        , true, "f64.le")
WASM_BINARY_OPCODE(F64Ge,             0x66, I_DD, CmGe_Db        , true, "f64.ge")
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Numeric operators
WASM_UNARY__OPCODE(I32Clz,            0x67, I_I , Clz32_Int      , true, "i32.clz")
WASM_UNARY__OPCODE(I32Ctz,            0x68, I_I , Ctz_Int        , true, "i32.ctz")
WASM_UNARY__OPCODE(I32Popcnt,         0x69, I_I , PopCnt_Int     , true, "i32.popcnt")
WASM_BINARY_OPCODE(I32Add,            0x6a, I_II, Add_Int        , true, "i32.add")
WASM_BINARY_OPCODE(I32Sub,            0x6b, I_II, Sub_Int        , true, "i32.sub")
WASM_BINARY_OPCODE(I32Mul,            0x6c, I_II, Mul_Int        , true, "i32.mul")
WASM_BINARY_OPCODE(I32DivS,           0x6d, I_II, Div_Trap_Int   , true, "i32.div_s")
WASM_BINARY_OPCODE(I32DivU,           0x6e, I_II, Div_Trap_UInt  , true, "i32.div_u")
WASM_BINARY_OPCODE(I32RemS,           0x6f, I_II, Rem_Trap_Int   , true, "i32.rem_s")
WASM_BINARY_OPCODE(I32RemU,           0x70, I_II, Rem_Trap_UInt  , true, "i32.rem_u")
WASM_BINARY_OPCODE(I32And,            0x71, I_II, And_Int        , true, "i32.and")
WASM_BINARY_OPCODE(I32Or,             0x72, I_II, Or_Int         , true, "i32.or")
WASM_BINARY_OPCODE(I32Xor,            0x73, I_II, Xor_Int        , true, "i32.xor")
WASM_BINARY_OPCODE(I32Shl,            0x74, I_II, Shl_Int        , true, "i32.shl")
WASM_BINARY_OPCODE(I32ShrS,           0x75, I_II, Shr_Int        , true, "i32.shr_s")
WASM_BINARY_OPCODE(I32ShrU,           0x76, I_II, Shr_UInt       , true, "i32.shr_u")
WASM_BINARY_OPCODE(I32Rol,            0x77, I_II, Rol_Int        , true, "i32.rotl")
WASM_BINARY_OPCODE(I32Ror,            0x78, I_II, Ror_Int        , true, "i32.rotr")

WASM_UNARY__OPCODE(I64Clz,            0x79, L_L , Clz_Long       , true, "i64.clz")
WASM_UNARY__OPCODE(I64Ctz,            0x7a, L_L , Ctz_Long       , true, "i64.ctz")
WASM_UNARY__OPCODE(I64Popcnt,         0x7b, L_L , PopCnt_Long    , true, "i64.popcnt")
WASM_BINARY_OPCODE(I64Add,            0x7c, L_LL, Add_Long       , true, "i64.add")
WASM_BINARY_OPCODE(I64Sub,            0x7d, L_LL, Sub_Long       , true, "i64.sub")
WASM_BINARY_OPCODE(I64Mul,            0x7e, L_LL, Mul_Long       , true, "i64.mul")
WASM_BINARY_OPCODE(I64DivS,           0x7f, L_LL, Div_Trap_Long  , true, "i64.div_s")
WASM_BINARY_OPCODE(I64DivU,           0x80, L_LL, Div_Trap_ULong , true, "i64.div_u")
WASM_BINARY_OPCODE(I64RemS,           0x81, L_LL, Rem_Trap_Long  , true, "i64.rem_s")
WASM_BINARY_OPCODE(I64RemU,           0x82, L_LL, Rem_Trap_ULong , true, "i64.rem_u")
WASM_BINARY_OPCODE(I64And,            0x83, L_LL, And_Long       , true, "i64.and")
WASM_BINARY_OPCODE(I64Or,             0x84, L_LL, Or_Long        , true, "i64.or")
WASM_BINARY_OPCODE(I64Xor,            0x85, L_LL, Xor_Long       , true, "i64.xor")
WASM_BINARY_OPCODE(I64Shl,            0x86, L_LL, Shl_Long       , true, "i64.shl")
WASM_BINARY_OPCODE(I64ShrS,           0x87, L_LL, Shr_Long       , true, "i64.shr_s")
WASM_BINARY_OPCODE(I64ShrU,           0x88, L_LL, Shr_ULong      , true, "i64.shr_u")
WASM_BINARY_OPCODE(I64Rol,            0x89, L_LL, Rol_Long       , true, "i64.rotl")
WASM_BINARY_OPCODE(I64Ror,            0x8a, L_LL, Ror_Long       , true, "i64.rotr")

WASM_UNARY__OPCODE(F32Abs,            0x8b, F_F , Abs_Flt        , true, "f32.abs")
WASM_UNARY__OPCODE(F32Neg,            0x8c, F_F , Neg_Flt        , true, "f32.neg")
WASM_UNARY__OPCODE(F32Ceil,           0x8d, F_F , Ceil_Flt       , true, "f32.ceil")
WASM_UNARY__OPCODE(F32Floor,          0x8e, F_F , Floor_Flt      , true, "f32.floor")
WASM_UNARY__OPCODE(F32Trunc,          0x8f, F_F , Trunc_Flt      , true, "f32.trunc")
WASM_UNARY__OPCODE(F32NearestInt,     0x90, F_F , Nearest_Flt    , true, "f32.nearest")
WASM_UNARY__OPCODE(F32Sqrt,           0x91, F_F , Sqrt_Flt       , true, "f32.sqrt")
WASM_BINARY_OPCODE(F32Add,            0x92, F_FF, Add_Flt        , true, "f32.add")
WASM_BINARY_OPCODE(F32Sub,            0x93, F_FF, Sub_Flt        , true, "f32.sub")
WASM_BINARY_OPCODE(F32Mul,            0x94, F_FF, Mul_Flt        , true, "f32.mul")
WASM_BINARY_OPCODE(F32Div,            0x95, F_FF, Div_Flt        , true, "f32.div")
WASM_BINARY_OPCODE(F32Min,            0x96, F_FF, Min_Flt        , true, "f32.min")
WASM_BINARY_OPCODE(F32Max,            0x97, F_FF, Max_Flt        , true, "f32.max")
WASM_BINARY_OPCODE(F32CopySign,       0x98, F_FF, Copysign_Flt   , true, "f32.copysign")

WASM_UNARY__OPCODE(F64Abs,            0x99, D_D , Abs_Db         , true, "f64.abs")
WASM_UNARY__OPCODE(F64Neg,            0x9a, D_D , Neg_Db         , true, "f64.neg")
WASM_UNARY__OPCODE(F64Ceil,           0x9b, D_D , Ceil_Db        , true, "f64.ceil")
WASM_UNARY__OPCODE(F64Floor,          0x9c, D_D , Floor_Db       , true, "f64.floor")
WASM_UNARY__OPCODE(F64Trunc,          0x9d, D_D , Trunc_Db       , true, "f64.trunc")
WASM_UNARY__OPCODE(F64NearestInt,     0x9e, D_D , Nearest_Db     , true, "f64.nearest")
WASM_UNARY__OPCODE(F64Sqrt,           0x9f, D_D , Sqrt_Db        , true, "f64.sqrt")
WASM_BINARY_OPCODE(F64Add,            0xa0, D_DD, Add_Db         , true, "f64.add")
WASM_BINARY_OPCODE(F64Sub,            0xa1, D_DD, Sub_Db         , true, "f64.sub")
WASM_BINARY_OPCODE(F64Mul,            0xa2, D_DD, Mul_Db         , true, "f64.mul")
WASM_BINARY_OPCODE(F64Div,            0xa3, D_DD, Div_Db         , true, "f64.div")
WASM_BINARY_OPCODE(F64Min,            0xa4, D_DD, Min_Db         , true, "f64.min")
WASM_BINARY_OPCODE(F64Max,            0xa5, D_DD, Max_Db         , true, "f64.max")
WASM_BINARY_OPCODE(F64CopySign,       0xa6, D_DD, Copysign_Db    , true, "f64.copysign")
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Conversions
WASM_UNARY__OPCODE(I32Wrap_I64,       0xa7, I_L , Conv_LTI       , true, "i32.wrap/i64")
WASM_UNARY__OPCODE(I32TruncS_F32,     0xa8, I_F , Conv_Check_FTI , true, "i32.trunc_s/f32")
WASM_UNARY__OPCODE(I32TruncU_F32,     0xa9, I_F , Conv_Check_FTU , true, "i32.trunc_u/f32")
WASM_UNARY__OPCODE(I32TruncS_F64,     0xaa, I_D , Conv_Check_DTI , true, "i32.trunc_s/f64")
WASM_UNARY__OPCODE(I32TruncU_F64,     0xab, I_D , Conv_Check_DTU , true, "i32.trunc_u/f64")

WASM_UNARY__OPCODE(I64ExtendS_I32,    0xac, L_I , Conv_ITL       , true, "i64.extend_s/i32")
WASM_UNARY__OPCODE(I64ExtendU_I32,    0xad, L_I , Conv_UTL       , true, "i64.extend_u/i32")
WASM_UNARY__OPCODE(I64TruncS_F32,     0xae, L_F , Conv_Check_FTL , true, "i64.trunc_s/f32")
WASM_UNARY__OPCODE(I64TruncU_F32,     0xaf, L_F , Conv_Check_FTUL, true, "i64.trunc_u/f32")
WASM_UNARY__OPCODE(I64TruncS_F64,     0xb0, L_D , Conv_Check_DTL , true, "i64.trunc_s/f64")
WASM_UNARY__OPCODE(I64TruncU_F64,     0xb1, L_D , Conv_Check_DTUL, true, "i64.trunc_u/f64")

#define __has_nontrapping (Wasm::WasmNontrapping::IsEnabled())
#define __prefix (WASM_PREFIX_NUMERIC << 8)
WASM_UNARY__OPCODE(I32SatTruncS_F32, __prefix | 0x00, I_F, Conv_Sat_FTI, __has_nontrapping, "i32.trunc_s:sat/f32")
WASM_UNARY__OPCODE(I32SatTruncU_F32, __prefix | 0x01, I_F, Conv_Sat_FTU, __has_nontrapping, "i32.trunc_u:sat/f32")
WASM_UNARY__OPCODE(I32SatTruncS_F64, __prefix | 0x02, I_D, Conv_Sat_DTI, __has_nontrapping, "i32.trunc_s:sat/f64")
WASM_UNARY__OPCODE(I32SatTruncU_F64, __prefix | 0x03, I_D, Conv_Sat_DTU, __has_nontrapping, "i32.trunc_u:sat/f64")

WASM_UNARY__OPCODE(I64SatTruncS_F32, __prefix | 0x04, L_F, Conv_Sat_FTL, __has_nontrapping, "i64.trunc_s:sat/f32")
WASM_UNARY__OPCODE(I64SatTruncU_F32, __prefix | 0x05, L_F, Conv_Sat_FTUL, __has_nontrapping, "i64.trunc_u:sat/f32")
WASM_UNARY__OPCODE(I64SatTruncS_F64, __prefix | 0x06, L_D, Conv_Sat_DTL, __has_nontrapping, "i64.trunc_s:sat/f64")
WASM_UNARY__OPCODE(I64SatTruncU_F64, __prefix | 0x07, L_D, Conv_Sat_DTUL, __has_nontrapping, "i64.trunc_u:sat/f64")
#undef __has_nontrapping
#undef __prefix

WASM_UNARY__OPCODE(F32SConvertI32,    0xb2, F_I , Fround_Int     , true, "f32.convert_s/i32")
WASM_UNARY__OPCODE(F32UConvertI32,    0xb3, F_I , Conv_UTF       , true, "f32.convert_u/i32")
WASM_UNARY__OPCODE(F32SConvertI64,    0xb4, F_L , Conv_LTF       , true, "f32.convert_s/i64")
WASM_UNARY__OPCODE(F32UConvertI64,    0xb5, F_L , Conv_ULTF      , true, "f32.convert_u/i64")
WASM_UNARY__OPCODE(F32DemoteF64,      0xb6, F_D , Fround_Db      , true, "f32.demote/f64")

WASM_UNARY__OPCODE(F64SConvertI32,    0xb7, D_I , Conv_ITD       , true, "f64.convert_s/i32")
WASM_UNARY__OPCODE(F64UConvertI32,    0xb8, D_I , Conv_UTD       , true, "f64.convert_u/i32")
WASM_UNARY__OPCODE(F64SConvertI64,    0xb9, D_L , Conv_LTD       , true, "f64.convert_s/i64")
WASM_UNARY__OPCODE(F64UConvertI64,    0xba, D_L , Conv_ULTD      , true, "f64.convert_u/i64")
WASM_UNARY__OPCODE(F64PromoteF32,     0xbb, D_F , Conv_FTD       , true, "f64.promote/f32")
////////////////////////////////////////////////////////////

// Reinterpretations
WASM_UNARY__OPCODE(I32ReinterpretF32, 0xbc, I_F , Reinterpret_FTI, true, "i32.reinterpret/f32")
WASM_UNARY__OPCODE(I64ReinterpretF64, 0xbd, L_D , Reinterpret_DTL, true, "i64.reinterpret/f64")
WASM_UNARY__OPCODE(F32ReinterpretI32, 0xbe, F_I , Reinterpret_ITF, true, "f32.reinterpret/i32")
WASM_UNARY__OPCODE(F64ReinterpretI64, 0xbf, D_L , Reinterpret_LTD, true, "f64.reinterpret/i64")

// New sign extend operators
#define __has_signextends (Wasm::SignExtends::IsEnabled())
WASM_UNARY__OPCODE(I32Extend8_s , 0xc0, I_I, I32Extend8_s , __has_signextends, "i32.extend8_s")
WASM_UNARY__OPCODE(I32Extend16_s, 0xc1, I_I, I32Extend16_s, __has_signextends, "i32.extend16_s")
WASM_UNARY__OPCODE(I64Extend8_s , 0xc2, L_L, I64Extend8_s , __has_signextends, "i64.extend8_s")
WASM_UNARY__OPCODE(I64Extend16_s, 0xc3, L_L, I64Extend16_s, __has_signextends, "i64.extend16_s")
WASM_UNARY__OPCODE(I64Extend32_s, 0xc4, L_L, I64Extend32_s, __has_signextends, "i64.extend32_s")

#define __has_atomics (Wasm::Threads::IsEnabled())
#define __prefix (WASM_PREFIX_THREADS << 8)
WASM_ATOMICREAD_OPCODE (I32AtomicLoad          , __prefix | 0x10, I_I  , __has_atomics, Js::ArrayBufferView::TYPE_INT32, "i32.atomic.load")
WASM_ATOMICREAD_OPCODE (I64AtomicLoad          , __prefix | 0x11, L_I  , __has_atomics, Js::ArrayBufferView::TYPE_INT64, "i64.atomic.load")
WASM_ATOMICREAD_OPCODE (I32AtomicLoad8U        , __prefix | 0x12, I_I  , __has_atomics, Js::ArrayBufferView::TYPE_UINT8, "i32.atomic.load8_u")
WASM_ATOMICREAD_OPCODE (I32AtomicLoad16U       , __prefix | 0x13, I_I  , __has_atomics, Js::ArrayBufferView::TYPE_UINT16, "i32.atomic.load16_u")
WASM_ATOMICREAD_OPCODE (I64AtomicLoad8U        , __prefix | 0x14, L_I  , __has_atomics, Js::ArrayBufferView::TYPE_UINT8_TO_INT64, "i64.atomic.load8_u")
WASM_ATOMICREAD_OPCODE (I64AtomicLoad16U       , __prefix | 0x15, L_I  , __has_atomics, Js::ArrayBufferView::TYPE_UINT16_TO_INT64, "i64.atomic.load16_u")
WASM_ATOMICREAD_OPCODE (I64AtomicLoad32U       , __prefix | 0x16, L_I  , __has_atomics, Js::ArrayBufferView::TYPE_UINT32_TO_INT64, "i64.atomic.load32_u")
WASM_ATOMICSTORE_OPCODE(I32AtomicStore         , __prefix | 0x17, I_II , __has_atomics, Js::ArrayBufferView::TYPE_INT32, "i32.atomic.store")
WASM_ATOMICSTORE_OPCODE(I64AtomicStore         , __prefix | 0x18, L_IL , __has_atomics, Js::ArrayBufferView::TYPE_INT64, "i64.atomic.store")
WASM_ATOMICSTORE_OPCODE(I32AtomicStore8        , __prefix | 0x19, I_II , __has_atomics, Js::ArrayBufferView::TYPE_INT8, "i32.atomic.store8")
WASM_ATOMICSTORE_OPCODE(I32AtomicStore16       , __prefix | 0x1a, I_II , __has_atomics, Js::ArrayBufferView::TYPE_INT16, "i32.atomic.store16")
WASM_ATOMICSTORE_OPCODE(I64AtomicStore8        , __prefix | 0x1b, L_IL , __has_atomics, Js::ArrayBufferView::TYPE_INT8_TO_INT64, "i64.atomic.store8")
WASM_ATOMICSTORE_OPCODE(I64AtomicStore16       , __prefix | 0x1c, L_IL , __has_atomics, Js::ArrayBufferView::TYPE_INT16_TO_INT64, "i64.atomic.store16")
WASM_ATOMICSTORE_OPCODE(I64AtomicStore32       , __prefix | 0x1d, L_IL , __has_atomics, Js::ArrayBufferView::TYPE_INT32_TO_INT64, "i64.atomic.store32")
WASM_ATOMIC_OPCODE     (I32AtomicRmwAdd        , __prefix | 0x1e, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT32, "i32.atomic.rmw.add")
WASM_ATOMIC_OPCODE     (I64AtomicRmwAdd        , __prefix | 0x1f, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT64, "i64.atomic.rmw.add")
WASM_ATOMIC_OPCODE     (I32AtomicRmw8UAdd      , __prefix | 0x20, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8, "i32.atomic.rmw8_u.add")
WASM_ATOMIC_OPCODE     (I32AtomicRmw16UAdd     , __prefix | 0x21, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16, "i32.atomic.rmw16_u.add")
WASM_ATOMIC_OPCODE     (I64AtomicRmw8UAdd      , __prefix | 0x22, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8_TO_INT64, "i64.atomic.rmw8_u.add")
WASM_ATOMIC_OPCODE     (I64AtomicRmw16UAdd     , __prefix | 0x23, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16_TO_INT64, "i64.atomic.rmw16_u.add")
WASM_ATOMIC_OPCODE     (I64AtomicRmw32UAdd     , __prefix | 0x24, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT32_TO_INT64, "i64.atomic.rmw32_u.add")
WASM_ATOMIC_OPCODE     (I32AtomicRmwSub        , __prefix | 0x25, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT32, "i32.atomic.rmw.sub")
WASM_ATOMIC_OPCODE     (I64AtomicRmwSub        , __prefix | 0x26, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT64, "i64.atomic.rmw.sub")
WASM_ATOMIC_OPCODE     (I32AtomicRmw8USub      , __prefix | 0x27, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8, "i32.atomic.rmw8_u.sub")
WASM_ATOMIC_OPCODE     (I32AtomicRmw16USub     , __prefix | 0x28, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16, "i32.atomic.rmw16_u.sub")
WASM_ATOMIC_OPCODE     (I64AtomicRmw8USub      , __prefix | 0x29, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8_TO_INT64, "i64.atomic.rmw8_u.sub")
WASM_ATOMIC_OPCODE     (I64AtomicRmw16USub     , __prefix | 0x2a, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16_TO_INT64, "i64.atomic.rmw16_u.sub")
WASM_ATOMIC_OPCODE     (I64AtomicRmw32USub     , __prefix | 0x2b, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT32_TO_INT64, "i64.atomic.rmw32_u.sub")
WASM_ATOMIC_OPCODE     (I32AtomicRmwAnd        , __prefix | 0x2c, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT32, "i32.atomic.rmw.and")
WASM_ATOMIC_OPCODE     (I64AtomicRmwAnd        , __prefix | 0x2d, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT64, "i64.atomic.rmw.and")
WASM_ATOMIC_OPCODE     (I32AtomicRmw8UAnd      , __prefix | 0x2e, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8, "i32.atomic.rmw8_u.and")
WASM_ATOMIC_OPCODE     (I32AtomicRmw16UAnd     , __prefix | 0x2f, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16, "i32.atomic.rmw16_u.and")
WASM_ATOMIC_OPCODE     (I64AtomicRmw8UAnd      , __prefix | 0x30, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8_TO_INT64, "i64.atomic.rmw8_u.and")
WASM_ATOMIC_OPCODE     (I64AtomicRmw16UAnd     , __prefix | 0x31, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16_TO_INT64, "i64.atomic.rmw16_u.and")
WASM_ATOMIC_OPCODE     (I64AtomicRmw32UAnd     , __prefix | 0x32, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT32_TO_INT64, "i64.atomic.rmw32_u.and")
WASM_ATOMIC_OPCODE     (I32AtomicRmwOr         , __prefix | 0x33, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT32, "i32.atomic.rmw.or")
WASM_ATOMIC_OPCODE     (I64AtomicRmwOr         , __prefix | 0x34, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT64, "i64.atomic.rmw.or")
WASM_ATOMIC_OPCODE     (I32AtomicRmw8UOr       , __prefix | 0x35, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8, "i32.atomic.rmw8_u.or")
WASM_ATOMIC_OPCODE     (I32AtomicRmw16UOr      , __prefix | 0x36, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16, "i32.atomic.rmw16_u.or")
WASM_ATOMIC_OPCODE     (I64AtomicRmw8UOr       , __prefix | 0x37, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8_TO_INT64, "i64.atomic.rmw8_u.or")
WASM_ATOMIC_OPCODE     (I64AtomicRmw16UOr      , __prefix | 0x38, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16_TO_INT64, "i64.atomic.rmw16_u.or")
WASM_ATOMIC_OPCODE     (I64AtomicRmw32UOr      , __prefix | 0x39, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT32_TO_INT64, "i64.atomic.rmw32_u.or")
WASM_ATOMIC_OPCODE     (I32AtomicRmwXor        , __prefix | 0x3a, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT32, "i32.atomic.rmw.xor")
WASM_ATOMIC_OPCODE     (I64AtomicRmwXor        , __prefix | 0x3b, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT64, "i64.atomic.rmw.xor")
WASM_ATOMIC_OPCODE     (I32AtomicRmw8UXor      , __prefix | 0x3c, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8, "i32.atomic.rmw8_u.xor")
WASM_ATOMIC_OPCODE     (I32AtomicRmw16UXor     , __prefix | 0x3d, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16, "i32.atomic.rmw16_u.xor")
WASM_ATOMIC_OPCODE     (I64AtomicRmw8UXor      , __prefix | 0x3e, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8_TO_INT64, "i64.atomic.rmw8_u.xor")
WASM_ATOMIC_OPCODE     (I64AtomicRmw16UXor     , __prefix | 0x3f, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16_TO_INT64, "i64.atomic.rmw16_u.xor")
WASM_ATOMIC_OPCODE     (I64AtomicRmw32UXor     , __prefix | 0x40, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT32_TO_INT64, "i64.atomic.rmw32_u.xor")
WASM_ATOMIC_OPCODE     (I32AtomicRmwXchg       , __prefix | 0x41, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT32, "i32.atomic.rmw.xchg")
WASM_ATOMIC_OPCODE     (I64AtomicRmwXchg       , __prefix | 0x42, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_INT64, "i64.atomic.rmw.xchg")
WASM_ATOMIC_OPCODE     (I32AtomicRmw8UXchg     , __prefix | 0x43, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8, "i32.atomic.rmw8_u.xchg")
WASM_ATOMIC_OPCODE     (I32AtomicRmw16UXchg    , __prefix | 0x44, I_II , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16, "i32.atomic.rmw16_u.xchg")
WASM_ATOMIC_OPCODE     (I64AtomicRmw8UXchg     , __prefix | 0x45, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8_TO_INT64, "i64.atomic.rmw8_u.xchg")
WASM_ATOMIC_OPCODE     (I64AtomicRmw16UXchg    , __prefix | 0x46, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16_TO_INT64, "i64.atomic.rmw16_u.xchg")
WASM_ATOMIC_OPCODE     (I64AtomicRmw32UXchg    , __prefix | 0x47, L_IL , (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT32_TO_INT64, "i64.atomic.rmw32_u.xchg")
WASM_ATOMIC_OPCODE     (I32AtomicRmwCmpxchg    , __prefix | 0x48, I_III, (false && __has_atomics), Js::ArrayBufferView::TYPE_INT32, "i32.atomic.rmw.cmpxchg")
WASM_ATOMIC_OPCODE     (I64AtomicRmwCmpxchg    , __prefix | 0x49, L_ILL, (false && __has_atomics), Js::ArrayBufferView::TYPE_INT64, "i64.atomic.rmw.cmpxchg")
WASM_ATOMIC_OPCODE     (I32AtomicRmw8UCmpxchg  , __prefix | 0x4a, I_III, (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8, "i32.atomic.rmw8_u.cmpxchg")
WASM_ATOMIC_OPCODE     (I32AtomicRmw16UCmpxchg , __prefix | 0x4b, I_III, (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16, "i32.atomic.rmw16_u.cmpxchg")
WASM_ATOMIC_OPCODE     (I64AtomicRmw8UCmpxchg  , __prefix | 0x4c, L_ILL, (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT8_TO_INT64, "i64.atomic.rmw8_u.cmpxchg")
WASM_ATOMIC_OPCODE     (I64AtomicRmw16UCmpxchg , __prefix | 0x4d, L_ILL, (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT16_TO_INT64, "i64.atomic.rmw16_u.cmpxchg")
WASM_ATOMIC_OPCODE     (I64AtomicRmw32UCmpxchg , __prefix | 0x4e, L_ILL, (false && __has_atomics), Js::ArrayBufferView::TYPE_UINT32_TO_INT64, "i64.atomic.rmw32_u.cmpxchg")
#undef __prefix
#undef __has_atomics

#if ENABLE_DEBUG_CONFIG_OPTIONS
#define __prefix (WASM_PREFIX_TRACING << 8)
WASM_UNARY__OPCODE(PrintFuncName    , __prefix | 0x00, V_I , PrintFuncName    , true, "")
WASM_EMPTY__OPCODE(PrintArgSeparator, __prefix | 0x01,       PrintArgSeparator, true, "")
WASM_EMPTY__OPCODE(PrintBeginCall   , __prefix | 0x02,       PrintBeginCall   , true, "")
WASM_EMPTY__OPCODE(PrintNewLine     , __prefix | 0x03,       PrintNewLine     , true, "")
WASM_UNARY__OPCODE(PrintEndCall     , __prefix | 0x04, V_I , PrintEndCall     , true, "")
WASM_UNARY__OPCODE(PrintI32         , __prefix | 0x0c, I_I , PrintI32         , true, "")
WASM_UNARY__OPCODE(PrintI64         , __prefix | 0x0d, L_L , PrintI64         , true, "")
WASM_UNARY__OPCODE(PrintF32         , __prefix | 0x0e, F_F , PrintF32         , true, "")
WASM_UNARY__OPCODE(PrintF64         , __prefix | 0x0f, D_D , PrintF64         , true, "")
#undef __prefix
#endif

//Simd
#ifdef ENABLE_WASM_SIMD
#include "WasmBinaryOpcodesSimd.h"
#endif

#undef WASM_PREFIX_THREADS
#undef WASM_PREFIX_NUMERIC
#undef WASM_PREFIX_TRACING
#undef WASM_PREFIX
#undef WASM_OPCODE
#undef WASM_SIGNATURE
#undef WASM_CTRL_OPCODE
#undef WASM_MISC_OPCODE
#undef WASM_MEM_OPCODE
#undef WASM_MEMREAD_OPCODE
#undef WASM_MEMSTORE_OPCODE
#undef WASM_ATOMIC_OPCODE
#undef WASM_ATOMICREAD_OPCODE
#undef WASM_ATOMICSTORE_OPCODE
#undef WASM_UNARY__OPCODE
#undef WASM_BINARY_OPCODE
#undef WASM_EMPTY__OPCODE
