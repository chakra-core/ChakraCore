//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Concise-body lambda function containing in expression",
        body: function () {
            var l = a => '0' in [123]
            assert.areEqual("a => '0' in [123]", l.toString(), "consise-body lambda containing in expression");
            assert.isTrue(l(), "in expression can be the concise-body lambda body");
        }
    },
    {
        name: "Concise-body lambda function as var decl initializer in a for..in loop",
        body: function () {
            for (var a = () => 'pass' in []) {
                assert.fail("Should not enter for loop since [] has no properties");
            }
            assert.areEqual('pass', a(), "var decl from for loop should have initialized a");

            for (var a2 = () => 'pass' in [123]) {
                assert.areEqual('0', a2, "Should enter the for loop with property '0'");
            }
            assert.areEqual('0', a2, "var decl from for loop should have been assigned to during iteration");
        }
    },
    {
        name: "Concise-body lambda function as var decl initializer in a for..in..in loop",
        body: function () {
            for (var b = () => 'pass' in [] in []) {
              assert.fail("Should not enter for loop");
            }
            assert.areEqual('pass', b(), "var decl from for loop should still have initial value");

            for (var b2 = () => 'pass' in '0' in [123]) {
              assert.fail("var decl initialization turns into var b2 = () => 'pass' in true which should not enter this loop");
            }
            assert.areEqual('pass', b2(), "var decl was not overriden inside the for loop");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
