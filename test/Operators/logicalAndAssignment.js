//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

assert.sameValue = function sameValue(expected, actual, message) {
    assert.areEqual(actual, expected, message);
}

const tests = [
    {
        name: "lgcl and assignment operator",
        body: function () {
            var value = undefined;
            assert.sameValue(value &&= 1, undefined, "(value &&= 1) === undefined; where value = undefined");

            value = null;
            assert.sameValue(value &&= 1, null, "(value &&= 1) === null where value = null");

            value = false;
            assert.sameValue(value &&= 1, false, "(value &&= 1) === false; where value = false");

            value = 0;
            assert.sameValue(value &&= 1, 0, "(value &&= 1) === 0; where value = 0");

            value = -0;
            assert.sameValue(value &&= 1, -0, "(value &&= 1) === -0; where value = -0");

            value = NaN;
            assert.sameValue(value &&= 1, NaN, "(value &&= 1) === NaN; where value = NaN");

            value = "";
            assert.sameValue(value &&= 1, "", '(value &&= 1) === "" where value = ""');



            value = true;
            assert.sameValue(value &&= 1, 1, "(value &&= 1) === 1; where value = true");

            value = 2;
            assert.sameValue(value &&= 1, 1, "(value &&= 1) === 1; where value = 2");

            value = "test";
            assert.sameValue(value &&= 1, 1, '(value &&= 1) === 1; where value = "test"');

            var sym = Symbol("");
            value = sym;
            assert.sameValue(value &&= 1, 1, "(value &&= 1) === 1; where value = Symbol()");

            var obj = {};
            value = obj;
            assert.sameValue(value &&= 1, 1, "(value &&= 1) === 1; where value = {}");
        }
    },
    {
        name: "lgcl and eval strict",
        body: function () {
            "use strict",
                eval &&= 20;
        }
    },
    {
        name: "lgcl and arguments strict",
        body: function () {
            "use strict",
                arguments &&= 20;
        }
    },
    {
        name: "lgcl and assignment lhs before rhs",
        body: function () {
            function DummyError() { }

            class TestError extends Error {}

            assert.throws(function () {
                var base = null;
                var prop = function () {
                    throw new DummyError();
                };
                var expr = function () {
                    throw new TestError("right-hand side expression evaluated");
                };

                base[prop()] &&= expr();
            }, DummyError);

            // assert.throws(function () {
            //     var base = null;
            //     var prop = {
            //         toString: function () {
            //             throw new TestError("property key evaluated");
            //         }
            //     };
            //     var expr = function () {
            //         throw new TestError("right-hand side expression evaluated");
            //     };

            //     base[prop] &&= expr();
            // }, TypeError);

            // var count = 0;
            // var obj = {};
            // function incr() {
            //     return ++count;
            // }

            // assert.sameValue(obj[incr()] &&= incr(), undefined, "obj[incr()] &&= incr()");
            // assert.sameValue(obj[1], undefined, "obj[1]");
            // assert.sameValue(count, 1, "count");

            // obj[2] = 1;
            // assert.sameValue(obj[incr()] &&= incr(), 3, "obj[incr()] &&= incr()");
            // assert.sameValue(obj[2], 3, "obj[2]");
            // assert.sameValue(count, 3, "count");
        }
    }
]

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
