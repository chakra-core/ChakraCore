//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

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

let identity = [
    "^",
    "$",
    "\\",
    ".",
    "*",
    "+",
    "?",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
    "|",
    "/",
];

let pseudoIdentity = [
    ["f", "\f"],
    ["n", "\n"],
    ["r", "\r"],
    ["t", "\t"],
    ["v", "\v"],
]

var tests = [
    {
        name: "Unicode-mode RegExp Forbidden Escapes (RegExp constructor)",
        body: function () {
            for (re of forbidden) {
                assert.throws(function () { new RegExp(re, 'u') }, SyntaxError, 'Invalid regular expression: invalid escape in unicode pattern');
                assert.doesNotThrow(function () { new RegExp(re) });
            }
        }
    },
    {
        name: "Unicode-mode RegExp Forbidden Escapes (literal)",
        body: function () {
            for (re of forbidden) {
                assert.throws(function () { eval(`/${re}/u`); }, SyntaxError, 'Invalid regular expression: invalid escape in unicode pattern');
                assert.doesNotThrow(function () { eval(`/${re}/`); }, SyntaxError, 'Invalid regular expression: invalid escape in unicode pattern');
            }
        }
    },
    {
        name: "Allow IdentityEscapes (RegExp constructor)",
        body: function () {
            for (c of identity) {
                assert.doesNotThrow(function () { new RegExp(`\\${c}`, 'u') });
                assert.doesNotThrow(function () { new RegExp(`\\${c}`) });
            }
        }
    },
    {
        name: "Allow IdentityEscapes (literal)",
        body: function () {
            for (c of identity) {
                assert.doesNotThrow(function () { eval(`/\\${c}/u`); });
                assert.doesNotThrow(function () { eval(`/\\${c}/`); });
            }
        }
    },
    {
        name: "Allow Pseudo-Identity Escapes (RegExpconstructor)",
        body: function () {
            for (test of pseudoIdentity) {
                let c = test[0];
                let rendered = test[1];
                let re;
                assert.doesNotThrow(function () { re = new RegExp(`\\${c}`, 'u') });
                assert.isTrue(re.test(rendered));
                assert.doesNotThrow(function () { re = new RegExp(`\\${c}`) });
                assert.isTrue(re.test(rendered));
            }
        }
    },
    {
        name: "Allow Pseudo-Identity Escapes (literal)",
        body: function () {
            for (test of pseudoIdentity) {
                let c = test[0];
                let rendered = test[1];
                let re;
                assert.doesNotThrow(function () { re = eval(`/\\${c}/u`) });
                assert.isTrue(re.test(rendered));
                assert.doesNotThrow(function () { re = eval(`/\\${c}/`) });
                assert.isTrue(re.test(rendered));
            }
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
