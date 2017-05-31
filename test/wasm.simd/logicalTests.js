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
];

const module = new WebAssembly.Module(readbuffer('logical.wasm'));
const instance = new WebAssembly.Instance(module, { "dummy" : { "memory" : memObj } }).exports;

function moveArgsIntoArray(args, offset, arr) {
    for (let i = 0; i < args.length; i++) {
        arr[offset + i] = args[i];
    }
}

let testIntLogicalOps = function (funcname, args1, args2, resultArr) {

    const len = args1.length;
    const typeIndex = Math.log2(len) - 2;
    let arr = arrays [typeIndex];

    moveArgsIntoArray(args1, 0, arr);
    moveArgsIntoArray(args2, len, arr);
    moveArgsIntoArray(resultArr, 2 * len, arr);

    instance[funcname]();

    const resultOffset = 2 * len;
    for (let i = 0; i < len; i++) {
        assertEquals(arr[i], arr[resultOffset + i]);
    }
}

testIntLogicalOps("m128_and", [-1, 0x80007001, 0, 7], [-1, 0x80007001, 0, 7], [-1, 0x80007001, 0, 7]);
testIntLogicalOps("m128_and", [-1, 0x80000001, 0x80007001, 7], [0, 0x80000001, -1 ^ 0x80007001, 7], [0, 0x80000001, 0, 7]);

testIntLogicalOps("m128_or", [-1, 0xFFFF0000, 0x80000001, 0x55555555], [0, 0x0000FFFF, 0x80000001 ^ -1, 0xaaaaaaaa], [-1, -1, -1, -1]);
testIntLogicalOps("m128_or", [0xFFFF0000, 0, 0x80007001, 0], [0, 0, 0, 0x55555555], [0xFFFF0000, 0, 0x80007001, 0x55555555]);

testIntLogicalOps("m128_xor", [-1, 0xFFFF0000, 0x80000001, 0x55555555], [0, 0x0000FFFF, 0x80000001 ^ -1, 0xaaaaaaaa], [-1, -1, -1, -1]);
testIntLogicalOps("m128_xor", [0xFFFF0000, 0x55555555, -1, 0], [0, 0xaaaaaaaa, 0xFFFF0000, 0x55555555], [0xFFFF0000, -1, 0x0000FFFF, 0x55555555]);

testIntLogicalOps("m128_not", [-1, 0xFFFF0000, 0x80000001, 0x55555555], [0, 0, 0, 0] /*dummy*/, [0, 0x0000FFFF, 0x80000001 ^ -1, 0xaaaaaaaa]);

let testBoolReduceOps = function (funcname, args1, expected) {

    const len = args1.length;
    let typeIndex = Math.log2(len) - 2;
    let arr = arrays [typeIndex];

    moveArgsIntoArray(args1, 0, arr);
    let result = instance[funcname]();
    assertEquals(expected, result);
}

testBoolReduceOps("i32x4_anytrue", [0, 0, 0, 0], false);
testBoolReduceOps("i32x4_anytrue", [0, 0, 0, 0xFFFFFFFF], true);
testBoolReduceOps("i32x4_anytrue", [0xFFFFFFFF, 0, 0, 0], true);

testBoolReduceOps("i32x4_alltrue", [0, 0, 0, 0], false);
testBoolReduceOps("i32x4_alltrue", [0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0], false);
testBoolReduceOps("i32x4_alltrue", [0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF], true);

testBoolReduceOps("i16x8_anytrue", [0, 0, 0, 0, 0, 0, 0, 0], false);
testBoolReduceOps("i16x8_anytrue", [0, 0, 0, 0, 0, 0, 0xFFFF, 0], true);
testBoolReduceOps("i16x8_anytrue", [0, 0, 0xFFFF, 0, 0, 0, 0, 0], true);

testBoolReduceOps("i16x8_alltrue", [0, 0, 0, 0, 0, 0, 0, 0], false);
testBoolReduceOps("i16x8_alltrue", [0, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF], false);
testBoolReduceOps("i16x8_alltrue", [0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF], true);

testBoolReduceOps("i8x16_anytrue", [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], false);
testBoolReduceOps("i8x16_anytrue", [0, 0, 0, 0, 0, 0, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0], true);
testBoolReduceOps("i8x16_anytrue", [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0, 0], true);

testBoolReduceOps("i8x16_alltrue", [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], false);
testBoolReduceOps("i8x16_alltrue", [0xFF, 0xFF, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF], false);
testBoolReduceOps("i8x16_alltrue", [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF], true);

if (passed) {
    print("Passed");
}
