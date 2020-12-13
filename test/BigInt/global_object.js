//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Test BigInt global object properties",
        body: function () {
            assert.isTrue(BigInt.length == 1);
            assert.isTrue(BigInt.name == "BigInt");
            assert.isTrue(BigInt.prototype == "[object Object]");
            assert.isTrue(BigInt.prototype.constructor === BigInt);
            assert.isTrue(BigInt.__proto__ === Function.prototype);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
