//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const ArrayPrototype = Array.prototype;
const at = ArrayPrototype.at;

const call = Function.call.bind(Function.call); // (fn: Function, thisArg: any, ...args: any[]) => any


const tests = [
    {
        name: "Existence of Array.prototype.at",
        body: function () {
            assert.isTrue(typeof at === "function", "Array.prototype.at should be a function");
        }
    },
    {
        name: "Array.prototype.at's descriptor",
        body: function () {
            const desc = Object.getOwnPropertyDescriptor(ArrayPrototype, "at");
            assert.areEqual(desc.configurable, true, "Array.prototype.at should be configurable");
            assert.areEqual(desc.writable, true, "Array.prototype.at should be writable");
            assert.areEqual(desc.enumerable, false, "Array.prototype.at should not be enumerable");
        }
    },
    {
        name: "Properties of Array.prototype.at",
        body: function () {
            const nameDesc = Object.getOwnPropertyDescriptor(at, "name");
            assert.areEqual(at.name, "at", 'Array.prototype.at.name should be "at"');
            assert.areEqual(nameDesc.configurable, true, "Array.prototype.at.name should be configurable");
            assert.areEqual(nameDesc.writable, false, "Array.prototype.at.name should not be writable");
            assert.areEqual(nameDesc.enumerable, false, "Array.prototype.at.name should not be enumerable");
            const lengthDesc = Object.getOwnPropertyDescriptor(at, "length");
            assert.areEqual(at.length, 1, 'Array.prototype.at.length should be 1');
            assert.areEqual(lengthDesc.configurable, true, "Array.prototype.at.length should be configurable");
            assert.areEqual(lengthDesc.writable, false, "Array.prototype.at.length should not be writable");
            assert.areEqual(lengthDesc.enumerable, false, "Array.prototype.at.length should not be enumerable");
        }
    },
    {
        name: "Array.prototype.at called on nullish object",
        body: function () {
            assert.throws(() => { at.call(null) }, TypeError);
            assert.throws(() => { at.call(undefined) }, TypeError);
        }
    },
    {
        name: "Indexation of array with non-numerical argument",
        body: function () {
            const array = [0, 1];

            assert.areEqual(array.at(false), 0, 'array.at(false) should be 0');
            assert.areEqual(array.at(null), 0, 'array.at(null) should be 0');
            assert.areEqual(array.at(undefined), 0, 'array.at(undefined) should be 0');
            assert.areEqual(array.at(""), 0, 'array.at("") should be 0');
            assert.areEqual(array.at(function () { }), 0, 'array.at(function() {}) should be 0');
            assert.areEqual(array.at([]), 0, 'array.at([]) should be 0');
            assert.areEqual(array.at(true), 1, 'array.at(true) should be 1');
            assert.areEqual(array.at("1"), 1, 'array.at("1") should be 1');
        }
    },
    {
        name: "Indexation of array with negative numerical argument",
        body: function () {
            const array = [7, 1, 4, , 2, 12];

            assert.areEqual(array.at(0), 7, 'array.at(0) should be 7');
            assert.areEqual(array.at(-1), 12, 'array.at(-1) should be 12');
            assert.areEqual(array.at(-2), 2, 'array.at(-2) should be 2');
            assert.areEqual(array.at(-3), undefined, 'array.at(-3) should be undefined');
            assert.areEqual(array.at(-4), 4, 'array.at(-4) should be 4');
        }
    },
    {
        name: "Indexation of array with positive numerical argument",
        body: function () {
            const array = [7, 1, 4, , 2, 12];

            assert.areEqual(array.at(0), 7, 'array.at(0) should be 7');
            assert.areEqual(array.at(1), 1, 'array.at(-1) should be 1');
            assert.areEqual(array.at(2), 4, 'array.at(-2) should be 4');
            assert.areEqual(array.at(3), undefined, 'array.at(-3) should be undefined');
            assert.areEqual(array.at(4), 2, 'array.at(-4) should be 2');
        }
    },
    {
        name: "Out of bounds",
        body: function () {
            const array = [];

            assert.areEqual(array.at(0), undefined, `array.at(0) should be undefined`);
            assert.areEqual(array.at(-1), undefined, `array.at(-1) should be undefined`);
            assert.areEqual(array.at(2), undefined, `array.at(2) should be undefined`);
        }
    },
    {
        name: "Argument ToInteger()",
        body: function () {
            let count = 0;
            const index = {
                valueOf() {
                    count++;
                    return 1;
                }
            };

            const array = [0, 1, 2, 3];

            assert.areEqual(array.at(index), 1, 'result of array.at(index) should be 1');
            assert.areEqual(count, 1, 'The value of count should be 1');
        }
    },
    {
        name: "Array.prototype.at called on Array-Like object",
        body: function () {
            const arraylike = { 0: 1, 1: 2, 2: 3, length: 2 };

            assert.areEqual(call(at, arraylike, 0), 1, `call(at, arraylike, 0) should be 1`);
            assert.areEqual(call(at, arraylike, -1), 2, `call(at, arraylike, -1) should be 2`); // -1 + arraylike.length === 1
            assert.areEqual(call(at, arraylike, 2), undefined, `call(at, arraylike, 2) should be undefined`);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
