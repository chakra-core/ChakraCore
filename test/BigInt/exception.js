//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "With add, sub",
        body: function () {
            assert.throws(() => {var x = 2n + 3;}, TypeError);
            assert.throws(() => {var x = 2 + 3n;}, TypeError);
            assert.throws(() => {var x = 2n - 3;}, TypeError);
            assert.throws(() => {var x = 2 - 3n;}, TypeError);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
