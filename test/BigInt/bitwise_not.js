//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "BigInt literal",
        body: function () {
            var x = ~123n;
            assert.isTrue(x == -124n);
        }
    },
    {
        name: "Negative BigInt literal",
        body: function () {
            var x = ~-123n;
            assert.isTrue(x == 122n);
        }
    },   
    {
        name: "0n",
        body: function () {
            var x = ~0n;
            assert.isTrue(x == -1n);
        }
    },
    {
        name: "BigInt Object",
        body: function () {
            var x = ~BigInt(12345n);
            var y = BigInt(-12346n);
            assert.isTrue(x == y);
        }
    },
    {
        name: "Out of 64 bit range",
        body: function () {
            var x = ~1234567890123456789012345678901234567890n;
            var y = -1234567890123456789012345678901234567891n;
            assert.isTrue(x == y);
        }
    },
    {
        name: "Very big",
        body: function () {
            var x = eval('1234567890'.repeat(20)+'0n');
            var y = -x-1n;
            var z = ~x;
            assert.isTrue(z == y);
        }
    },
    {
        name: "With assign",
        body: function () {
            var x = 3n;
            var y = x;
            assert.isTrue(x == 3n);
            assert.isTrue(y == 3n);
            y = ~x;            
            assert.isTrue(x == 3n);
            assert.isTrue(y == -4n);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
