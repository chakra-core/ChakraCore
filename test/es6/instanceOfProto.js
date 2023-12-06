//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check
/// <reference path="..\UnitTestFramework\UnitTestFramework.js" />
WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const tests = [
    {
        name: "Issue #6507 : Use @@hasInstance of built-in prototype",
        body() {
            const obj = { __proto__: String };
            const testFn = () => "hello" instanceof obj;
            assert.doesNotThrow(testFn, "instanceof should not throw");
            assert.areEqual(false, testFn(), "instanceof should return false");
        }
    }
]

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });