//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testASCII(lower, upper, message) {
    const toUppers = [ String.prototype.toUpperCase, String.prototype.toLocaleUpperCase ];
    const toLowers = [ String.prototype.toLowerCase, String.prototype.toLocaleLowerCase ];
    for (const func of toUppers) {
        assert.areEqual(upper, func.call(lower), `lower.${func.name}(): ${message}`);
        assert.areEqual(upper, func.call(upper), `upper.${func.name}(): ${message}`);
    }
    for (const func of toLowers) {
        assert.areEqual(lower, func.call(upper), `upper.${func.name}(): ${message}`);
        assert.areEqual(lower, func.call(lower), `lower.${func.name}(): ${message}`);
    }
}

testRunner.runTests([
    {
        name: "Visible ASCII characters",
        body() {
            testASCII("", "", "Empty string");

            let i = 32;
            let upper = "";
            let lower = "";
            for (; i < 65; i++) {
                upper += String.fromCharCode(i);
                lower += String.fromCharCode(i);
            }
            for (; i < 91; i++) {
                upper += String.fromCharCode(i);
            }
            for (i = 97; i < 123; i++) {
                lower += String.fromCharCode(i);
            }
            for (i = 91; i < 97; i++) {
                upper += String.fromCharCode(i);
                lower += String.fromCharCode(i);
            }
            for (i = 123; i < 127; i++) {
                upper += String.fromCharCode(i);
                lower += String.fromCharCode(i);
            }

            testASCII(lower, upper, "Visible ASCII");
        }
    },
    {
        name: "Special characters",
        body() {
            const specialCharacters = {
                "\n": "newline",
                "\t": "tab",
                "\r": "carriage return",
                "\0": "null",
                "\"": "double quote",
                "\'": "single quote",
                "\b": "backspace",
            };

            for (const c in specialCharacters) {
                testASCII(`${c}microsoft`, `${c}MICROSOFT`, `string with ${specialCharacters[c]} at the beginning`);
                testASCII(`micro${c}soft`, `MICRO${c}SOFT`, `string with ${specialCharacters[c]} in the middle`);
                testASCII(`microsoft${c}`, `MICROSOFT${c}`, `string with ${specialCharacters[c]} at the end`);
            }
        }
    },
    {
        name: "Type conversion",
        body() {
            const convertible = [
                [new Number(123), "123", "123"],
                [new Boolean(true), "true", "TRUE"],
                [new String("aBc"), "abc", "ABC"],
                [new Object(), "[object object]", "[OBJECT OBJECT]"],
                [["Chakra", 2018, true], "chakra,2018,true", "CHAKRA,2018,TRUE"],
                [{ toString: () => "Hello" }, "hello", "HELLO"]
            ];

            for (const test of convertible) {
                for (const func of [String.prototype.toLowerCase, String.prototype.toLocaleLowerCase]) {
                    assert.areEqual(test[1], func.call(test[0]), `${func.name}: type conversion of ${test[0]} to ${test[1]}`);
                }
                for (const func of [String.prototype.toUpperCase, String.prototype.toLocaleUpperCase]) {
                    assert.areEqual(test[2], func.call(test[0]), `${func.name}: type conversion of ${test[0]} to ${test[2]}`);
                }
            }
        }
    },
    {
        name: "Correct errors are thrown",
        body() {
            for (const badThis of [null, undefined]) {
                for (const func of [String.prototype.toUpperCase, String.prototype.toLocaleUpperCase, String.prototype.toLowerCase, String.prototype.toLocaleLowerCase]) {
                    assert.throws(() => func.call(badThis), TypeError, `${func.name}.call(${Object.prototype.toString.call(badThis)})`);
                }
            }
        }
    },
], { verbose: false });
