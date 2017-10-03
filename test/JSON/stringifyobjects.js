//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

const tests = [
    {
        name: "Stringify error",
        body: function () {
            const err = new Error("message");
            assert.areEqual('{"description":"message","message":"message"}', JSON.stringify(err));
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
