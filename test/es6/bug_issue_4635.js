//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Named function expression should have a name property even if it's being assigned to an object property",
    body: function () {
        var obj = {};
        obj.a = class A { };
        assert.areEqual('A', obj.a.name, "Since the class expression is named, it should have a name property");
    }
  },
  {
    name: "Unnamed function expression should not have a name property",
    body: function () {
        var obj = {};
        obj.a = class { };
        assert.areEqual('', obj.a.name, "Since the class expression is unnamed, it should not have a name property");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
