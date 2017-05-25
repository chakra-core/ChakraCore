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
const arrays = [new Int32Array (memObj.buffer),
    new Int16Array (memObj.buffer),
    new Int8Array (memObj.buffer),
    new Float32Array (memObj.buffer)
];

const module = new WebAssembly.Module(readbuffer('replace.wasm'));
const instance = new WebAssembly.Instance(module, { "dummy" : { "memory" : memObj } }).exports;

let testIntFloatReplace = function (funcname, splat, val, len) {

    let typeIndex = Math.log2(len) - 2;
    let arr = arrays [typeIndex];

    if (len == 32) { //special case for floats
        len = 4;
    }

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

testIntFloatReplace("i32x4_replace", -1, -2147483648, 4);
testIntFloatReplace("i32x4_replace", 7, -1, 4);
testIntFloatReplace("i32x4_replace", 0, 2147483647, 4);

testIntFloatReplace("i16x8_replace", -1, -32768, 8);
testIntFloatReplace("i16x8_replace", 1001, -1, 8);
testIntFloatReplace("i16x8_replace", 0, 32767, 8);

testIntFloatReplace("i8x16_replace", -1, -128, 16);
testIntFloatReplace("i8x16_replace", 100, -1, 16);
testIntFloatReplace("i8x16_replace", 0, 127, 16);

testIntFloatReplace("f32x4_replace", Number.NEGATIVE_INFINITY, 0.125, 32);
testIntFloatReplace("f32x4_replace", 777.0, 1001.0, 32);
testIntFloatReplace("f32x4_replace", -1.0, Number.POSITIVE_INFINITY, 32);
testIntFloatReplace("f32x4_replace", -100.0, 1.7014118346046924e+38, 32);

if (passed) {
    print("Passed");
}
