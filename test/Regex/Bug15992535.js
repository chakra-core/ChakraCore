//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Regex parser correctly throws error if too many capturing groups",
        body: function () {
            assert.doesNotThrow(
                // Should succeed.  Use 2^15 - 2 pairs of parens, because
                // the entire regex always counts as a capturing group.
                () => { return new RegExp("(ab)".repeat(0x7ffe)); }
            );
            assert.throws(
                () => { return new RegExp("(ab)".repeat(0x8000)); },
                RangeError,
                "regex parsing throws when the regex has more than 2^15 - 1 capturing groups",
                "Regular expression cannot have more than 32,767 capturing groups"
            );
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
