//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Subtract BigInt literal",
        body: function () {
            var x = 5n;
            var y = 2n;
            var z = x - y;
            assert.isTrue(z == 3n);
        }
    },
    {
        name: "Subtract to change length",
        body: function () {
            var x = 4294967297n;
            assert.isTrue(x == 4294967297n);
            assert.isTrue(x - 2n == 4294967295n);
        }
    },
    {
        name: "Out of 64 bit range",
        body: function () {
            var x = 1234567890123456789012345678901234567892n;
            var y = BigInt(1234567890123456789012345678901234567890n);
            assert.isTrue(x - 2n == y);
        }
    },
    {
        name: "Very big",
        body: function () {
            var x = eval('1234567890'.repeat(20)+'0n');
            var y = BigInt(eval('1234567890'.repeat(20)+'7n'));
            assert.isTrue(x == y - 7n);
        }
    },
    {
        name: "Subtract with signed number",
        body: function () {
            assert.isTrue(-1n - 2n == -3n);
            assert.isTrue(2n - -1n == 3n);
            assert.isTrue(-2n - 1n == -3n);
            assert.isTrue(1n - -2n == 3n);
            assert.isTrue(-1n - -2n == 1n);
            assert.isTrue(-2n - -1n == -1n);
        }
    },
    {
        name: "With zero",
        body: function () {
            assert.isTrue(-4n - 0n == -4n);
            assert.isTrue(4n - 0n == 4n);
            assert.isTrue(0n - 4n == -4n);
            assert.isTrue(0n - -4n == 4n);
            assert.isTrue(4n - 4n == 0n);
            assert.isTrue(-4n - -4n == 0n);
        }
    },
    {
        name: "With assign",
        body: function () {
            var x = 3n;
            var y = 20n;
            y -= x;
            assert.isTrue(x == 3n);
            assert.isTrue(y == 17n);
            y = x - 4n;
            assert.isTrue(y == -1n);
            y = 11n - x;
            assert.isTrue(y == 8n);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
