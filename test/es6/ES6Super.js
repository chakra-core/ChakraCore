//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "super in direct eval inside an object method",
    body: function () {
        var value;
        var obj = {
            m() { value = eval('super.value;'); }
        };

        assert.areEqual(undefined, value);
        obj.m();
        assert.areEqual(undefined, value);
        Object.setPrototypeOf(obj, { value: "super" });
        obj.m();
        assert.areEqual("super", value);
    }
  },
  {
    name: "home object's prototype is null",
    body: function () {
        var value;
        var obj = {
            m() {
                value = "identifier";
                super.prop;
             },
            n() {
                value = "expression";
                super["prop"];
            }
        };

        Object.setPrototypeOf(obj, null);
        assert.throws(obj.m, TypeError, "identifier", "Unable to get property 'prop' of undefined or null reference");
        assert.throws(obj.n, TypeError, "expression", "Unable to get property 'prop' of undefined or null reference");
    }
  },
  {
    name: "super in strict mode - object",
    body: function () {
        "use strict";
        var obj0 = {
            m() {
                super.prop = "identifier";
                Object.freeze(obj0);
                super.prop = "super";
            }
        };

        var obj1 = {
            m() {
                super['prop'] = "expression";
                Object.freeze(obj1);
                super['prop'] = "super";
            }
        };

        var base = {};
        Object.setPrototypeOf(obj0, base);
        Object.setPrototypeOf(obj1, base);
        assert.throws(()=>obj0.m(), TypeError, "identifier", "Assignment to read-only properties is not allowed in strict mode");
        assert.areEqual("identifier", obj0.prop);
        assert.throws(()=>obj1.m(), TypeError, "identifier", "Assignment to read-only properties is not allowed in strict mode");
        assert.areEqual("expression", obj1.prop);
    }
  },
  {
    name: "super in strict mode - class",
    body: function () {
        class A {
            m() {
                super.prop = "identifier";
                Object.freeze(A.prototype);
                super.prop = "super";
            }

            test() {
                assert.throws(()=>A.prototype.m(), TypeError, "identifier", "Assignment to read-only properties is not allowed in strict mode");
                assert.areEqual("identifier", A.prototype.prop);
            }
        }
        A.prototype.test();

        class B {
            m() {
                super['prop'] = "expression";
                Object.freeze(B.prototype);
                super['prop'] = "super";
            }

            test() {
                assert.throws(()=>B.prototype.m(), TypeError, "identifier", "Assignment to read-only properties is not allowed in strict mode");
                assert.areEqual("expression", B.prototype.prop);
            }
        }
        B.prototype.test();
    }
  },
  {
    name: "super property in eval",
    body: function () {
        var valuex, valuey, valuez, valuet;

        var a = { x: 'a', y: 'a', z: 'a', t: 'a' };
        var b = { y: 'b', t: 'b' };
        Object.setPrototypeOf(b, a);

        var obj = {
            x : 'obj',
            y : 'obj',
            z : 'obj',
            t : 'obj',
            m() {
                valuex = eval('super.x');
                valuey = eval('super.y');
            },
            n() {
                valuez = eval('super["z"]');
                valuet = eval('super["t"]');
            }
        };

        Object.setPrototypeOf(obj, b);
        obj.m();
        assert.areEqual("a", valuex, "value x == 'a'");
        assert.areEqual("b", valuey, "value y == 'b'");
        obj.n();
        assert.areEqual("a", valuez, "value z == 'a'");
        assert.areEqual("b", valuet, "value t == 'b'");
    }
  },
  {
    name: "evaluation of expression before making super property reference",
    body: function () {
        var value = undefined;

        assert.throws(function(){ eval('super[value = 0]'); }, ReferenceError, "super[value = 0]", "Missing or invalid 'super' binding");
        assert.areEqual(0, value);
    }
  },
  {
    name: "evaluation of argument list after getting super constructor",
    body: function () {
        var value = undefined;

        class A extends Object { constructor() { super(value = 1); } }
        Object.setPrototypeOf(A, toString);
        assert.throws(()=>new A(), TypeError, "Function is not a constructor", "Function is not a constructor");
        assert.areEqual(undefined, value);
    }
  },
  {
    name: "super reference may not be deleted",
    body: function () {
        assert.throws(
            function() {
                class A extends Object {
                    constructor() { delete super.prop; }
                }
                new A();
            }, ReferenceError, "attempts to delete super property", "Unable to delete property with a super reference");
    }
  },
  {
    name: "attempts to call a null super constructor throws TypeError",
    body: function () {
        var value = 0;
        assert.throws(
            function() {
                class A extends null {
                    constructor() { value++; super(); value++; }
                }
                new A();
            }, TypeError, "attempts to call a null super constructor", "Function is not a constructor");

        assert.areEqual(1, value);
    }
  },
  {
    name: "direct super calls from a class constructors",
    body: function () {
        var count = 0;
        assert.throws(function(){class A{constructor(){eval("count++; super();");}};new A();}, SyntaxError, "", "Invalid use of the 'super' keyword");
        assert.throws(function(){class A{constructor(){(()=>eval("count++; super();"))();}};new A();}, SyntaxError, "", "Invalid use of the 'super' keyword");
        assert.throws(function(){class A{constructor(){(()=>{(()=>eval("count++; super();"))();})();}};new A();}, SyntaxError, "", "Invalid use of the 'super' keyword");
        assert.throws(function(){class A{constructor(){eval("eval(\"count++; super();\");");}};new A();}, SyntaxError, "", "Invalid use of the 'super' keyword");
        assert.throws(function(){class A{constructor(){eval("(()=>{count++; super();})();");}};new A();}, SyntaxError, "", "Invalid use of the 'super' keyword");
        assert.throws(function(){class A{constructor(){eval("(()=>eval(\"count++; super();\"))();");}};new A();}, SyntaxError, "", "Invalid use of the 'super' keyword");
        assert.areEqual(0, count, "SyntaxError preempts side effects")

        assert.doesNotThrow(function(){class A extends Object{constructor(){eval("count++; super();");}};new A();}, "");
        assert.doesNotThrow(function(){class A extends Object{constructor(){(()=>eval("count++; super();"))();}};new A();}, "");
        assert.doesNotThrow(function(){class A extends Object{constructor(){(()=>{(()=>eval("count++; super();"))();})();}};new A();}, "");
        assert.doesNotThrow(function(){class A extends Object{constructor(){eval("eval(\"count++; super();\");");}};new A();}, "");
        assert.doesNotThrow(function(){class A extends Object{constructor(){eval("(()=>{count++; super();})();");}};new A();}, "");
        assert.doesNotThrow(function(){class A extends Object{constructor(){eval("(()=>eval(\"count++; super();\"))();");}};new A();}, "");
        assert.areEqual(6, count, "Side effects expected without SyntaxError");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
