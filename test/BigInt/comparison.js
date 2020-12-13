//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Compare BigInt literal",
        body: function () {
            assert.isTrue(123n == 123n);
            assert.isTrue(123n < 1234n);
            assert.isFalse(123n > 1234n);
            assert.isFalse(123n == 1234n);
            assert.isTrue(1234n >= 1233n);
            assert.isTrue(1234n > 123n);
        }
    },
    {
        name: "Compare signed BigInt literal",
        body: function () {
            assert.isTrue(-123n == -123n);
            assert.isFalse(-123n < -1234n);
            assert.isTrue(-123n > -1234n);
            assert.isFalse(-123n == -1234n);
            assert.isFalse(-1234n >= -1233n);
            assert.isFalse(-1234n > -123n);
            assert.isTrue(123n > -1n);
            assert.isTrue(-1n > -123456789012n);
        }
    },
    {
        name: "Compare zero BigInt literal",
        body: function () {
            assert.isTrue(0n == -0n);
            assert.isTrue(-123n < -0n);
            assert.isTrue(-0n > -1234n);
            assert.isTrue(-0n <= 123n);
            assert.isFalse(0n >= 1233n);
            assert.isTrue(0n > -123n);
            assert.isTrue(0n > -1n);
            assert.isTrue(0n > -123456789012n);
        }
    },
    {
        name: "Init BigInt literal and compare",
        body: function () {
            var x = 12345678901234567890n;
            var y = 12345678901234567891n;
            assert.isFalse(x == y);
            assert.isTrue(x < y);
            assert.isTrue(x <= y);
            assert.isTrue(x == x);
            assert.isFalse(x >= y);
            assert.isFalse(x > y);
        }
    },
    {
        name: "Init BigInt Object and compare",
        body: function () {
            var x = BigInt(12345678901234567890n);
            var y = BigInt(12345678901234567891n);
            assert.isFalse(x == y);
            assert.isTrue(x < y);
            assert.isTrue(x <= y);
            assert.isTrue(x == x);
            assert.isFalse(x >= y);
            assert.isFalse(x > y);
        }
    },
    {
        name: "Out of 64 bit range",
        body: function () {
            var x = 1234567890123456789012345678901234567890n;
            var y = BigInt(1234567890123456789012345678901234567891n);
            assert.isFalse(x == y);
            assert.isTrue(x < y);
            assert.isTrue(x <= y);
            assert.isTrue(x == x);
            assert.isFalse(x >= y);
            assert.isFalse(x > y);
        }
    },
    {
        name: "Very big BigInt, test resize",
        body: function () {
            var x = eval('1234567890'.repeat(20) + 'n');
            var y = eval('1234567891'.repeat(20) + 'n');
            assert.isFalse(x == y);
            assert.isTrue(x < y);
            assert.isTrue(x <= y);
            assert.isTrue(x == x);
            assert.isFalse(x >= y);
            assert.isFalse(x > y);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
