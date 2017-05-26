//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Access to __chakraLibrary must not work",
        body: function () {
            assert.isUndefined(__chakraLibrary, "__chakraLibrary must be undefined outside of the BuiltIns methods.");
        },
        name: "Local __chakraLibrary variable must not have an impact on indexOf",
        body: function () {
            var __chakraLibrary = -1;
            var array = [1, 2, 3, 4];
            assert.areEqual(__chakraLibrary, -1, "__chakraLibrary must be -1 outside of the BuiltIns methods.");
            assert.areEqual(array.indexOf(4), 3, "The __chakraLibrary in the indexOf method must give access to built ins methods instead of -1.");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
