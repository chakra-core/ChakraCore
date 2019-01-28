//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Add and sub",
        body: function () {
            assert.isTrue(1n+2n+3n+4n == 10n);
            assert.isTrue(1n-2n+3n+4n == 6n);
            assert.isTrue(1n-2n-3n+4n == 0n);        
            assert.isTrue(1n-2n-3n-4n == -8n);
        }
    },
    {
        name: "valueOf",
        body: function() {
            assert.isTrue(1n+{valueOf:() => 2n} == 3n);
            assert.isTrue(1n-{valueOf:() => 2n} == -1n);
            assert.isTrue(1n+{valueOf:() => -2n} == -1n);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
