//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Assign BigInt literal",
        body: function () {
            var x = 123n;
            var y = x;
            assert.isTrue(x == 123n);
            assert.isTrue(y == 123n);
            x++;
            assert.isTrue(x == 124n);
            assert.isTrue(y == 123n);
            y = x;
            ++x;
            assert.isTrue(x == 125n);
            assert.isTrue(y == 124n);
        }
    },
    {
        name: "Assign BigInt object",
        body: function () {
            var x = BigInt(123n);
            var y = x;
            assert.isTrue(x == 123n);
            assert.isTrue(y == 123n);
            x++;
            assert.isTrue(x == 124n);
            assert.isTrue(y == 123n);
            y = x;
            ++x;
            assert.isTrue(x == 125n);
            assert.isTrue(y == 124n);
        }
    },
    {
        name: "Value change with add and sub",
        body: function () {
            var x = BigInt(123n);
            var y = x;
            assert.isTrue(x == 123n);
            assert.isTrue(y == 123n);
            x = x + 2n;
            assert.isTrue(x == 125n);
            assert.isTrue(y == 123n);
            y = x;
            x = x - 2n;
            assert.isTrue(x == 123n);
            assert.isTrue(y == 125n);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
