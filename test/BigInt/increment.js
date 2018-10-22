//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Increment BigInt literal",
        body: function () {
            var x = 123n;
            assert.isTrue(x == 123n);
            x++;
            assert.isTrue(x == 124n);
            ++x;
            assert.isTrue(x == 125n);
        }
    },
    {
        name: "Increment negative BigInt literal",
        body: function () {
            var x = -123n;
            assert.isTrue(x == -123n);
            x++;
            assert.isTrue(x == -122n);
            ++x;
            assert.isTrue(x == -121n);
        }
    },   
    {
        name: "Increment -1n",
        body: function () {
            var x = -1n;
            assert.isTrue(x == -1n);
            x++;
            assert.isTrue(x == 0n);
            ++x;
            assert.isTrue(x == 1n);
        }
    },
    {
        name: "Increment to change length",
        body: function () {
            var x = 4294967295n;
            assert.isTrue(x == 4294967295n);
            x++;
            assert.isTrue(x == 4294967296n);
            ++x;
            assert.isTrue(x == 4294967297n);
            var y = -4294967297n;
            assert.isTrue(y == -4294967297n);
            y++;
            assert.isTrue(y == -4294967296n);
            ++y;
            assert.isTrue(y == -4294967295n);
        }
    },
    {
        name: "Increment BigInt Object",
        body: function () {
            var x = BigInt(12345678901234567890n);
            var y = BigInt(12345678901234567891n);
            assert.isTrue(x < y);
            ++x;
            assert.isTrue(x == y);
            x++;
            assert.isTrue(x >= y);
        }
    },
    {
        name: "Out of 64 bit range",
        body: function () {
            var x = 1234567890123456789012345678901234567890n;
            var y = BigInt(1234567890123456789012345678901234567891n);
            assert.isFalse(x == y);
            x++;
            ++y;
            assert.isTrue(x < y);
            ++x;
            assert.isTrue(x == y);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
