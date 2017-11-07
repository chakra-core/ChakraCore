//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WebAssembly.compile(readbuffer('basic.wasm'))
.then(function(val)
{
    var a = new WebAssembly.Instance(val, {test: {foo: function(a){print(a); return 2;}}});
    print(a.exports.a);
    print(a.exports.a(11));
    print(a.exports.a(11));
    let instance2 = new WebAssembly.Instance(val, {test: {foo: function(a){print(a-1); return 3;}}});
    print(instance2.exports.a(11));
    print(instance2.exports.a(11));
    var b = 0;
    var c = new Int32Array(a.exports.memory.buffer);
    for(var i=0; i<10000; i++)
    {
        b+= c[i];
    }
    print(b);

})
.catch(function(reason){print(reason)});
