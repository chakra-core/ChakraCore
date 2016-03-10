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

#ifndef WASM_KEYWORD_UNARY_D
#define WASM_KEYWORD_UNARY_D(token, name, opPrefix) \
    WASM_KEYWORD_UNARY(token##_F64, name, opPrefix##_Db, F64, F64)
#endif

#ifndef WASM_KEYWORD_UNARY_FD
#define WASM_KEYWORD_UNARY_FD(token, name, opPrefix) \
    WASM_KEYWORD_UNARY(token##_F32, name, opPrefix##_Flt, F32, F32) \
    WASM_KEYWORD_UNARY_D(token, name, opPrefix)
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
