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
            assert.isTrue(1n*2n == 2n);
            assert.isTrue(2n*1n == 2n);
        }
    },
    {
        name: "change length",
        body: function () {
            assert.isTrue(4294967295n * 2n == 8589934590n);
            assert.isTrue(123n * 18446744073709551615n == 2268949521066274848645n);
        }
    },
    {
        name: "Out of 64 bit range",
        body: function () {
            var x = 1234567890123456789012345678901234567890n;
            var y = BigInt(6172839450617283945061728394506172839450n);
            assert.isTrue(x * 5n == y);
        }
    },
    {
        name: "Very big",
        body: function () {
            var x = eval('1234567890'.repeat(20)+'0n');
            var y = BigInt(eval('1234567890'.repeat(20)+'7n'));
            assert.isTrue(x*y == 15241578753238836750495351562566681945008382873376009755225118122311263526910001524158887669562677518670946627038562550221003043773814983252552966212772443410028959019878067369875323883776284103056504141139485896967159837982037108626735823345526793582838000483112332160794086427327693963857597928498242646061072549927232083524835691205694817405890606569121173139765328562261853981054717510588324962300n);
        }
    },
    {
        name: "With signed number",
        body: function () {
            assert.isTrue(-3n * 4n == -12n);
            assert.isTrue(3n * -4n == -12n);
            assert.isTrue(-3n * -4n == 12n);
            assert.isTrue(-1n * 1n == -1n);
        }
    },
    {
        name: "With zero",
        body: function () {
            assert.isTrue(-4n * 0n == 0n);
            assert.isTrue(4n * 0n == 0n);
            assert.isTrue(0n * 4n == 0n);
            assert.isTrue(0n * -4n == 0n);
        }
    },
    {
        name: "With assign",
        body: function () {
            var x = 3n;
            var y = 2n;
            y *= x;
            assert.isTrue(x == 3n);
            assert.isTrue(y == 6n);
            y = x * 4n;
            assert.isTrue(y == 12n);
            y = 8n * x;
            assert.isTrue(y == 24n);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
