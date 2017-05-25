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
const arrays = [new Int32Array (memObj.buffer), 
    new Int16Array (memObj.buffer), 
    new Int8Array (memObj.buffer), 
    new Float32Array (memObj.buffer)
];

//SPLAT
const splatModule = new WebAssembly.Module(readbuffer('splat.wasm'));
const splatInstance = new WebAssembly.Instance(splatModule, { "dummy" : { "memory" : memObj } }).exports;

let testSplat = function (funcname, val, len, checkFun) {

    let typeIndex = Math.log2(len) - 2;
    let arr = arrays [typeIndex];

    if (len == 32) { //special case for floats
        len = 4;        
    }
    
    for (let i = 0; i < len; i++) {
        arr[i] = val;
    }

    splatInstance[funcname](0, val);

    for (let i = 0; i < len; i++) {
        assertEquals (val, arr[i]);
    }

}

testSplat("i32x4_splat", 0, 4);
testSplat("i32x4_splat", -1, 4);
testSplat("i32x4_splat", -2147483648, 4);

testSplat("i16x8_splat", 0, 8);
testSplat("i16x8_splat", -1, 8);
testSplat("i16x8_splat", 32766, 8);

testSplat("i8x16_splat", 0, 16);
testSplat("i8x16_splat", -1, 16);
testSplat("i8x16_splat", -128, 16);

testSplat("f32x4_splat", 0.125, 32);
testSplat("f32x4_splat", 1001.0, 32);
testSplat("f32x4_splat", Number.POSITIVE_INFINITY, 32);
testSplat("f32x4_splat", 1.7014118346046924e+38, 32);

//NEG
const module = new WebAssembly.Module(readbuffer('neg.wasm'));
const instance = new WebAssembly.Instance(module, { "dummy" : { "memory" : memObj } }).exports;

let testNeg = function (funcname, val, len) {
    
    let typeIndex = Math.log2(len) - 2; 
    let arr = arrays [typeIndex];

    if (len == 32) { //special case for floats
        len = 4;        
    }
    
    for (let i = 0; i < len; i++) {
        arr[i] = val;
    }

    instance[funcname](0, val);

    for (let i = 0; i < len; i++) {
        assertEquals(-val, arr[i]);
    }
};

testNeg("i32x4_neg", 0, 4);
testNeg("i32x4_neg", 1, 4);
testNeg("i32x4_neg", 2147483647, 4);

testNeg("i16x8_neg", 0, 8);
testNeg("i16x8_neg", 1, 8);
testNeg("i16x8_neg", 32767, 8);

testNeg("i8x16_neg", 0, 16);
testNeg("i8x16_neg", 1, 16);
testNeg("i8x16_neg", 127, 16);

testNeg("f32x4_neg", 0.0, 32);
testNeg("f32x4_neg", 1.0, 32);
testNeg("f32x4_neg", 1000.0, 32);
testNeg("f32x4_neg", Number.POSITIVE_INFINITY, 32);

if (passed) {
    print("Passed");
}
