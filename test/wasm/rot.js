//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const blob = WScript.LoadBinaryFile('rot.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;
print(a.rotl(11,2)); // == 44
print(a.rotl(65536,2)); // == 262144
print(a.rotr(65536,2)); //  == 16384
print(a.rotl(0xff00, 24)); // == 255
print(a.rotl(0x80000000, 2)); // == 2
print(a.rotr(0x00000001, 1)); // == -2147483648
