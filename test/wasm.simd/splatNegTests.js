//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let passed = true;

function assertEquals(expected, actual) {
    if (expected != actual) {
        passed = false;
        throw `Expected ${expected}, received ${actual}`;
    }
}

let check = function(expected, funName, ...args) {
    let fun = eval(funName);
    var result;
    try {
       result = fun(...args);
    }
    catch (e) {
        result = e.name;
    }

    if(result != expected)
    {
        passed = false;
        print(`${funName}(${[...args]}) produced ${result}, expected ${expected}`);
    }
}

const INITIAL_SIZE = 1;
const memObj = new WebAssembly.Memory({initial:INITIAL_SIZE});
const arrays = {
    "i32x4" : { arr : new Int32Array (memObj.buffer)   , len : 4 } ,
    "i16x8" : { arr : new Int16Array (memObj.buffer)   , len : 8 } ,
    "i8x16" : { arr : new Int8Array (memObj.buffer)    , len : 16 } ,
    "f32x4" : { arr : new Float32Array (memObj.buffer) , len : 4 },
    "f64x2" : { arr : new Float64Array (memObj.buffer) , len : 2 }
};

//SPLAT
const splatModule = new WebAssembly.Module(readbuffer('splat.wasm'));
const splatInstance = new WebAssembly.Instance(splatModule, { "dummy" : { "memory" : memObj } }).exports;

let testSplat = function (funcname, val) {

    const type = funcname.split('_')[0];
    const arr = arrays [type].arr;
    const len = arrays [type].len;

    for (let i = 0; i < len; i++) {
        arr[i] = val;
    }

    splatInstance[funcname](0, val);

    for (let i = 0; i < len; i++) {
        assertEquals (val, arr[i]);
    }

}

testSplat("i32x4_splat", 0);
testSplat("i32x4_splat", -1);
testSplat("i32x4_splat", -2147483648);

testSplat("i16x8_splat", 0);
testSplat("i16x8_splat", -1);
testSplat("i16x8_splat", 32766);

testSplat("i8x16_splat", 0);
testSplat("i8x16_splat", -1);
testSplat("i8x16_splat", -128);

testSplat("f32x4_splat", 0.125);
testSplat("f32x4_splat", 1001.0);
testSplat("f32x4_splat", Number.POSITIVE_INFINITY);
testSplat("f32x4_splat", 1.7014118346046924e+38);

testSplat("f64x2_splat", 0.125);
testSplat("f64x2_splat", 1001.0);
testSplat("f64x2_splat", Number.POSITIVE_INFINITY);
testSplat("f64x2_splat", 1.7014118346046924e+38);

//NEG
const module = new WebAssembly.Module(readbuffer('neg.wasm'));
const instance = new WebAssembly.Instance(module, { "dummy" : { "memory" : memObj } }).exports;


let testNeg = function (funcname, val) {

    const type = funcname.split('_')[0];
    const arr = arrays [type].arr;
    const len = arrays [type].len;

    for (let i = 0; i < len; i++) {
        arr[i] = val;
    }

    instance[funcname](0, val);

    for (let i = 0; i < len; i++) {
        assertEquals(-val, arr[i]);
    }
};

testNeg("i32x4_neg", 0);
testNeg("i32x4_neg", 1);
testNeg("i32x4_neg", 2147483647);

testNeg("i16x8_neg", 0);
testNeg("i16x8_neg", 1);
testNeg("i16x8_neg", 32767);

testNeg("i8x16_neg", 0);
testNeg("i8x16_neg", 1);
testNeg("i8x16_neg", 127);

testNeg("f32x4_neg", 0.0);
testNeg("f32x4_neg", 1.0);
testNeg("f32x4_neg", 1000.0);
testNeg("f32x4_neg", Number.POSITIVE_INFINITY, 32);

testNeg("f64x2_neg", -0.0);
testNeg("f64x2_neg", 1.0);
testNeg("f64x2_neg", 1234.56);
testNeg("f64x2_neg", Number.POSITIVE_INFINITY);

if (passed) {
    print("Passed");
}
