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

const INITIAL_SIZE = 1;
const memObj = new WebAssembly.Memory({initial:INITIAL_SIZE});
const arrays = {
    "i32x4" : { arr : new Int32Array (memObj.buffer)   , len : 4 } ,
    "i16x8" : { arr : new Int16Array (memObj.buffer)   , len : 8 } ,
    "i8x16" : { arr : new Int8Array (memObj.buffer)    , len : 16 } ,
    "f32x4" : { arr : new Float32Array (memObj.buffer) , len : 4 } ,
    "f64x2" : { arr : new Float64Array (memObj.buffer) , len : 2 }
};

const module = new WebAssembly.Module(readbuffer('replace.wasm'));
const instance = new WebAssembly.Instance(module, { "dummy" : { "memory" : memObj } }).exports;

let testIntFloatReplace = function (funcname, splat, val) {

    const type = funcname.split('_')[0];
    const arr = arrays [type].arr;
    const len = arrays [type].len;

    for (let i = 0; i < len; i++) {
        instance[funcname+i](splat, val);
        for (let j = 0; j < len; j++) {
            if (i != j) {
                assertEquals(splat, arr[j]);
            }
            else {
                assertEquals(val, arr[j]);
            }
        }
    }
}

testIntFloatReplace("i32x4_replace", -1, -2147483648);
testIntFloatReplace("i32x4_replace", 7, -1);
testIntFloatReplace("i32x4_replace", 0, 2147483647);

testIntFloatReplace("i16x8_replace", -1, -32768);
testIntFloatReplace("i16x8_replace", 1001, -1);
testIntFloatReplace("i16x8_replace", 0, 32767);

testIntFloatReplace("i8x16_replace", -1, -128);
testIntFloatReplace("i8x16_replace", 100, -1);
testIntFloatReplace("i8x16_replace", 0, 127);

testIntFloatReplace("f32x4_replace", Number.NEGATIVE_INFINITY, 0.125);
testIntFloatReplace("f32x4_replace", 777.0, 1001.0);
testIntFloatReplace("f32x4_replace", -1.0, Number.POSITIVE_INFINITY);
testIntFloatReplace("f32x4_replace", -100.0, 1.7014118346046924e+38);

testIntFloatReplace("f64x2_replace", Number.NEGATIVE_INFINITY, 0.125);
testIntFloatReplace("f64x2_replace", 777.0, 1001.0);
testIntFloatReplace("f64x2_replace", -1.0, Number.POSITIVE_INFINITY);
testIntFloatReplace("f64x2_replace", -100.0, 1.7014118346046924e+38);

if (passed) {
    print("Passed");
}
