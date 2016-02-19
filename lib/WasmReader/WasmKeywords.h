//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef WASM_KEYWORD
#define WASM_KEYWORD(token, name)
#endif

#ifndef WASM_KEYWORD_FDI
#define WASM_KEYWORD_FDI(token, name) \
    WASM_KEYWORD(token##_F32, name) \
    WASM_KEYWORD(token##_F64, name) \
    WASM_KEYWORD(token##_I32, name)
#endif

#ifndef WASM_KEYWORD_BIN
#define WASM_KEYWORD_BIN(token, name) WASM_KEYWORD(token, name)
#endif

#ifndef WASM_KEYWORD_BIN_TYPED
#define WASM_KEYWORD_BIN_TYPED(token, name, op, resultType, lhsType, rhsType) \
    WASM_KEYWORD_BIN(token, name)
#endif

#ifndef WASM_KEYWORD_COMPARE
#define WASM_KEYWORD_COMPARE(token, name, op, type) \
    WASM_KEYWORD_BIN_TYPED(token, name, op, I32, type, type)
#endif

#ifndef WASM_KEYWORD_COMPARE_F
#define WASM_KEYWORD_COMPARE_F(token, name, op) \
    WASM_KEYWORD_COMPARE(token##_F32, name, op, F32)
#endif

#ifndef WASM_KEYWORD_COMPARE_D
#define WASM_KEYWORD_COMPARE_D(token, name, op) \
    WASM_KEYWORD_COMPARE(token##_F64, name, op, F64)
#endif

#ifndef WASM_KEYWORD_COMPARE_I
#define WASM_KEYWORD_COMPARE_I(token, name, op) \
    WASM_KEYWORD_COMPARE(token##_I32, name, op, I32)
#endif

#ifndef WASM_KEYWORD_COMPARE_FD
#define WASM_KEYWORD_COMPARE_FD(token, name, opPrefix) \
    WASM_KEYWORD_COMPARE_F(token, name, opPrefix##_Flt) \
    WASM_KEYWORD_COMPARE_D(token, name, opPrefix##_Db)
#endif

#ifndef WASM_KEYWORD_COMPARE_FDI
#define WASM_KEYWORD_COMPARE_FDI(token, name, opPrefix) \
    WASM_KEYWORD_COMPARE_FD(token, name, opPrefix) \
    WASM_KEYWORD_COMPARE_I(token, name, opPrefix##_Int)
#endif

#ifndef WASM_KEYWORD_BIN_MATH_F
#define WASM_KEYWORD_BIN_MATH_F(token, name, op) \
    WASM_KEYWORD_BIN_TYPED(token##_F32, name, op, F32, F32, F32)
#endif

#ifndef WASM_KEYWORD_BIN_MATH_D
#define WASM_KEYWORD_BIN_MATH_D(token, name, op) \
    WASM_KEYWORD_BIN_TYPED(token##_F64, name, op, F64, F64, F64)
#endif

#ifndef WASM_KEYWORD_BIN_MATH_I
#define WASM_KEYWORD_BIN_MATH_I(token, name, op) \
    WASM_KEYWORD_BIN_TYPED(token##_I32, name, op, I32, I32, I32)
#endif

#ifndef WASM_KEYWORD_BIN_MATH_FD
#define WASM_KEYWORD_BIN_MATH_FD(token, name, opPrefix) \
    WASM_KEYWORD_BIN_MATH_F(token, name, opPrefix##_Flt) \
    WASM_KEYWORD_BIN_MATH_D(token, name, opPrefix##_Db)
#endif

#ifndef WASM_KEYWORD_BIN_MATH_FDI
#define WASM_KEYWORD_BIN_MATH_FDI(token, name, opPrefix) \
    WASM_KEYWORD_BIN_MATH_FD(token, name, opPrefix) \
    WASM_KEYWORD_BIN_MATH_I(token, name, opPrefix##_Int)
#endif

#ifndef WASM_KEYWORD_UNARY
#define WASM_KEYWORD_UNARY(token, name, op, resultType, inputType) WASM_KEYWORD(token, name)
#endif

#ifndef WASM_KEYWORD_UNARY_I
#define WASM_KEYWORD_UNARY_I(token, name, opPrefix) \
    WASM_KEYWORD_UNARY(token##_I32, name, opPrefix##_Int, I32, I32)
#endif

#ifndef WASM_KEYWORD_UNARY_FD
#define WASM_KEYWORD_UNARY_FD(token, name, opPrefix) \
    WASM_KEYWORD_UNARY(token##_F32, name, opPrefix##_Flt, F32, F32) \
    WASM_KEYWORD_UNARY(token##_F64, name, opPrefix##_Db, F64, F64)
#endif

#ifndef WASM_MEMTYPE
#define WASM_MEMTYPE(token, name) WASM_KEYWORD(token, name)
#endif

#ifndef WASM_LOCALTYPE
#define WASM_LOCALTYPE(token, name) WASM_MEMTYPE(token, name)
#endif

#ifndef WASM_MEMOP
#define WASM_MEMOP(token, name, type) WASM_KEYWORD(token, name)
#endif

#ifndef WASM_MEMREAD
#define WASM_MEMREAD(token, name, type) WASM_MEMOP(token, name, type)
#endif

#ifndef WASM_MEMREAD_I
#define WASM_MEMREAD_I(token, name) \
    WASM_MEMREAD(token##_I32, name, I32)
#endif

#ifndef WASM_MEMREAD_FDI
#define WASM_MEMREAD_FDI(token, name) \
    WASM_MEMREAD(token##_F32, name, F32) \
    WASM_MEMREAD(token##_F64, name, F64) \
    WASM_MEMREAD_I(token, name)
#endif

#ifndef WASM_MEMSTORE
#define WASM_MEMSTORE(token, name, type) WASM_MEMOP(token, name, type)
#endif

#ifndef WASM_MEMSTORE_I
#define WASM_MEMSTORE_I(token, name) \
    WASM_MEMSTORE(token##_I32, name, I32)
#endif

#ifndef WASM_MEMSTORE_FDI
#define WASM_MEMSTORE_FDI(token, name) \
    WASM_MEMSTORE(token##_F32, name, F32) \
    WASM_MEMSTORE(token##_F64, name, F64) \
    WASM_MEMSTORE_I(token, name)
#endif

// memory
WASM_MEMREAD_FDI(LOAD,    load)
WASM_MEMREAD_I(LOAD8S,     load8_s)
WASM_MEMREAD_I(LOAD16S,    load16_s)
WASM_MEMREAD_I(LOAD8U,     load8_u)
WASM_MEMREAD_I(LOAD16U,    load16_u)
WASM_MEMSTORE_FDI(STORE,   store)
WASM_MEMSTORE_I(STORE8,    store8)
WASM_MEMSTORE_I(STORE16,   store16)

WASM_KEYWORD(GETPARAM,               getparam)
WASM_KEYWORD(GETLOCAL,               getlocal)
WASM_KEYWORD(SETLOCAL,               setlocal)

// types
WASM_MEMTYPE(I8,     i8)
WASM_MEMTYPE(I16,    i16)

WASM_LOCALTYPE(I32,    i32)
WASM_LOCALTYPE(I64,    i64)
WASM_LOCALTYPE(F32,    f32)
WASM_LOCALTYPE(F64,    f64)

// calls
WASM_KEYWORD(CALL, call)
WASM_KEYWORD(CALL_INDIRECT, call_indirect)

// control flow ops
WASM_KEYWORD(NOP,        nop)
WASM_KEYWORD(BLOCK,      block)
WASM_KEYWORD(IF,         if)
WASM_KEYWORD(IF_ELSE,    if_else)
WASM_KEYWORD(LOOP,       loop)
WASM_KEYWORD(LABEL,      label)
WASM_KEYWORD(BREAK,      break)
WASM_KEYWORD(SWITCH,     switch)
WASM_KEYWORD(RETURN,     return)
WASM_KEYWORD_FDI(CONST,      const)

// structures
WASM_KEYWORD(FUNC,       func)
WASM_KEYWORD(PARAM,      param)
WASM_KEYWORD(RESULT,     result)
WASM_KEYWORD(LOCAL,      local)
WASM_KEYWORD(MODULE,     module)
WASM_KEYWORD(EXPORT,     export)
WASM_KEYWORD(IMPORT,     import)
WASM_KEYWORD(TABLE,      table)
WASM_KEYWORD(MEMORY,     memory)
WASM_KEYWORD(TYPE,       type)

// unary ops
WASM_KEYWORD_UNARY_I(NOT,    not, Not)
WASM_KEYWORD_UNARY_I(CLZ,    clz, Clz32)
WASM_KEYWORD_UNARY_FD(NEG,    neg, Neg)
WASM_KEYWORD_UNARY_FD(ABS,    abs, Abs)
WASM_KEYWORD_UNARY_FD(CEIL,   ceil, Ceil)
WASM_KEYWORD_UNARY_FD(FLOOR,  floor, Floor)

// TODO: michhol, new ops
// WASM_KEYWORD_UNARY_FD(TRUNC, trunc, Trunc)
// WASM_KEYWORD_UNARY_I(CTZ,    ctz, Ctz)
// WASM_KEYWORD_UNARY_I(POPCNT, popcnt, PopCnt)
// WASM_KEYWORD_UNARY_FD(ROUND,  round, Round)

// conversion ops
WASM_KEYWORD_UNARY(TRUNC_S_F32_I32, trunc_s, Conv_FTI, I32, F32)
WASM_KEYWORD_UNARY(TRUNC_S_F64_I32, trunc_s, Conv_DTI, I32, F64)
WASM_KEYWORD_UNARY(CONVERT_S_I32_F32, convert_s, Fround_Int, F32, I32)
WASM_KEYWORD_UNARY(CONVERT_S_I32_F64, convert_s, Conv_ITD, F64, I32)
WASM_KEYWORD_UNARY(CONVERT_U_I32_F64, convert_u, Conv_UTD, F64, I32)
WASM_KEYWORD_UNARY(PROMOTE_F32_F64, promote, Conv_ITD, F64, F32)
WASM_KEYWORD_UNARY(DEMOTE_F64_F32, demote, Fround_Db, F32, F64)

// TODO: new conversions
// i32.trunc_u/f32
// i32.trunc_u/f64
// i32.reinterpret/f32
// f32.convert_u/i32
// f32.reinterpret/i32


// binary ops
WASM_KEYWORD_BIN_MATH_FDI(ADD, add, Add)
WASM_KEYWORD_BIN_MATH_FDI(SUB, sub, Sub)
WASM_KEYWORD_BIN_MATH_FDI(MUL, mul, Mul)

WASM_KEYWORD_BIN_MATH_I(DIVS, divs, Div_Int)
WASM_KEYWORD_BIN_MATH_I(MODS, mods, Rem_Int)
WASM_KEYWORD_BIN_MATH_I(AND, and, And_Int)
WASM_KEYWORD_BIN_MATH_I(OR, or, Or_Int)
WASM_KEYWORD_BIN_MATH_I(XOR, xor, Xor_Int)
WASM_KEYWORD_BIN_MATH_I(SHL, shl, Shl_Int)
WASM_KEYWORD_BIN_MATH_I(SHR, shr, Shr_Int)
WASM_KEYWORD_BIN_MATH_I(DIVU, divu, Div_UInt)
WASM_KEYWORD_BIN_MATH_I(MODU, modu, Rem_UInt)

WASM_KEYWORD_BIN_MATH_FD(DIV, div, Div)

WASM_KEYWORD_BIN_MATH_D(MOD, mod, Rem_Db)


// compare ops
WASM_KEYWORD_COMPARE_FDI(EQ,    eq, CmEq)
WASM_KEYWORD_COMPARE_FDI(NEQ,   neq, CmNe)
WASM_KEYWORD_COMPARE_I(LTS,    lts, CmLt_Int)
WASM_KEYWORD_COMPARE_I(LTU,    ltu, CmLt_UnInt)
WASM_KEYWORD_COMPARE_I(LES,    les, CmLe_Int)
WASM_KEYWORD_COMPARE_I(LEU,    leu, CmLe_UnInt)
WASM_KEYWORD_COMPARE_I(GTS,    gts, CmGt_Int)
WASM_KEYWORD_COMPARE_I(GTU,    gtu, CmGt_UnInt)
WASM_KEYWORD_COMPARE_I(GES,    ges, CmGe_Int)
WASM_KEYWORD_COMPARE_I(GEU,    geu, CmGe_UnInt)
WASM_KEYWORD_COMPARE_FD(LT,     lt, CmLt)
WASM_KEYWORD_COMPARE_FD(LE,     le, CmLe)
WASM_KEYWORD_COMPARE_FD(GT,     gt, CmGt)
WASM_KEYWORD_COMPARE_FD(GE,     ge, CmGe)

#undef WASM_MEMSTORE_FDI
#undef WASM_MEMSTORE_I
#undef WASM_MEMSTORE
#undef WASM_MEMREAD_FDI
#undef WASM_MEMREAD_I
#undef WASM_MEMREAD
#undef WASM_MEMOP
#undef WASM_KEYWORD_COMPARE_FDI
#undef WASM_KEYWORD_COMPARE_FD
#undef WASM_KEYWORD_COMPARE_I
#undef WASM_KEYWORD_COMPARE_D
#undef WASM_KEYWORD_COMPARE_F
#undef WASM_KEYWORD_COMPARE
#undef WASM_KEYWORD_UNARY_I
#undef WASM_KEYWORD_UNARY_FD
#undef WASM_KEYWORD_UNARY
#undef WASM_KEYWORD_BIN_MATH_FDI
#undef WASM_KEYWORD_BIN_MATH_FD
#undef WASM_KEYWORD_BIN_MATH_I
#undef WASM_KEYWORD_BIN_MATH_D
#undef WASM_KEYWORD_BIN_MATH_F
#undef WASM_KEYWORD_BIN_TYPED
#undef WASM_KEYWORD_BIN
#undef WASM_LOCALTYPE
#undef WASM_MEMTYPE
#undef WASM_KEYWORD_FDI
#undef WASM_KEYWORD
