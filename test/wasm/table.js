//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const blob = WScript.LoadBinaryFile('table.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;
//print(a.Mt.call(2));
//print(a.call(0));
//print(a.call(1));
//print(a.call(2));
//print(a.call(3));
//print(a.call(4));
//print(a.call(5));
print(a.call(6));
print(a.call(7));
print(a.call(8));
print(a.call(9));

