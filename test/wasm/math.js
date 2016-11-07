//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let passed = true;
let check = function(expected, fun, funName, ...args)
{
    let result = fun(...args);
    if(result !== expected)
    {
        passed = false;
        print(`${funName}(${[...args]}) produced ${result}, expected ${expected}`);
    }
}


let ffi = {};
let wasm = readbuffer("math.wasm");
const {exports: {ctz, ctzI64}} = Wasm.instantiateModule(new Uint8Array(wasm), ffi);

const customCtzI64 = WebAssembly.nativeTypeCallTest.bind(null, ctzI64);

check(0, ctz, "ctz", 1);
check(2, ctz, "ctz", 4);
check(31, ctz, "ctz", -Math.pow(2,31));
check(32, ctz, "ctz", 0);
check("64", customCtzI64, "ctzI64", 0);
check("64", customCtzI64, "ctzI64", "0");
check("0", customCtzI64, "ctzI64", "1");
check("31", customCtzI64, "ctzI64", "" + -Math.pow(2,31));
check("58", customCtzI64, "ctzI64", "0x3400000000000000");
check("63", customCtzI64, "ctzI64", "-9223372036854775808");


if(passed)
{
    print("Passed");
}
