//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var mod = new WebAssembly.Module(readbuffer('rot.wasm'));
var a = new WebAssembly.Instance(mod).exports;
print(a.rotl(11,2)); // == 44
print(a.rotl(65536,2)); // == 262144
print(a.rotr(65536,2)); //  == 16384
print(a.rotl(0xff00, 24)); // == 255
print(a.rotl(0x80000000, 2)); // == 2
print(a.rotr(0x00000001, 1)); // == -2147483648
