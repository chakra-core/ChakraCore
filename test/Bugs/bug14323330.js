//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var r = typeof this;

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "typeof global this is 'object'",
    body: function () {
        assert.areEqual('object', r, "'typeof this' is 'object' for the global this");
    }
  },
  {
    name: "typeof nested function this",
    body: function () {
        function foo() {
            return typeof this;
        }
        assert.areEqual('object', foo(), "'typeof this' is 'object' for nested function this called with default 'this' binding");
        assert.areEqual('function', foo.call(foo), "'typeof this' should be 'function' when 'this' binding is overriden");
        
        function bar() {
            "use strict";
            return typeof this;
        }
        assert.areEqual('undefined', bar(), "'typeof this' is 'undefined' for nested strict function this called with default 'this' binding");
        assert.areEqual('function', bar.call(bar), "'typeof this' should be 'function' when 'this' binding is overriden");
    }
  },
  {
    name: "typeof nested function new.target",
    body: function () {
        var out = 'wrong';
        function foo() {
            out = typeof new.target;
        }
        foo();
        assert.areEqual('undefined', out, "'typeof new.target' is 'undefined' for normal function call");
        new foo();
        assert.areEqual('function', out, "'typeof new.target' is 'function' for function called as constructor");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
