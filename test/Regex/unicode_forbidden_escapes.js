//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Unicode-mode RegExp Forbidden Escapes",
        body: function () {
            let forbidden = [
                '\\p',
                '\\P',
                '\\a',
                '\\A',
                '\\e',
                '\\E',
                '\\y',
                '\\Y',
                '\\z',
                '\\Z',
            ];

            for (re of forbidden) {
                assert.throws(function () { new RegExp(re, 'u') }, SyntaxError, 'Invalid regular expression: invalid escape in unicode pattern');
                assert.doesNotThrow(function () { new RegExp(re) });
            }
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
