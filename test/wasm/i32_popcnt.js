//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var mod = new WebAssembly.Module(readbuffer('i32_popcnt.wasm'));
var a = new WebAssembly.Instance(mod).exports;
print(a.popcount(3)); 
print(a.popcount(7)); 
print(a.popcount(8));
print (a.popcount(-1));
print (a.popcount(0));


