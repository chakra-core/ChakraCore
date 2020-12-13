//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Unescaped <LS> and <PS> characters are allowed in string literals",
        body: function () {
            assert.areEqual(eval("'\u2028'"), "\u2028");
            assert.areEqual(" ", "\u2028");
            assert.areEqual(eval("'\u2029'"), "\u2029");
            assert.areEqual(" ", "\u2029");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
