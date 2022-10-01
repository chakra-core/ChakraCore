//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2022 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check
/// <reference path="../UnitTestFramework/UnitTestFramework.js" />

WScript.LoadScriptFile("../UnitTestFramework/UnitTestFramework.js");

const tests = [
    {
        name: "Uint8Array.prototype.sort basic properties",
        body() {
            assert.areEqual(1, Uint8Array.prototype.sort.length, "Uint8Array.prototype.sort should have length of 1");
            assert.areEqual("sort", Uint8Array.prototype.sort.name, "Uint8Array.prototype.sort should have name 'sort'");
            const desc = Object.getOwnPropertyDescriptor(Uint8Array.prototype.__proto__, "sort");
            assert.isTrue(desc.writable, "Uint8Array.prototype.sort.writable === true");
            assert.isFalse(desc.enumerable, "Uint8Array.prototype.sort.enumerable === false");
            assert.isTrue(desc.configurable, "Uint8Array.prototype.sort.configurable === true");
            assert.areEqual('function', typeof desc.value, "typeof Uint8Array.prototype.sort === 'function'");
            assert.throws(() => { [].sort("not a function"); }, TypeError);
            assert.throws(() => { [].sort(null); }, TypeError);
            assert.doesNotThrow(() => { [].sort(undefined); }, "Uint8Array.prototype.sort with undefined sort parameter does not throw");
        }
    },
    {
        name: "Uint8Array.prototype.sort basic sort cases with arrays of numbers",
        body() {
            const arrayOne = [120, 5, 8, 4, 6, 9, 9, 10, 2, 3];
            assert.areEqual([10, 120, 2, 3, 4, 5, 6, 8, 9, 9], arrayOne.sort(undefined), "Uint8Array.sort with default comparator should sort based on string ordering");
            const result = [2, 3, 4, 5, 6, 8, 9, 9, 10, 120];
            assert.areEqual(result, arrayOne.sort((x, y) => x - y), "Uint8Array.sort with numerical comparator should sort numerically");
            assert.areEqual(result, arrayOne, "Uint8Array.sort should sort original array as well as return value");
            assert.areEqual(result, arrayOne.sort((x, y) => { return (x > y) ? 1 : ((x < y) ? -1 : 0); }), "1/-1/0 numerical comparison should be the same as x - y");
            const arrayTwo = [25, 8, 7, 41];
            assert.areEqual([41, 25, 8, 7], arrayTwo.sort((a, b) => b - a), "Uint8Array.sort with (a,b)=>b-a should sort descending");
            assert.areEqual([7, 8, 25, 41], arrayTwo.sort((a, b) => a - b), "Uint8Array.sort with (a,b)=>a-b should sort ascending");
            assert.areEqual([1, 1.2, 4, 4.8, 12], [1, 1.2, 12, 4.8, 4].sort((a, b) => a - b), "Uint8Array.sort with numerical comparator handles floats correctly");
        }
    },
    {
        name: "Uint8Array.prototype.sort with a compare function with side effects",
        body() {
            let xyz = 5;
            function setXyz() { xyz = 10; return 0; }
            [].sort(setXyz);
            assert.areEqual(5, xyz, "Uint8Array.sort does not call compare function when length is 0");
            [1].sort(setXyz)
            assert.areEqual(5, xyz, "Uint8Array.sort does not call compare function when length is 1");
            [undefined, undefined, undefined, undefined].sort(setXyz);
            assert.areEqual(5, xyz, "Uint8Array.sort does not call compare function when all elements are undefined");
            [5, undefined, , undefined].sort(setXyz);
            assert.areEqual(5, xyz, "Uint8Array.sort does not call compare function when only one element is defined");
            [1, 2, undefined].sort(setXyz);
            assert.areEqual(10, xyz, "Uint8Array.sort calls compare function if there is > 1 defined element");
        }
    },
    {
        name: "Uint8Array.prototype.sort default comparison should not call valueOf",
        body() {
            valueOf = false;
            const arr = [{
                valueOf() { valueOf = true; return 0; }
            }, 1, 1, 1,];
            arr.sort();
            assert.isFalse(valueOf);
        }
    }
];

//assert.areEqual does not work for directly comparing sparse arrays
function compareSparseArrays(arrayOne, arrayTwo, message) {
    const len = arrayOne.length;
    assert.areEqual(len, arrayTwo.length, message + " lengths are not the same");
    for (let i = 0; i < len; ++i) {
        assert.areEqual(arrayOne[i], arrayTwo[i], message + " property " + i + " is not the same");
    }
}

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
