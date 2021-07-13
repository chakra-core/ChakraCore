//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Global scope test on Arrays",
        body: function ()
        {
            var globalScope     = -1;
            var at              = globalScope;
            var find            = globalScope;
            var findIndex       = globalScope;
            var findLast        = globalScope;
            var findLastIndex   = globalScope
            var fill            = globalScope;
            var copyWithin      = globalScope;
            var entries         = globalScope;
            var includes        = globalScope;
            var keys            = globalScope;
            var values          = globalScope;
            var flat            = globalScope;
            var flatMap         = globalScope;
            with([])
            {
                assert.areEqual(globalScope, at,            "at property is not brought into scope by the with statement");
                assert.areEqual(globalScope, find,          "find property is not brought into scope by the with statement");
                assert.areEqual(globalScope, findIndex,     "findIndex property is not brought into scope by the with statement");
                assert.areEqual(globalScope, findLast,      "findLast property is not brought into scope by the with statement");
                assert.areEqual(globalScope, findLastIndex, "findLastIndex property is not brought into scope by the with statement");
                assert.areEqual(globalScope, fill,          "fill property is not brought into scope by the with statement");
                assert.areEqual(globalScope, copyWithin,    "copyWithin property is not brought into scope by the with statement");
                assert.areEqual(globalScope, entries,       "entries property is not brought into scope by the with statement");
                assert.areEqual(globalScope, includes,      "includes property is not brought into scope by the with statement");
                assert.areEqual(globalScope, keys,          "keys property is not brought into scope by the with statement");
                assert.areEqual(globalScope, values,        "values property is not brought into scope by the with statement");
                assert.areEqual(globalScope, flat,          "flat property is not brought into scope by the with statement");
                assert.areEqual(globalScope, flatMap,       "flatMap property is not brought into scope by the with statement");
            }
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
