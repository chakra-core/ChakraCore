//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "calling Symbol.toPrimitive on Date prototype should not AV",
    body: function () {
         Date.prototype[Symbol.toPrimitive].call({},'strin' + 'g');
    }
  }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
