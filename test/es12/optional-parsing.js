//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check
/// <reference path="../UnitTestFramework/UnitTestFramework.js" />

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const tests = [
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
