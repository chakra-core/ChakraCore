//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Lambda with a string constant on the following line shouldn't AV",
    body: function () {
jtmchw => z
'123'
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
