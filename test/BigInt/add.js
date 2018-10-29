//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Add BigInt literal",
        body: function () {
            assert.isTrue(1n+2n == 3n);
            assert.isTrue(2n+1n == 3n);
        }
    },
    {
        name: "Add to change length",
        body: function () {
            assert.isTrue(4294967295n + 2n == 4294967297n);
            assert.isTrue(2n + 4294967295n == 4294967297n);
        }
    },
    {
        name: "Out of 64 bit range",
        body: function () {
            var x = 1234567890123456789012345678901234567890n;
            var y = BigInt(1234567890123456789012345678901234567895n);
            assert.isTrue(x + 5n == y);
        }
    },
    {
        name: "Very big",
        body: function () {
            var x = eval('1234567890'.repeat(20)+'0n');
            var y = BigInt(eval('1234567890'.repeat(20)+'7n'));
            assert.isTrue(x + 7n == y);
        }
    },
    {
        name: "Add with signed number",
        body: function () {
            assert.isTrue(-1n + 2n == 1n);
            assert.isTrue(2n + -1n == 1n);
            assert.isTrue(-2n + 1n == -1n);
            assert.isTrue(1n + -2n == -1n);
            assert.isTrue(-1n + -2n == -3n);
            assert.isTrue(-2n + -1n == -3n);
        }
    },
    {
        name: "With zero",
        body: function () {
            assert.isTrue(-4n + 0n == -4n);
            assert.isTrue(4n + 0n == 4n);
            assert.isTrue(0n + 4n == 4n);
            assert.isTrue(0n + -4n == -4n);
            assert.isTrue(4n + -4n == 0n);
            assert.isTrue(-4n + 4n == 0n);
        }
    },
    {
        name: "With assign",
        body: function () {
            var x = 3n;
            var y = 2n;
            y += x;
            assert.isTrue(x == 3n);
            assert.isTrue(y == 5n);
            y = x + 4n;
            assert.isTrue(y == 7n);
            y = 8n + x;
            assert.isTrue(y == 11n);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
