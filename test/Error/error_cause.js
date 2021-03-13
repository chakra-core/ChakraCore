//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");


const o = {};
const tests = [
    {
        name: "Error.prototype.cause",
        body: function () {
            assert.isFalse("cause" in Error.prototype, "Cause property must not exist in *Error.prototype");
        }
    },
    {
        name: "Error().cause === undefined",
        body: function () {
            assert.isFalse("cause" in SyntaxError(), "Error().cause should not be defined if specified by cause property of options parameter in *Error constructor");
        }
    },
    {
        name: `Error("", { cause: o })'s descriptor`,
        body: function () {
            const e = Error("", { cause: o });
            const desc = Object.getOwnPropertyDescriptor(e, "cause");
            assert.areEqual(desc.configurable, true, "e.cause should be configurable");
            assert.areEqual(desc.writable, true, "e.cause should be writable");
            assert.areEqual(desc.enumerable, false, "e.cause should not be enumerable");
        }
    },
    {
        name: "o === Error('', { cause: o }).cause",
        body: function () {
            assert.areEqual(RangeError("", { cause: o }).cause, o, `Cause property value should be kept as-is`);
        }
    },
    {
        name: "Proxy options parameter",
        body: function () {
            const options = new Proxy({ cause: o }, {
                has(target, p) {
                    hasCounter++;
                    return p in target;
                },
                get(target, p) {
                    getCounter++;
                    return target[p];
                }
            });
            var hasCounter = 0, getCounter = 0;
            const e = ReferenceError("test", options);
            assert.areEqual(hasCounter, 1, `hasCounter should be 1`);
            assert.areEqual(getCounter, 1, `getCounter should be 1`);
            assert.areEqual(e.cause, o, `Cause property value should be kept as-is`);
            assert.areEqual(hasCounter, 1, `hasCounter should be 1`);
            assert.areEqual(getCounter, 1, `getCounter should be 1`);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
