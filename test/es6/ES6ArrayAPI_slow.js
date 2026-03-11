//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const uint32Max = 4294967295;
const tests = [
    {
        name: "Issue #6770 (Assertion failure in copyWithin)",
        body() {
            const array = [];
            array.length = uint32Max;
            array.copyWithin();
        }
    }
];
testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
