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
    },
    {
        name: "Optional call of root function",
        body() {
            assert.areEqual(42, eval?.("42"));

            globalThis.doNotUseThisBadGlobalFunction = () => 42;
            assert.areEqual(42, doNotUseThisBadGlobalFunction?.());
            assert.areEqual(42, eval("doNotUseThisBadGlobalFunction?.()"));
        }
    },
    {
        name: "Optional call in eval (function)",
        body() {
            function fn() {
                return 42;
            }
            assert.areEqual(42, eval("fn?.()"));
        },
    },
    {
        name: "Optional call in eval (lambda)",
        body() {
            const fn = () => 42;
            assert.areEqual(42, eval("fn?.()"));
        },
    },
    {
        name: "Optional call in eval (object)",
        body() {
            const obj = {
                fn: () => 42,
            };
            assert.areEqual(42, eval("obj.fn?.()"));
            assert.areEqual(42, eval("obj?.fn?.()"));
            assert.areEqual(42, eval("obj?.fn()"));
        },
    },
    {
        name: "Optional call in eval (undefined)",
        body() {
            assert.areEqual(undefined, eval("doesNotExist?.()"));
        },
    },
    {
        name: "Optional call in eval respects scope",
        body() {
            function fn() {
                return 42;
            }
            assert.areEqual(24, eval(`
                function fn(){
                    return 24;
                }
                fn?.()
            `));
        },
    },
    {
        name: "Optional call to eval should be indirect eval",
        body() {
            const x = 2;
            const y = 4;
            assert.areEqual(6, eval("x + y"));
            assert.throws(() => eval?.("x + y"), ReferenceError, "Should not have access to local scope", "'x' is not defined");
        }
    },
    {
        name: "Opt-chain arguments should work in JIT",
        body() {
            function passArg(arg1) {
                return arg1;
            }
            assert.areEqual(undefined, passArg(simpleObj.null?.something));
            assert.areEqual(undefined, passArg(simpleObj.undefined?.something));
            assert.areEqual(undefined, passArg(simpleObj.something?.something));

            assert.areEqual(null, passArg(simpleObj?.null));
            assert.areEqual(undefined, passArg(simpleObj?.undefined));
            assert.areEqual(42, passArg(simpleObj?.something));
            assert.areEqual("42", passArg(simpleObj?.something.toString()));
            assert.areEqual("42", passArg(simpleObj?.something?.toString()));
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
