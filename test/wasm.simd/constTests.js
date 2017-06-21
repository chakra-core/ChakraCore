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
const arr = new Uint32Array (memObj.buffer);

const module = new WebAssembly.Module(readbuffer('const.wasm'));
const instance = new WebAssembly.Instance(module, { "dummy" : { "memory" : memObj } }).exports;

let testIntLogicalOps = function (funcname, resultArr) {
    const len = 4
    instance[funcname]();
    for (let i = 0; i < len; i++) {
        assertEquals(arr[i], resultArr[i]);
    }
}

testIntLogicalOps("m128_const_1", [0, 0xFF00ABCC, 0, 0]);
testIntLogicalOps("m128_const_2", [0xA100BC00, 0xFFFFFFFF, 0xFF00, 0x1]);
testIntLogicalOps("m128_const_3", [0xFFFFFFFF, 0xFFFFFFFF, 0, 0xFFFFFFFF]);
testIntLogicalOps("m128_const_4", [0, 0, 0, 0]);

if (passed) {
    print("Passed");
}
