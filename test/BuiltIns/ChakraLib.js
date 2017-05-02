//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Access to __chakralib",
        body: function () {
            assert.areEqual(__chakralib, undefined, "__chakralib must be undefined outside of the BuiltIns methods.");

            var __chakralib = -1;
            var array = [1, 2, 3, 4];
            assert.areEqual(__chakralib, -1, "__chakralib must be -1 outside of the BuiltIns methods.");
            assert.areEqual(array.indexOf(1), 0, "The __chakralib in the indexOf method must give access to built ins methods instead of -1.");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
