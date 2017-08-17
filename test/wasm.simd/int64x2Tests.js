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
const array = new Int32Array (memObj.buffer);

const int64Module = new WebAssembly.Module(readbuffer('int64x2.wasm'));
const int64Instance = new WebAssembly.Instance(int64Module, { "dummy" : { "memory" : memObj } }).exports;

function moveArgsIntoArray(args, offset, arr) {
    for (let i = 0; i < args.length; i++) {
        arr[offset + i] = args[i];
    }
}

let testInt64MathOps = function (funcname, inputArr, funcArgs, resultArr) {

    moveArgsIntoArray(inputArr, 0, array);
    int64Instance[funcname](...funcArgs);

    for (let i = 0; i < resultArr.length; i++) {
        assertEquals(array[i], resultArr[i]);
    }

}

testInt64MathOps("func_i64x2_splat", [0xF0F0F0F0, 0xAABBAABB], [],  [0xF0F0F0F0|0, 0xAABBAABB|0, 0xF0F0F0F0|0, 0xAABBAABB|0]);
testInt64MathOps("func_i64x2_splat", [0xBEEF0000, 0xCAFEFFFF], [],  [0xBEEF0000|0, 0xCAFEFFFF|0, 0xBEEF0000|0, 0xCAFEFFFF|0]);
testInt64MathOps("func_i64x2_extractlane_0", [0xDEADDEAD, 0xABBAABBA, 0x0, 0x0], [],  [0xDEADDEAD|0, 0xABBAABBA|0, 0xDEADDEAD|0, 0xABBAABBA|0]);
testInt64MathOps("func_i64x2_extractlane_1", [0x0, 0x0, 0xDEADDEAD, 0xABBAABBA], [],  [0xDEADDEAD|0, 0xABBAABBA|0, 0xDEADDEAD|0, 0xABBAABBA|0]);
testInt64MathOps("func_i64x2_replacelane_0", [0xDEADDEAD, 0xABBAABBA, 0xBEEF0000, 0xCAFEFFFF, 0xCCCCCCCC, 0xDDDDDDDD], [],  [0xCCCCCCCC|0, 0xDDDDDDDD|0, 0xBEEF0000|0, 0xCAFEFFFF|0]);
testInt64MathOps("func_i64x2_replacelane_1", [0xDEADDEAD, 0xABBAABBA, 0xBEEF0000, 0xCAFEFFFF, 0xCCCCCCCC, 0xDDDDDDDD], [],  [0xDEADDEAD|0, 0xABBAABBA|0, 0xCCCCCCCC|0, 0xDDDDDDDD|0]);
testInt64MathOps("func_i64x2_add", [0x00000001, 0x000FF000, 0xC, 0xA, 0xFFFFFFFF, 0xFF00000E, 0x3, 0x5], [],  [0x0, 0xFF0FF00F|0, 0xF, 0xF]);
testInt64MathOps("func_i64x2_sub", [00000003, 0xD0CFB000, 0xC, 0xA, 0xFFFFFFFF, 0xD0BA0000, 0x3, 0x5], [],  [0x00000004, 0x0015afff, 0x9, 0x5]);
testInt64MathOps("func_i64x2_neg", [0x0, 0x80000000, 0xFFFFFFFF, 0x7FFFFFFF], [],  [0x0, 0x80000000|0, 0x00000001, 0x80000000|0]);
testInt64MathOps("func_i64x2_shl", [0x00100001, 0x80000001, 0xFFFFFFFF, 0xFFFFFFFF], [3],  [0x00800008|0, 0x00000008, 0xFFFFFFF8|0, 0xFFFFFFFF|0]);
testInt64MathOps("func_i64x2_shl", [0x00100001, 0x80000001, 0xFFFFFFFF, 0xFFFFFFFF], [67],  [0x00800008|0, 0x00000008, 0xFFFFFFF8|0, 0xFFFFFFFF|0]);
testInt64MathOps("func_i64x2_shr_u", [0x00100001, 0x80000001, 0xFFFFFFFF, 0xFFFFFFFF], [3],  [0x20020000|0, 0x10000000|0, 0xFFFFFFFF|0, 0x1FFFFFFF|0]);
testInt64MathOps("func_i64x2_shr_u", [0x00100001, 0x80000001, 0xFFFFFFFF, 0xFFFFFFFF], [67],  [0x20020000|0, 0x10000000|0, 0xFFFFFFFF|0, 0x1FFFFFFF|0]);
testInt64MathOps("func_i64x2_shr_s", [0x00100001, 0x80000001, 0xFFFFFFFF, 0xFFFFFFFF], [3],  [0x20020000|0, 0xf0000000|0, 0xFFFFFFFF|0, 0xFFFFFFFF|0]);
testInt64MathOps("func_i64x2_shr_s", [0x00100001, 0x80000001, 0xFFFFFFFF, 0xFFFFFFFF], [67],  [0x20020000|0, 0xf0000000|0, 0xFFFFFFFF|0, 0xFFFFFFFF|0]);
testInt64MathOps("func_i64x2_anytrue", [0x0, 0x80000000, 0x0, 0x0], [],  [0x1]);
testInt64MathOps("func_i64x2_anytrue", [0x0, 0x00000000, 0x0, 0x1], [],  [0x1]);
testInt64MathOps("func_i64x2_anytrue", [0x0, 0x0, 0x0, 0x0], [],  [0x0]);
testInt64MathOps("func_i64x2_alltrue", [0x1, 0x0, 0x0, 0x1], [],  [0x1]);
testInt64MathOps("func_i64x2_alltrue", [0x0, 0x1, 0x1, 0x0], [],  [0x1]);
testInt64MathOps("func_i64x2_alltrue", [0x0, 0x0, 0x1, 0x1], [],  [0x0]);
testInt64MathOps("func_i64x2_alltrue", [0x1, 0x0, 0x0, 0x0], [],  [0x0]);

if (passed) {
    print("Passed");
}