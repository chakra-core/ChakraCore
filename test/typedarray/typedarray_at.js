//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const TypedArrayPrototype = Object.getPrototypeOf(Int8Array).prototype;
const at = TypedArrayPrototype.at;


const tests = [
    {
        name: "Existence of TypedArray.prototype.at",
        body: function () {
            assert.isTrue(typeof at === "function", "TypedArray.prototype.at should be a function");
        }
    },
    {
        name: "TypedArray.prototype.at's descriptor",
        body: function () {
            const desc = Object.getOwnPropertyDescriptor(TypedArrayPrototype, "at");
            assert.areEqual(desc.configurable, true, "TypedArray.prototype.at should be configurable");
            assert.areEqual(desc.writable, true, "TypedArray.prototype.at should be writable");
            assert.areEqual(desc.enumerable, false, "TypedArray.prototype.at should not be enumerable");
        }
    },
    {
        name: "Properties of TypedArray.prototype.at",
        body: function () {
            const nameDesc = Object.getOwnPropertyDescriptor(at, "name");
            assert.areEqual(at.name, "at", 'TypedArray.prototype.at.name should be "at"');
            assert.areEqual(nameDesc.configurable, true, "TypedArray.prototype.at.name should be configurable");
            assert.areEqual(nameDesc.writable, false, "TypedArray.prototype.at.name should not be writable");
            assert.areEqual(nameDesc.enumerable, false, "TypedArray.prototype.at.name should not be enumerable");
            const lengthDesc = Object.getOwnPropertyDescriptor(at, "length");
            assert.areEqual(at.length, 1, 'TypedArray.prototype.at.length should be 1');
            assert.areEqual(lengthDesc.configurable, true, "TypedArray.prototype.at.length should be configurable");
            assert.areEqual(lengthDesc.writable, false, "TypedArray.prototype.at.length should not be writable");
            assert.areEqual(lengthDesc.enumerable, false, "TypedArray.prototype.at.length should not be enumerable");
        }
    },
    {
        name: "TypedArray.prototype.at called on invalid object",
        body: function () {
            assert.throws(() => { at.call(null) }, TypeError);
            assert.throws(() => { at.call(undefined) }, TypeError);
        }
    },
    {
        name: "Indexation of typed array with non-numerical argument",
        body: function () {
            const typedarray = new Int8Array([0, 1]);

            assert.areEqual(typedarray.at(false), 0, 'typedarray.at(false) should be 0');
            assert.areEqual(typedarray.at(null), 0, 'typedarray.at(null) should be 0');
            assert.areEqual(typedarray.at(undefined), 0, 'typedarray.at(undefined) should be 0');
            assert.areEqual(typedarray.at(""), 0, 'typedarray.at("") should be 0');
            assert.areEqual(typedarray.at(function () { }), 0, 'typedarray.at(function() {}) should be 0');
            assert.areEqual(typedarray.at([]), 0, 'typedarray.at([]) should be 0');
            assert.areEqual(typedarray.at(true), 1, 'typedarray.at(true) should be 1');
            assert.areEqual(typedarray.at("1"), 1, 'typedarray.at("1") should be 1');
        }
    },
    {
        name: "Indexation of typed array with negative numerical argument",
        body: function () {
            const typedarray = new Int16Array([1, 2, 3]);

            assert.areEqual(typedarray.at(0), 1, 'typedarray.at(0) should be 1');
            assert.areEqual(typedarray.at(-1), 3, 'typedarray.at(-1) should be 3');
            assert.areEqual(typedarray.at(-2), 2, 'typedarray.at(-2) should be 2');
        }
    },
    {
        name: "Indexation of typed array with positive numerical argument",
        body: function () {
            const typedarray = new Int32Array([1, 2, 3]);

            assert.areEqual(typedarray.at(0), 1, 'typedarray.at(0) should be 1');
            assert.areEqual(typedarray.at(1), 2, 'typedarray.at(1) should be 2');
            assert.areEqual(typedarray.at(2), 3, 'typedarray.at(2) should be 3');
        }
    },
    {
        name: "Out of bounds",
        body: function () {
            const typedarray = new Uint32Array;

            assert.areEqual(typedarray.at(0), undefined, `typedarray.at(0) should be undefined`);
            assert.areEqual(typedarray.at(-1), undefined, `typedarray.at(-1) should be undefined`);
            assert.areEqual(typedarray.at(2), undefined, `typedarray.at(2) should be undefined`);
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

            const typedarray = new Uint32Array([1, 2, 3]);

            assert.areEqual(typedarray.at(index), 2, 'result of typedarray.at(index) should be 2');
            assert.areEqual(count, 1, 'The value of count should be 1');
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
