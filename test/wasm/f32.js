//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const blob = WScript.LoadBinaryFile('f32.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;
print(a.min(11, 11.01)); // 11
print(a.max(11, 11.01)); // 11.010000228881836
print(a.min(NaN, 11.01)); // NaN
print(a.max(NaN, 11.01)); // NaN
print(a.min(11, NaN)); // NaN
print(a.max(1/0, 11.01)); // Infinity
print(a.max(11.01, 0/0)); // NaN
print(a.max(0/0, 11.01)); // NaN
print(a.max(NaN, -NaN)); // NaN
print("Reinterpret tests");
print(a.reinterpret_f2i(0))
print(a.reinterpret_f2i(-1)) //NaN
print(a.reinterpret_f2i(-1082130432));
print(a.reinterpret_f2i(-1081800544));
print(a.reinterpret_f2i(2139095040)); //Inf
print(a.reinterpret_f2i(-8388608)); //-Inf
print(a.reinterpret_f2i(-8388607)); //NaN

