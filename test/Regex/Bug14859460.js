//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

let tests = [
    {
        name: "repro",
        body: function() {
            assert.isTrue((/^(c|ctrl|control)$/i).test('Ctrl'));
        }
    },
    {
        name: "no repro",
        body: function() {
            assert.isTrue((/^(c|ctrl|control)/i).test('Ctrl'));
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
