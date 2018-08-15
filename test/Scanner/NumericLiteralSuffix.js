//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

// https://tc39.github.io/ecma262/#sec-reserved-words
let keywords = ['await', 'break', 'case', 'catch', 'class', 'const', 'continue', 'debugger', 'default', 'delete', 'do', 'else', 'export', 'extends', 'finally', 'for', 'function', 'if', 'import', 'in', 'instanceof', 'new', 'return', 'super', 'switch', 'this', 'throw', 'try', 'typeof', 'var', 'void', 'while', 'with', 'yield'];
let futureReservedWords = ['enum', 'implements', 'package', 'protected ', 'interface', 'private', 'public'];

// https://tc39.github.io/ecma262/#sec-names-and-keywords
let idStarts = ["\u{50}", '$', '_', "\\u{50}"];

// https://tc39.github.io/ecma262/#sec-literals-numeric-literals
let literalClasses = {
    'Decimal Integer Literal': [
        '0', '1', '123',
        '0.1', '1.1', '123.1', '123.123',
        '0e1', '1e1', '1e+1', '1e-1',
        '0E1', '1E1', '1E+1', '1E-1',
        '123e123', '123e+123', '123e-123',
        '123E123', '123E+123', '123E-123'
     ],
     'Binary Integer Literal': [
        '0b0', '0b1', '0b010101',
        '0B0', '0B1', '0B010101',
     ],
     'Octal Integer Literal': [
        '0o0', '0o1', '0o123',
        '0O0', '0O1', '0O123'
     ],
     'Hex Integer Literal': [
        '0x0', '0x1', '0x123', '0xabc', '0xABC', '0x123abc', '0x123ABC',
        '0X0', '0X1', '0X123', '0Xabc', '0XABC', '0X123abc', '0X123ABC'
     ]
};

var tests = [
    {
        name: "Numeric literal followed by an identifier start throws",
        body: function () {
            for (let literalClass in literalClasses) {
                for (let literal of literalClasses[literalClass]) {
                    for (let idStart of idStarts) {
                        for (let keyword of keywords) {
                          assert.throws(function () { eval(`${literal}${keyword}`); },            SyntaxError, `Keyword '${keyword}' directly after ${literalClass} '${literal}' throws`, "Unexpected identifier after numeric literal");
                        }
                        for (let futureReservedWord of futureReservedWords) {
                          assert.throws(function () { eval(`${literal}${futureReservedWord}`); }, SyntaxError, `Future reserved word '${futureReservedWord}' directly after ${literalClass} '${literal}' throws`, "Unexpected identifier after numeric literal");
                        }
                        for (let idStart of idStarts) {
                          assert.throws(function () { eval(`${literal}${idStart}`); },            SyntaxError, `Identifier start '${idStart}' directly after ${literalClass} '${literal}' throws`, "Unexpected identifier after numeric literal");
                        }
                    }
                }
            }
        }
    },
    {
        name: "Numeric literal followed by invalid digit throws",
        body: function () {
            let nonOctalDigits = ['8', '9'];
            for (let literal of literalClasses['Octal Integer Literal']) {
                for (let nonOctalDigit of nonOctalDigits) {
                    assert.throws(function () { eval(`${literal}${nonOctalDigit}`); },            SyntaxError, `Non-octal digit '${nonOctalDigit}' directly after Octal Integer Literal '${literal}' throws`, "Invalid number");
                }
            }

            let nonBinaryDigits = ['2', '3', '4', '5', '6', '7', '8', '9'];
            for (let literal of literalClasses['Binary Integer Literal']) {
                for (let nonBinaryDigit of nonBinaryDigits) {
                    assert.throws(function () { eval(`${literal}${nonBinaryDigit}`); },            SyntaxError, `Non-binary digit '${nonBinaryDigit}' directly after Binary Integer Literal '${literal}' throws`, "Invalid number");
                }
            }
        }
    },
    {
        name: "Multi-unit whitespace is ignored after numeric identifier",
        body: function () {
            var result = 0;
            eval("\u2028var\u2028x\u2028=\u20281234\u2028; result = x;");
            assert.areEqual(1234, result, "Mutli-unit whitespace after numeric literal does not affect literal value");
        }
    },
    {
        name: "Multi-unit count updated in the middle of a token",
        body: function () {
            if (WScript.Platform.INTL_LIBRARY === "winglob" || WScript.Platform.INTL_LIBRARY === "icu") {
                assert.throws(() => eval('\u20091a'), SyntaxError, 'Multi-unit whitespace followed by numeric literal followed by identifier', 'Unexpected identifier after numeric literal');
                assert.throws(() => eval('\u20091\\u0065'), SyntaxError, 'Multi-unit whitespace followed by numeric literal followed by unicode escape sequence', 'Unexpected identifier after numeric literal');
                assert.throws(() => eval('\u20090o1239'), SyntaxError, 'Multi-unit whitespace followed by invalid octal numeric literal', 'Invalid number');
            }
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });