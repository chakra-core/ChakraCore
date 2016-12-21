//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var mod = new WebAssembly.Module(readbuffer('table.wasm'));
var a1 = new WebAssembly.Instance(mod, {"m" : {x : 10}}).exports;

//local offset
print(a1.call(1));
print(a1.call(2));
print(a1.call(3));
print(a1.call(4));

//global offset
/* const global in init expr not supported in MVP
print(a1.call(6));
print(a1.call(7));
print(a1.call(8));
print(a1.call(9));
*/

//imported global offset
print(a1.call(10));
print(a1.call(11));
print(a1.call(12));
print(a1.call(13));

