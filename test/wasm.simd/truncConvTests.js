//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let passed = true;

function assertEquals(expected, actual) {
    if (expected != actual) {
        passed = false;
        throw { message : `Expected ${expected}, received ${actual}`};
    }
}

const INITIAL_SIZE = 1;
const memObj = new WebAssembly.Memory({initial:INITIAL_SIZE});
const arrays = {
    "i32x4" : new Int32Array (memObj.buffer),
    "f32x4" : new Float32Array (memObj.buffer)
};

const module = new WebAssembly.Module(readbuffer('truncconv.wasm'));
const instance = new WebAssembly.Instance(module, { "dummy" : { "memory" : memObj } }).exports;

let testTruncConvTests = function (funcname, args1, resultArr) {

    const len = args1.length;
    const inputIndex = conversionType(funcname.split("_")[0]);
    const outputIndex = funcname.split("_")[0];

    const inputArr = arrays[inputIndex];
    const outputArr = arrays[outputIndex];

    moveArgsIntoArray(args1, 0, inputArr);
    instance[funcname]();
    moveArgsIntoArray(resultArr, len, outputArr); //should come after instance[funcname]() for checkThrows to work

    for (let i = 0; i < len; i++) {
        assertEquals(outputArr[i], outputArr[i + len]);
    }

    return true; //otherwise will throw an exception
}

let checkInternal = function(expected, ...args) {

    var result;
    try {
    result = testTruncConvTests(...args);
    }
    catch (e) {
        result = e.message.replace("SIMD.Int32x4.FromFloat32x4: ", "");
    }

    if(result != expected)
    {
        passed = false;
        print(`testTruncConvTests(${[...args]}) produced ${result}, expected ${expected}`);
    }
}

let checkThrows = function(expected, ...args) {
    checkInternal(expected, ...args);
}

let check = function (...args) {
    checkInternal(true, ...args);
}

function moveArgsIntoArray(args, offset, arr) {
    for (let i = 0; i < args.length; i++) {
        arr[offset + i] = args[i];
    }
}


let conversionType = function (type) {
    return type == "i32x4" ? "f32x4" : "i32x4";
}

check("i32x4_trunc_s", [2147483520.0, -1, 1.25, -0.25], [2147483520, -1, 1, 0]);
checkThrows("argument out of range", "i32x4_trunc_s", [2147483520.0, -1, Number.NaN, -0.25]);
checkThrows("argument out of range", "i32x4_trunc_s", [2147483520.0, 2147483647.0, 1.25, -0.25]);
checkThrows("argument out of range", "i32x4_trunc_s", [-4294967040.0, -1, 1.25, -0.25]);

check("i32x4_trunc_u", [4294967040.0, 2147483520.0, 1.25, 0.25], [4294967040, 2147483520, 1, 0]);
checkThrows("argument out of range", "i32x4_trunc_u", [4294967040.0, 2147483520.0, Number.NaN, 0.25]);
checkThrows("argument out of range", "i32x4_trunc_u", [4294967040.0, 4294967296.0, 1.25, 0.25]);
checkThrows("argument out of range", "i32x4_trunc_u", [4294967040.0, 2147483520.0, 1.25, -1]);

check("f32x4_convert_s", [2147483647, -2147483647, 0, 65535], [2.14748365e+09, -2.14748365e+09, 0, 65535]);
check("f32x4_convert_s", [101, 1003, -10007, -65535], [101, 1003, -10007, -65535]);
check("f32x4_convert_u", [2147483647, 4294967295, 0, 65535], [2.14748365e+09, 4.29496730e+09, 0, 65535]);
check("f32x4_convert_u", [32767, 9999, 100003, 1], [32767, 9999, 100003, 1]);

if (passed) {
    print("Passed");
}
