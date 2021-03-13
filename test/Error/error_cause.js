//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");


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
        name: "o === Error('', { cause: o }).cause",
        body: function () {
            const o = {};
            assert.areEqual(RangeError("", { cause: o }).cause, o, `Cause property value should be kept as-is`);
        }
    },
    {
        name: "Proxy options parameter",
        body: function () {
            const o = {};
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
    },
    {
        name: "Proxy *Error.toString()",
        body: function () {
            const o = {};
            const e = TypeError("test", { cause: o });
            const p = new Proxy(e, {
                has(target, p) {
                    hasCounter++;
                    return p in target;
                },
                get(target, p) {
                    p === "cause" && getCounter++;
                    return target[p];
                }
            });
            var hasCounter = 0, getCounter = 0;
            p.toString();
            assert.areEqual(hasCounter, 1, `hasCounter should be 1`);
            assert.areEqual(getCounter, 1, `getCounter should be 1`);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
