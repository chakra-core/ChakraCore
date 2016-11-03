//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let passed = true;
let check = function(expected, funName, ...args)
{
    let fun = eval(funName);
    let result = fun(...args);
    if(result != expected)
    {
        passed = false;
        print(`${funName}(${[...args]}) produced ${result}, expected ${expected}`);
    }
}


let ffi = {};
var mod = new WebAssembly.Module(readbuffer('math.wasm'));
var exports = new WebAssembly.Instance(mod, ffi).exports;
check(0, "exports.ctz", 1);
check(2, "exports.ctz", 4);
check(31, "exports.ctz", -Math.pow(2,31));
check(32, "exports.ctz", 0);


if(passed)
{
    print("Passed");
}
