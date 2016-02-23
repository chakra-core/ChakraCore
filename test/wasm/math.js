//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let ffi = {};
let wasm = readbuffer("math.wasm");
let exports = Wasm.instantiateModule(new Uint8Array(wasm), ffi).exports;
print(exports.ctz(1));
print(exports.ctz(4));
print(exports.ctz(-Math.pow(2,31)));
print(exports.ctz(0));
