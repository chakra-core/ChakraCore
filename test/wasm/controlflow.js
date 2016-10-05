//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const blob = WScript.LoadBinaryFile('controlflow.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;
print(a.a(0));
print(a.a(1));
print(a.yield_top(0))
print(a.yield_top(1))
print(a.br_if(0))
print(a.br_if(1))
print(a.br())
print(a.block())
