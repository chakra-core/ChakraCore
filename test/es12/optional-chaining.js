//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check
/// <reference path="../UnitTestFramework/UnitTestFramework.js" />

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const simpleObj = { "null": null, "undefined": undefined };
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
        name: "Simple method calls",
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
        name: "Simple properties before call",
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
        name: "Simple indexers before call",
        body() {
            // Verify normal behavior
            assert.throws(() => simpleObj.nothing["something"], TypeError);
            assert.throws(() => simpleObj.null["something"], TypeError);
            assert.throws(() => simpleObj.undefined["something"], TypeError);

            // With optional-chains
            assert.isUndefined(simpleObj.nothing?.["something"](), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.null?.["something"](), "OptChain should evaluated to 'undefined'");
            assert.isUndefined(simpleObj.undefined?.["something"](), "OptChain should evaluated to 'undefined'");
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

            // ToDo: How can I run async tests?
            // simpleObj.nothing?.[await Promise.reject()];
            // simpleObj.null?.[await Promise.reject()];
            // simpleObj.undefined?.[await Promise.reject()];
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
    // Null check
    {
        name: "Only check for 'null' and 'undefined'",
        body() {
            assert.areEqual(0, ""?.length, "Expected empty string length");
        }
    },
    // Parsing
    {
        name: "Parse ternary correctly",
        body() {
            assert.areEqual(0.42, eval(`"this is not falsy"?.42 : 0`));
        }
    },
    {
        name: "Tagged Template in OptChain is illegal",
        body() {
            assert.throws(() => eval("simpleObj.undefined?.`template`"), SyntaxError, "No TaggedTemplate here", "Invalid tagged template in optional chain.");
            assert.throws(() => eval(`simpleObj.undefined?.
                \`template\``), SyntaxError, "No TaggedTemplate here", "Invalid tagged template in optional chain.");
        }
    },
    {
        name: "No new in OptChain",
        body() {
            assert.throws(() => eval(`
            class Test { }
            new Test?.();
            `), SyntaxError, "'new' in OptChain is illegal", "Invalid optional chain in new expression.");
        }
    },
    {
        name: "No super in OptChain",
        body() {
            assert.throws(() => eval(`
            class Base { }
            class Test extends Base {
                constructor(){
                    super?.();
                }
            }
            `), SyntaxError, "Super in OptChain is illegal", "Invalid use of the 'super' keyword");

            assert.throws(() => eval(`
            class Base { }
            class Test extends Base {
                constructor(){
                    super();

                    super?.abc;
                }
            }
            `), SyntaxError, "Super in OptChain is illegal", "Invalid use of the 'super' keyword");
        }
    },
    {
        name: "No assignment",
        body() {
            const a = {};
            assert.throws(() => eval(`a?.b++`), SyntaxError, "Assignment is illegal", "Invalid left-hand side in assignment.");
            assert.throws(() => eval(`a?.b += 1`), SyntaxError, "Assignment is illegal", "Invalid left-hand side in assignment.");
            assert.throws(() => eval(`a?.b = 5`), SyntaxError, "Assignment is illegal", "Invalid left-hand side in assignment.");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
