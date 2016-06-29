//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = Wasm.instantiateModule(readbuffer('basic.wasm'), {test: {foo: function(a){print(a); return 2;}}});
print(a.exports.a(11));
print(a.exports.a(11));
var b = 0;
var c = new Int32Array(a.exports.memory);
for(var i=0; i<10000; i++)
{
    b+= c[i];
}
print(b);
