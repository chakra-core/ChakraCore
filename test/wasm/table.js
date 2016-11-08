//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const blob1 = WScript.LoadBinaryFile('table.wasm');
const moduleBytesView1 = new Uint8Array(blob1);
var a1 = Wasm.instantiateModule(moduleBytesView1, {"m" : {x : 10}}).exports;

//local offset
print(a1.call(1));
print(a1.call(2));
print(a1.call(3));
print(a1.call(4));

//global offset
print(a1.call(6));
print(a1.call(7));
print(a1.call(8));
print(a1.call(9));

//imported global offset
print(a1.call(10));
print(a1.call(11));
print(a1.call(12));
print(a1.call(13));

