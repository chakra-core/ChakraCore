//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Reflect.construct consumes new.target",
    body: function () {
        class myBaseClass {
            construct() {
                assert.areEqual(myNewTarget, new.target, "the main point of the test is to make sure myNewTarget is passed into the constructor as new.target");
            }
        }

        class myDerivedClass extends myBaseClass {
        }

        function myNewTarget() {
        }

        Reflect.construct(myDerivedClass, [], myNewTarget);
     }
  },
  {
    name: "Reflect.construct should throw a type error if new.target is not a constructor",
    body: function () {
        class myBaseClass {
            construct() {

            }
        }

        class myDerivedClass extends myBaseClass {
        }

        function myNewTarget() {
        }

        assert.throws(function () { Reflect.construct(myDerivedClass, [], undefined ) }, TypeError, "undefined is not a constructor", "'newTarget' is not a constructor");
     }
  },
  {
    name: "Reflect.construct should work with Object constructor",
    body: function () {
        var p = {};
        var a = { __proto__: p };

        new class extends Object {
          constructor() {
            super(a);
            assert.isTrue(a.__proto__ === p);
          }
        };

        Reflect.construct(Object, [a]);
        assert.isTrue(a.__proto__ === p);

        var q = {};
        function C() {}
        C.prototype = q;
        var obj = Reflect.construct(Object, [a], C);
        assert.isTrue(obj.__proto__ === q);
        assert.isTrue(a.__proto__ === p);
    }
  }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
