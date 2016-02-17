//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef WASM_TOKEN
#define WASM_TOKEN(token)
#endif

WASM_TOKEN(ID)
WASM_TOKEN(LPAREN)
WASM_TOKEN(RPAREN)
WASM_TOKEN(DOT)
WASM_TOKEN(CONSTLIT)
WASM_TOKEN(FLOATLIT)
WASM_TOKEN(INTLIT)
WASM_TOKEN(STRINGLIT)
WASM_TOKEN(EOF)

#undef WASM_TOKEN
