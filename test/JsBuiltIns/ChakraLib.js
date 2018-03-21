//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Access to __chakraLibrary work",
        body: function () {
            assert.isNotUndefined(__chakraLibrary, "__chakraLibrary must NOT be undefined as we're using the -LdChakraLib flag.");
        },
    },
    {
        name: "Overriding __chakraLibrary must not work change the behavior.",
        body: function () {
            __chakraLibrary = -1;
            var array = [1, 2, 3, 4];
            assert.areNotEqual(__chakraLibrary, -1, "__chakraLibrary must not be equal to -1 as the -LdChakraLib flag force to use the internal library associated to the __chakraLibrary token.");
            assert.areEqual(array.indexOf(4), 3, "The __chakraLibrary in the indexOf method must give access to built ins methods instead of -1.");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
