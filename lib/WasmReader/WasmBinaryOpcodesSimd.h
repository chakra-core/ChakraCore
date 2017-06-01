//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


//temporary until 64x2 types are implemented
#ifndef FOREACH_SIMD_TYPE
#define FOREACH_SIMD_TYPE(V) \
    V(M128, F)
#endif

#ifndef FOREACH_SIMD_TYPE_W_BASE
#define FOREACH_SIMD_TYPE_W_BASE(V) \
    V(M128, F)   \
    V(M128, I)
#endif

#ifndef WASM_LANE_OPCODE
#define WASM_LANE_OPCODE(opname, opcode, sig, nyi) WASM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_EXTRACTLANE_OPCODE
#define WASM_EXTRACTLANE_OPCODE(opname, opcode, sig, asmjsop, nyi) WASM_LANE_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_REPLACELANE_OPCODE
#define WASM_REPLACELANE_OPCODE(opname, opcode, sig, asmjsop, nyi) WASM_LANE_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_SIMD_BUILD_OPCODE
#define WASM_SIMD_BUILD_OPCODE(opname, opcode, sig, asmjop, lanes, nyi) WASM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_SIMD_MEMREAD_OPCODE
#define WASM_SIMD_MEMREAD_OPCODE(opname, opcode, sig, asmjsop, viewtype, dataWidth, nyi) WASM_MEM_OPCODE(opname, opcode, sig, nyi)
#endif

#ifndef WASM_SIMD_MEMSTORE_OPCODE
#define WASM_SIMD_MEMSTORE_OPCODE(opname, opcode, sig, asmjsop, viewtype, dataWidth, nyi) WASM_MEM_OPCODE(opname, opcode, sig, nyi)
#endif

//SIMD Signatures
#define SIMD_EXTRACT(TYPE, BASE) WASM_SIGNATURE(BASE##_##TYPE, 2, WasmTypes::##BASE##32, WasmTypes::##TYPE)
#define SIMD_BUILD(TYPE, BASE) WASM_SIGNATURE(TYPE##_##BASE, 2, WasmTypes::##TYPE, WasmTypes::##BASE##32)

#define SIMD_ALL(TYPE, BASE) \
    SIMD_EXTRACT(TYPE, BASE) \
    SIMD_BUILD(TYPE, BASE)

FOREACH_SIMD_TYPE_W_BASE(SIMD_ALL)
#undef SIMD_ALL
#undef SIMD_BUILD
#undef SIMD_EXTRACT

WASM_SIGNATURE(M128X3, 3, WasmTypes::M128, WasmTypes::M128, WasmTypes::M128)
WASM_SIGNATURE(M128_I_M128, 3, WasmTypes::M128, WasmTypes::M128, WasmTypes::I32)
WASM_SIGNATURE(M128_M128, 2, WasmTypes::M128, WasmTypes::M128)

WASM_MISC_OPCODE(M128Const, 0x100, Limit, false)
WASM_SIMD_MEMREAD_OPCODE(M128Load, 0x101, M128_I, Simd128_LdArr_F4, Js::ArrayBufferView::TYPE_FLOAT32, 16, false)
WASM_SIMD_MEMSTORE_OPCODE(M128Store, 0x102, M128_I, Simd128_StArr_F4, Js::ArrayBufferView::TYPE_FLOAT32, 16, false)
WASM_UNARY__OPCODE(I16Splat, 0x103, M128_I, Simd128_Splat_I16, false)
WASM_UNARY__OPCODE(I8Splat, 0x104, M128_I, Simd128_Splat_I8, false)
WASM_UNARY__OPCODE(I4Splat, 0x105, M128_I, Simd128_Splat_I4, false)
WASM_MISC_OPCODE(I2Splat, 0x106, Limit, true)
WASM_UNARY__OPCODE(F4Splat, 0x107, M128_F, Simd128_Splat_F4, false)
WASM_MISC_OPCODE(F2Splat, 0x108, Limit, true)
WASM_EXTRACTLANE_OPCODE(I16ExtractLaneS, 0x109, I_M128, Simd128_ExtractLane_I16, false)
WASM_EXTRACTLANE_OPCODE(I16ExtractLaneU, 0x10a, I_M128, Simd128_ExtractLane_U16, false)
WASM_EXTRACTLANE_OPCODE(I8ExtractLaneS, 0x10b, I_M128, Simd128_ExtractLane_I8, false)
WASM_EXTRACTLANE_OPCODE(I8ExtractLaneU, 0x10c, I_M128, Simd128_ExtractLane_U8, false)
WASM_EXTRACTLANE_OPCODE(I4ExtractLane, 0x10d, I_M128, Simd128_ExtractLane_I4, false)
WASM_MISC_OPCODE(I2ExtractLane, 0x10e, Limit, true)
WASM_EXTRACTLANE_OPCODE(F4ExtractLane, 0x10f, I_M128, Simd128_ExtractLane_F4, false)
WASM_MISC_OPCODE(F2ExtractLane, 0x110, Limit, true)
WASM_REPLACELANE_OPCODE(I16ReplaceLane, 0x111, M128_I, Simd128_ReplaceLane_I16, false)
WASM_REPLACELANE_OPCODE(I8ReplaceLane, 0x112, M128_I, Simd128_ReplaceLane_I8, false)
WASM_REPLACELANE_OPCODE(I4ReplaceLane, 0x113, M128_I, Simd128_ReplaceLane_I4, false)
WASM_MISC_OPCODE(I2ReplaceLane, 0x114, Limit, true)
WASM_REPLACELANE_OPCODE(F4ReplaceLane, 0x115, M128_F, Simd128_ReplaceLane_F4, false)
WASM_MISC_OPCODE(F2ReplaceLane, 0x116, Limit, true)
WASM_MISC_OPCODE(V8X16Shuffle, 0x117, Limit, true)
WASM_MISC_OPCODE(I16Add, 0x118, Limit, true)
WASM_MISC_OPCODE(I8Add, 0x119, Limit, true)
WASM_MISC_OPCODE(I4Add, 0x11a, Limit, true)
WASM_MISC_OPCODE(I2Add, 0x11b, Limit, true)
WASM_MISC_OPCODE(I16Sub, 0x11c, Limit, true)
WASM_MISC_OPCODE(I8Sub, 0x11d, Limit, true)
WASM_MISC_OPCODE(I4Sub, 0x11e, Limit, true)
WASM_MISC_OPCODE(I2Sub, 0x11f, Limit, true)
WASM_MISC_OPCODE(I16Mul, 0x120, Limit, true)
WASM_MISC_OPCODE(I8Mul, 0x121, Limit, true)
WASM_MISC_OPCODE(I4Mul, 0x122, Limit, true)
WASM_UNARY__OPCODE(I16Neg, 0x123, M128_M128, Simd128_Neg_I16, false)
WASM_UNARY__OPCODE(I8Neg, 0x124, M128_M128, Simd128_Neg_I8, false)
WASM_UNARY__OPCODE(I4Neg, 0x125, M128_M128, Simd128_Neg_I4,  false)
WASM_MISC_OPCODE(I2Neg, 0x126, Limit, true)
WASM_MISC_OPCODE(I16AddSaturateS, 0x127, Limit, true)
WASM_MISC_OPCODE(I16AddSaturateU, 0x128, Limit, true)
WASM_MISC_OPCODE(I8AddSaturateS, 0x129, Limit, true)
WASM_MISC_OPCODE(I8AddSaturateU, 0x12a, Limit, true)
WASM_MISC_OPCODE(I16SubSaturateS, 0x12b, Limit, true)
WASM_MISC_OPCODE(I16SubSaturateU, 0x12c, Limit, true)
WASM_MISC_OPCODE(I8SubSaturateS, 0x12d, Limit, true)
WASM_MISC_OPCODE(I8SubSaturateU, 0x12e, Limit, true)
WASM_MISC_OPCODE(I16Shl, 0x12f, Limit, true)
WASM_MISC_OPCODE(I8Shl, 0x130, Limit, true)
WASM_MISC_OPCODE(I4Shl, 0x131, Limit, true)
WASM_MISC_OPCODE(I2Shl, 0x132, Limit, true)
WASM_MISC_OPCODE(I16ShrS, 0x133, Limit, true)
WASM_MISC_OPCODE(I16ShrU, 0x134, Limit, true)
WASM_MISC_OPCODE(I8ShrS, 0x135, Limit, true)
WASM_MISC_OPCODE(I8ShrU, 0x136, Limit, true)
WASM_MISC_OPCODE(I4ShrS, 0x137, Limit, true)
WASM_MISC_OPCODE(I4ShrU, 0x138, Limit, true)
WASM_MISC_OPCODE(I2ShrS, 0x139, Limit, true)
WASM_MISC_OPCODE(I2ShrU, 0x13a, Limit, true)
WASM_MISC_OPCODE(M128And, 0x13b, Limit, true)
WASM_MISC_OPCODE(M128Or, 0x13c, Limit, true)
WASM_MISC_OPCODE(M128Xor, 0x13d, Limit, true)
WASM_MISC_OPCODE(M128Not, 0x13e, Limit, true)
WASM_MISC_OPCODE(M128Bitselect, 0x13f, Limit, true)
WASM_MISC_OPCODE(I16AnyTrue, 0x140, Limit, true)
WASM_MISC_OPCODE(I8AnyTrue, 0x141, Limit, true)
WASM_MISC_OPCODE(I4AnyTrue, 0x142, Limit, true)
WASM_MISC_OPCODE(I2AnyTrue, 0x143, Limit, true)
WASM_MISC_OPCODE(I16AllTrue, 0x144, Limit, true)
WASM_MISC_OPCODE(I8AllTrue, 0x145, Limit, true)
WASM_MISC_OPCODE(I4AllTrue, 0x146, Limit, true)
WASM_MISC_OPCODE(I2AllTrue, 0x147, Limit, true)
WASM_MISC_OPCODE(I16Eq, 0x148, Limit, true)
WASM_MISC_OPCODE(I8Eq, 0x149, Limit, true)
WASM_MISC_OPCODE(I4Eq, 0x14a, Limit, true)
WASM_MISC_OPCODE(F4Eq, 0x14b, Limit, true)
WASM_MISC_OPCODE(F2Eq, 0x14c, Limit, true)
WASM_MISC_OPCODE(I16Ne, 0x14d, Limit, true)
WASM_MISC_OPCODE(I8Ne, 0x14e, Limit, true)
WASM_MISC_OPCODE(I4Ne, 0x14f, Limit, true)
WASM_MISC_OPCODE(F4Ne, 0x150, Limit, true)
WASM_MISC_OPCODE(F2Ne, 0x151, Limit, true)
WASM_MISC_OPCODE(I16LtS, 0x152, Limit, true)
WASM_MISC_OPCODE(I16LtU, 0x153, Limit, true)
WASM_MISC_OPCODE(I8LtS, 0x154, Limit, true)
WASM_MISC_OPCODE(I8LtU, 0x155, Limit, true)
WASM_MISC_OPCODE(I4LtS, 0x156, Limit, true)
WASM_MISC_OPCODE(I4LtU, 0x157, Limit, true)
WASM_MISC_OPCODE(F4Lt, 0x158, Limit, true)
WASM_MISC_OPCODE(F2Lt, 0x159, Limit, true)
WASM_MISC_OPCODE(I16LeS, 0x15a, Limit, true)
WASM_MISC_OPCODE(I16LeU, 0x15b, Limit, true)
WASM_MISC_OPCODE(I8LeS, 0x15c, Limit, true)
WASM_MISC_OPCODE(I8LeU, 0x15d, Limit, true)
WASM_MISC_OPCODE(I4LeS, 0x15e, Limit, true)
WASM_MISC_OPCODE(I4LeU, 0x15f, Limit, true)
WASM_MISC_OPCODE(F4Le, 0x160, Limit, true)
WASM_MISC_OPCODE(F2Le, 0x161, Limit, true)
WASM_MISC_OPCODE(I16GtS, 0x162, Limit, true)
WASM_MISC_OPCODE(I16GtU, 0x163, Limit, true)
WASM_MISC_OPCODE(I8GtS, 0x164, Limit, true)
WASM_MISC_OPCODE(I8GtU, 0x165, Limit, true)
WASM_MISC_OPCODE(I4GtS, 0x166, Limit, true)
WASM_MISC_OPCODE(I4GtU, 0x167, Limit, true)
WASM_MISC_OPCODE(F4Gt, 0x168, Limit, true)
WASM_MISC_OPCODE(F2Gt, 0x169, Limit, true)
WASM_MISC_OPCODE(I16GeS, 0x16a, Limit, true)
WASM_MISC_OPCODE(I16GeU, 0x16b, Limit, true)
WASM_MISC_OPCODE(I8GeS, 0x16c, Limit, true)
WASM_MISC_OPCODE(I8GeU, 0x16d, Limit, true)
WASM_MISC_OPCODE(I4GeS, 0x16e, Limit, true)
WASM_MISC_OPCODE(I4GeU, 0x16f, Limit, true)
WASM_MISC_OPCODE(F4Ge, 0x170, Limit, true)
WASM_MISC_OPCODE(F2Ge, 0x171, Limit, true)
WASM_UNARY__OPCODE(F4Neg, 0x172, M128_M128, Simd128_Neg_F4, false)
WASM_MISC_OPCODE(F2Neg, 0x173, Limit, true)
WASM_MISC_OPCODE(F4Abs, 0x174, Limit, true)
WASM_MISC_OPCODE(F2Abs, 0x175, Limit, true)
WASM_MISC_OPCODE(F4Min, 0x176, Limit, true)
WASM_MISC_OPCODE(F2Min, 0x177, Limit, true)
WASM_MISC_OPCODE(F4Max, 0x178, Limit, true)
WASM_MISC_OPCODE(F2Max, 0x179, Limit, true)
WASM_MISC_OPCODE(F4Add, 0x17a, Limit, true)
WASM_MISC_OPCODE(F2Add, 0x17b, Limit, true)
WASM_MISC_OPCODE(F4Sub, 0x17c, Limit, true)
WASM_MISC_OPCODE(F2Sub, 0x17d, Limit, true)
WASM_MISC_OPCODE(F4Div, 0x17e, Limit, true)
WASM_MISC_OPCODE(F2Div, 0x17f, Limit, true)
WASM_MISC_OPCODE(F4Mul, 0x180, Limit, true)
WASM_MISC_OPCODE(F2Mul, 0x181, Limit, true)
WASM_MISC_OPCODE(F4Sqrt, 0x182, Limit, true)
WASM_MISC_OPCODE(F2Sqrt, 0x183, Limit, true)
WASM_UNARY__OPCODE(F4ConvertS, 0x184, M128_M128, Simd128_FromInt32x4_F4, false)
WASM_UNARY__OPCODE(F4ConvertU, 0x185, M128_M128, Simd128_FromUint32x4_F4, false)
WASM_MISC_OPCODE(F2ConvertS, 0x186, Limit, true)
WASM_MISC_OPCODE(F2ConvertU, 0x187, Limit, true)
WASM_UNARY__OPCODE(I4TruncS, 0x188, M128_M128, Simd128_FromFloat32x4_I4, false)
WASM_UNARY__OPCODE(I4TruncU, 0x189, M128_M128, Simd128_FromFloat32x4_U4, false)
WASM_MISC_OPCODE(I2TruncS, 0x18a, Limit, true)
WASM_MISC_OPCODE(I2TruncU, 0x18b, Limit, true)

#undef WASM_SIMD_BUILD_OPCODE
#undef WASM_LANE_OPCODE
#undef WASM_EXTRACTLANE_OPCODE
#undef WASM_SIMD_MEMREAD_OPCODE
#undef WASM_SIMD_MEMSTORE_OPCODE
#undef WASM_REPLACELANE_OPCODE
