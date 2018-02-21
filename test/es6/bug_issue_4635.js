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
    name: "Unnamed function expression should have an empty name property",
    body: function () {
        var obj = {};
        obj.a = class { };
        assert.areEqual('', obj.a.name, "Since the class expression is unnamed, it should have an empty name property");
    }
  },
  {
    name: "Instance of a named function expression",
    body: function () {
        var obj = {};
        obj.a = class A { 
            n() { return this.constructor.name; }
        };
        var a = new obj.a();
        assert.areEqual('A', a.constructor.name, "Constructor should be class itself which should have a name property");
        assert.areEqual('A', a.n(), "Name property lookup via instance method");
    }
  },
  {
    name: "Instance of an unnamed function expression",
    body: function () {
        var obj = {};
        obj.a = class {
            n() { return this.constructor.name; }
        };
        var a = new obj.a();
        assert.areEqual('', a.constructor.name, "Constructor should be class itself which should have an empty name property");
        assert.areEqual('', a.n(), "Name property lookup via instance method");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
