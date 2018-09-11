//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("../UnitTestFramework/UnitTestFramework.js");

const tests = [
    {
        name: "trimLeft is same function as trimStart",
        body: function () {
            // NOTE: See comments in test/UnitTestFramework/UnitTestFramework.js for more info about what assertions you can use
            assert.areEqual(String.prototype.trimLeft, String.prototype.trimStart, "Both trimStart and trimLeft should point to the same function");
        }
    },
    {
        name: "trimRight is same function as trimEnd",
        body: function () {
            assert.areEqual(String.prototype.trimRight, String.prototype.trimEnd,  "Both trimRight and trimEnd should point to the same function");
        }
    }
];

// The test runner will pass "-args summary -endargs" to ch, so that "summary" appears as argument [0[] to the script,
// and the following line will then ensure that the test will only display summary output (i.e. "PASS").
testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });