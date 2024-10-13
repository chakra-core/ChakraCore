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
        name: "Simple properties",
        body() {
            // Verify normal behavior
            assert.throws(() => simpleObj.nothing.something, TypeError);
            assert.throws(() => simpleObj.null.something, TypeError);
            assert.throws(() => simpleObj.undefined.something, TypeError);

            // With optional-chains
            assert.isUndefined(simpleObj.nothing?.something, "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.null?.something, "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.undefined?.something, "OptChain should evaluated to 'undefined'");
        }
    },
    {
        name: "Simple indexers",
        body() {
            // Verify normal behavior
            assert.throws(() => simpleObj.nothing["something"], TypeError);
            assert.throws(() => simpleObj.null["something"], TypeError);
            assert.throws(() => simpleObj.undefined["something"], TypeError);

            // With optional-chains
            assert.isUndefined(simpleObj.nothing?.["something"], "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.null?.["something"], "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.undefined?.["something"], "OptChain should evaluated to 'undefined'");
        }
    },
    {
        name: "Short-circuiting ignores indexer expression and method args",
        body() {
            let i = 0;

            assert.isUndefined(simpleObj.nothing?.[i++]);
            assert.isUndefined(simpleObj.null?.[i++]);
            assert.isUndefined(simpleObj.undefined?.[i++]);

            assert.isUndefined(simpleObj.nothing?.[i++]());
            assert.isUndefined(simpleObj.null?.[i++]());
            assert.isUndefined(simpleObj.undefined?.[i++]());

            assert.isUndefined(simpleObj.nothing?.something(i++));
            assert.isUndefined(simpleObj.null?.something(i++));
            assert.isUndefined(simpleObj.undefined?.something(i++));

            assert.strictEqual(0, i, "Indexer may never be evaluated");
        }
    },
    {
        name: "Short-circuiting ignores nested properties",
        body() {
            assert.isUndefined(simpleObj.nothing?.a.b.c.d.e.f.g.h);
            assert.isUndefined(simpleObj.null?.a.b.c.d.e.f.g.h);
            assert.isUndefined(simpleObj.undefined?.a.b.c.d.e.f.g.h);
        }
    },
    {
        name: "Short-circuiting multiple levels",
        body() {
            let i = 0;
            const specialObj = {
                get null() {
                    i++;
                    return null;
                },
                get undefined() {
                    i++;
                    return undefined;
                }
            };

            assert.isUndefined(specialObj?.null?.a.b.c.d?.e.f.g.h);
            assert.isUndefined(specialObj?.undefined?.a.b.c.d?.e.f.g.h);

            assert.areEqual(2, i, "Properties should be called")
        }
    },
    // Null check
    {
        name: "Only check for 'null' and 'undefined'",
        body() {
            assert.areEqual(0, ""?.length, "Expected empty string length");
        }
    },
    {
        name: "Unused opt-chain result should not crash jit",
        body() {
            assert.areEqual(undefined, eval(`boo?.();`));
            assert.areEqual("result", eval(`boo?.(); "result"`));
        }
    },
    {
        name: "Return register works with opt-chain",
        body() {
            function shouldReturnUndefined() {
                return simpleObj.null?.();
            }
            function shouldReturn2() {
                return "12"?.length;
            }
            assert.areEqual(undefined, shouldReturnUndefined());
            assert.areEqual(2, shouldReturn2());
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
