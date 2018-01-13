//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Date.parse must be able to parse the strings returned by Date.toString() for negative and zero-padded
// years. See https://github.com/Microsoft/ChakraCore/pull/4318

// This test is disabled on xplat because the time zone for negative years on xplat is different from
// time zone on Windows.

/// <reference path="../UnitTestFramework/UnitTestFramework.js" />
if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

function testDate(isoDateString) {
    let Dateobj = new Date(isoDateString);
    let value = Dateobj.valueOf();
    let str = Dateobj.toString();

    assert.areEqual(value, Date.parse(str), "Date.parse('" + str + "') returns wrong value.");
}

let tests = [{
    name: "test if Date.parse() can correctly parse outputs of Date.toString()",
    body: function () {
        testDate("0001-10-13T05:16:33Z");
        testDate("0011-10-13T05:16:33Z");
        testDate("0111-10-13T05:16:33Z");
        testDate("1111-10-13T05:16:33Z");

        // test BC years
        testDate("-000001-11-13T19:40:33Z");
        testDate("-000011-11-13T19:40:33Z");
        testDate("-000111-11-13T19:40:33Z");
        testDate("-001111-11-13T19:40:33Z");
    }
}];

testRunner.run(tests, { verbose: WScript.Arguments[0] != "summary" });
