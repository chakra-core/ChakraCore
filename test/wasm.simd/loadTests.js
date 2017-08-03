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

function testLoadOpsForType(funcname, module, laneValues, expectedResults, startPositions) {

    let memObj = new WebAssembly.Memory({initial:INITIAL_SIZE});
    const instance = new WebAssembly.Instance(module, { "dummy" : { "memory" : memObj } }).exports;

    let intArray = new Int32Array (memObj.buffer);

    let forEachTestPosition =  (action)  => {

            for (const pos of startPositions) {
            for (let i = 0; i < 4; i++) {
                action(pos, i); 
            }
        }
    };
    
    forEachTestPosition ((pos, i) => {intArray[pos + i]  = laneValues[i];});
    instance[funcname](0);
    forEachTestPosition((pos, i) => {assertEquals(intArray[pos + i], expectedResults[i]);});

    const MEM_SIZE_IN_BYTES = 1024 * 64;

    let check = function(expected, funName, ...args) {
        let fun = eval(funName);
        var result;
        try {
        result = fun(...args);
        
        }
        catch (e) {
            if (e.message === "Access index is out of range" || 
                e.message === "Simd typed array access: argument out of range" ||
                e.message === "argument out of range" ||
                e.message === "Memory index is out of range"
            ) {
                result = "Access index is out of range";
            }
            else {
                result = e.message;
            }
        }

        if(result != expected)
        {
            passed = false;
            print(`${funName}(${[...args]}) produced ${result}, expected ${expected}`);
        }
    }
    
    check(0, "instance.m128_load4", MEM_SIZE_IN_BYTES - 32);
    check(0, "instance.m128_load4", MEM_SIZE_IN_BYTES - 16);
    check("Access index is out of range", "instance.m128_load4", MEM_SIZE_IN_BYTES - 8);
    check("Access index is out of range", "instance.m128_load4", MEM_SIZE_IN_BYTES - 4);
    check("Access index is out of range", "instance.m128_load4_offset", 0xFFFFFFFC);
    check("Access index is out of range", "instance.m128_load4_offset", -1);

}

const INITIAL_SIZE = 1;
const module = new WebAssembly.Module(readbuffer('loads.wasm'));

const laneValues = [0xAAAAAAAA, 0xFFFFFFFF, 0X80000000, 0x90A762A6];
const expectedResults = [16, 32, 1, 14]; //i32.popcnt
const startPositions = [0, 5, 11, 17];

testLoadOpsForType("m128_load_test", module, laneValues, expectedResults,startPositions);

if (passed) {
    print("Passed");
}