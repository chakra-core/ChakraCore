//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check
/// <reference path="../UnitTestFramework/UnitTestFramework.js" />

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const simpleObj = { "null": null, "undefined": undefined, something: 42 };
Object.freeze(simpleObj);

const tests = [
    {
        name: "Simple method call on property",
        body() {
            // Verify normal behavior
            assert.throws(() => simpleObj.nothing(), TypeError);
            assert.throws(() => simpleObj.null(), TypeError);
            assert.throws(() => simpleObj.undefined(), TypeError);

            // With optional-chains
            assert.isUndefined(simpleObj.nothing?.(), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.null?.(), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.undefined?.(), "OptChain should evaluated to 'undefined'");
        }
    },
    {
        name: "Simple method call on indexer",
        body() {
            // Verify normal behavior
            assert.throws(() => simpleObj["nothing"](), TypeError);
            assert.throws(() => simpleObj["null"](), TypeError);
            assert.throws(() => simpleObj["undefined"](), TypeError);

            // With optional-chains
            assert.isUndefined(simpleObj["nothing"]?.(), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj["null"]?.(), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj["undefined"]?.(), "OptChain should evaluated to 'undefined'");
        }
    },
    {
        name: "Simple method call on non-callable property",
        body() {
            // Verify normal behavior
            assert.throws(() => simpleObj.something(), TypeError, "Non-callable prop", "Function expected");

            // With optional-chains
            assert.throws(() => simpleObj.something?.(), TypeError, "Non-callable prop", "Function expected");
            assert.throws(() => simpleObj?.something(), TypeError, "Non-callable prop", "Function expected");
            assert.throws(() => simpleObj?.something?.(), TypeError, "Non-callable prop", "Function expected");
        }
    },
    {
        name: "Simple method call on non-callable indexer",
        body() {
            // Verify normal behavior
            assert.throws(() => simpleObj["something"](), TypeError, "Non-callable prop", "Function expected");

            // With optional-chains
            assert.throws(() => simpleObj["something"]?.(), TypeError, "Non-callable prop", "Function expected");
            assert.throws(() => simpleObj?.["something"](), TypeError, "Non-callable prop", "Function expected");
            assert.throws(() => simpleObj?.["something"]?.(), TypeError, "Non-callable prop", "Function expected");
        }
    },
    {
        name: "Optional properties before call",
        body() {
            // Verify normal behavior
            assert.throws(() => simpleObj.nothing.something(), TypeError);
            assert.throws(() => simpleObj.null.something(), TypeError);
            assert.throws(() => simpleObj.undefined.something(), TypeError);

            // With optional-chains
            assert.isUndefined(simpleObj.nothing?.something(), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.null?.something(), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.undefined?.something(), "OptChain should evaluated to 'undefined'");
        }
    },
    {
        name: "Optional indexers before call",
        body() {
            // Verify normal behavior
            assert.throws(() => simpleObj.nothing["something"](), TypeError);
            assert.throws(() => simpleObj.null["something"](), TypeError);
            assert.throws(() => simpleObj.undefined["something"](), TypeError);

            // With optional-chains
            assert.isUndefined(simpleObj.nothing?.["something"](), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.null?.["something"](), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.undefined?.["something"](), "OptChain should evaluated to 'undefined'");
        }
    },
    {
        name: "Propagate 'this' correctly",
        body() {
            const specialObj = {
                b() { return this._b; },
                _b: { c: 42 }
            };

            assert.areEqual(42, specialObj.b().c);
            assert.areEqual(42, specialObj?.b().c);
            assert.areEqual(42, specialObj.b?.().c);
            assert.areEqual(42, specialObj?.b?.().c);
            assert.areEqual(42, (specialObj?.b)().c);
            assert.areEqual(42, (specialObj.b)?.().c);
            assert.areEqual(42, (specialObj?.b)?.().c);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
