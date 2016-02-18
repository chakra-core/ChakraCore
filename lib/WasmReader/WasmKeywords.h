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
#define WASM_KEYWORD_UNARY(token, name) WASM_KEYWORD(token, name)
#endif

#ifndef WASM_MEMTYPE
#define WASM_MEMTYPE(token, name) WASM_KEYWORD(token, name)
#endif

#ifndef WASM_LOCALTYPE
#define WASM_LOCALTYPE(token, name) WASM_MEMTYPE(token, name)
#endif

// memory
WASM_KEYWORD(GET_NEAR_UNALIGNED_S,   getnearunaligneds)
WASM_KEYWORD(GET_FAR_UNALIGNED_S,    getfarunaligneds)
WASM_KEYWORD(GET_NEAR_S,             getnears)
WASM_KEYWORD(GET_FAR_S,              getfars)
WASM_KEYWORD(SET_NEAR_UNALIGNED_S,   setnearunaligneds)
WASM_KEYWORD(SET_FAR_UNALIGNED_S,    setfarunaligneds)
WASM_KEYWORD(SET_NEAR_S,             setnears)
WASM_KEYWORD(SET_FAR_S,              setfars)

WASM_KEYWORD(GET_NEAR_UNALIGNED_U,   getnearunalignedu)
WASM_KEYWORD(GET_FAR_UNALIGNED_U,    getfarunalignedu)
WASM_KEYWORD(GET_NEAR_U,             getnearu)
WASM_KEYWORD(GET_FAR_U,              getfaru)
WASM_KEYWORD(SET_NEAR_UNALIGNED_U,   setnearunalignedu)
WASM_KEYWORD(SET_FAR_UNALIGNED_U,    setfarunalignedu)
WASM_KEYWORD(SET_NEAR_U,             setnearu)
WASM_KEYWORD(SET_FAR_U,              setfaru)

WASM_KEYWORD(GETPARAM,               getparam)
WASM_KEYWORD(GETLOCAL,               getlocal)
WASM_KEYWORD(SETLOCAL,               setlocal)
WASM_KEYWORD(GETGLOBAL,              getglobal)
WASM_KEYWORD(SETGLOBAL,              setglobal)

// types
WASM_MEMTYPE(I8,     i8)
WASM_MEMTYPE(I16,    i16)

WASM_LOCALTYPE(I32,    i32)
WASM_LOCALTYPE(I64,    i64)
WASM_LOCALTYPE(F32,    f32)
WASM_LOCALTYPE(F64,    f64)

// control flow ops
WASM_KEYWORD(NOP,        nop)
WASM_KEYWORD(BLOCK,      block)
WASM_KEYWORD(IF,         if)
WASM_KEYWORD(IF_ELSE,    if_else)
WASM_KEYWORD(LOOP,       loop)
WASM_KEYWORD(LABEL,      label)
WASM_KEYWORD(BREAK,      break)
WASM_KEYWORD(SWITCH,     switch)
WASM_KEYWORD(CALL,       call)
WASM_KEYWORD(DISPATCH,   dispatch)
WASM_KEYWORD(RETURN,     return)
WASM_KEYWORD(DESTRUCT,   destruct)
WASM_KEYWORD_FDI(CONST,      const)

// structures
WASM_KEYWORD(FUNC,       func)
WASM_KEYWORD(PARAM,      param)
WASM_KEYWORD(RESULT,     result)
WASM_KEYWORD(LOCAL,      local)
WASM_KEYWORD(MODULE,     module)
WASM_KEYWORD(GLOBAL,     global)
WASM_KEYWORD(EXPORT,     export)
WASM_KEYWORD(TABLE,      table)
WASM_KEYWORD(MEMORY,     memory)
WASM_KEYWORD(DATA,       data)

// unary ops
WASM_KEYWORD_UNARY(NOT,    not)
WASM_KEYWORD_UNARY(CLZ,    clz)
WASM_KEYWORD_UNARY(CTZ,    ctz)
WASM_KEYWORD_UNARY(NEG,    neg)
WASM_KEYWORD_UNARY(ABS,    abs)
WASM_KEYWORD_UNARY(CEIL,   ceil)
WASM_KEYWORD_UNARY(FLOOR,  floor)
WASM_KEYWORD_UNARY(TRUNC,  trunc)
WASM_KEYWORD_UNARY(ROUND,  round)

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

// conversions
WASM_KEYWORD(CONVERTS,   converts)
WASM_KEYWORD(CONVERTU,   convertu)
WASM_KEYWORD(CONVERT,    convert)
WASM_KEYWORD(CAST,       cast)

// process actions
WASM_KEYWORD(INVOKE,     invoke)
WASM_KEYWORD(ASSERTEQ,   asserteq)

#undef WASM_KEYWORD_COMPARE_FDI
#undef WASM_KEYWORD_COMPARE_FD
#undef WASM_KEYWORD_COMPARE_I
#undef WASM_KEYWORD_COMPARE_D
#undef WASM_KEYWORD_COMPARE_F
#undef WASM_KEYWORD_COMPARE
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
