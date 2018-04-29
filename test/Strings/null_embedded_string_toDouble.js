//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Number tests for issue https://github.com/Microsoft/ChakraCore/issues/5038

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

let tests =
[
    {
        name: 'Converting "\\x00" to Number should return NaN',
        body: function () {
            let arr_tests = [Number("\x00"), Number(" \x00"), -" \x00", +"\x00", +"  \n\x00"];
            arr_tests.forEach(function (num, index) {
                assert.areEqual("number", typeof (num), "Element at index " + index + " has wrong type.");
                assert.isTrue(Number.isNaN(num), "Element at index " + index + " is " + num + ". It should be NaN.");
            });
        }
    },
    {
        name: "Converting empty strings or whitespace-only strings to Number should return 0",
        body: function () {
            let arr_tests = [Number(""), Number(" "), +"", -"", +"  ",
                +"\t\n\r\v\f\xa0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200a\u2028\u2029\u202f\u205f\u3000\ufeff"];

            arr_tests.forEach(function (num, index) {
                assert.areEqual("number", typeof (num), "Element at index " + index + " has wrong type.");
                assert.areEqual(0, num, "Element at index " + index + " is " + num + ". It should be 0.");
            });
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
