//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let passed = true;
let check = function(expected, funName, ...args)
{
    let fun = eval(funName);
    var result;
    try {
       result = fun(...args);
    }
    catch (e) {
        result = e.message;
    }

    if(result != expected)
    {
        passed = false;
        print(`${funName}(${[...args]}) produced ${result}, expected ${expected}`);
    }
}


let ffi = {};
let wasm = readbuffer("trunc.wasm");
let exports = Wasm.instantiateModule(new Uint8Array(wasm), ffi).exports;

//i64 div/rem
check(1,"exports.test1");
check("Overflow","exports.test2");
check(1,"exports.test3");
check(1,"exports.test4");
check("Overflow","exports.test5");

check(1,"exports.test6");
check("Overflow","exports.test7");
check(1,"exports.test8");
check("Overflow","exports.test9");

check(1,"exports.test10");
check("Overflow","exports.test11");
check(1,"exports.test12");
check(1,"exports.test13");
check("Overflow","exports.test14");

check(1,"exports.test15");
check("Overflow","exports.test16");
check(1,"exports.test17");
check("Overflow","exports.test18");


if(passed)
{
    print("Passed");
}
