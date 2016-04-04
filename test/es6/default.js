//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Default argument parsing",
    body: function () {
      // Incomplete expressions
      assert.throws(function () { eval("function foo(a =) { return a; }"); },               SyntaxError, "Incomplete default expression throws in a function",                "Syntax error");
      assert.throws(function () { eval("var x = function(a =) { return a; }"); },           SyntaxError, "Incomplete default expression throws in a function expression",     "Syntax error");
      assert.throws(function () { eval("(a =) => a"); },                                    SyntaxError, "Incomplete default expression throws in a lambda",                  "Syntax error");
      assert.throws(function () { eval("var x = { foo(a =) { return a; } }"); },            SyntaxError, "Incomplete default expression throws in an object method",          "Syntax error");
      assert.throws(function () { eval("var x = class { foo(a =) { return a; } }"); },      SyntaxError, "Incomplete default expression throws in a class method",            "Syntax error");
      assert.throws(function () { eval("var x = { foo: function (a =) { return a; } }"); }, SyntaxError, "Incomplete default expression throws in an object member function", "Syntax error");
      assert.throws(function () { eval("function * foo(a =) { return a; }"); },             SyntaxError, "Incomplete default expression throws in a generator function",      "Syntax error");
      assert.throws(function () { eval("var x = function*(a =) { return a; }"); },          SyntaxError, "Incomplete default expression throws in a generator function",      "Syntax error");
      assert.throws(function () { eval("var x = class { * foo(a =) { return a; } }"); },    SyntaxError, "Incomplete default expression throws in a class generator method",  "Syntax error");

      // Duplicate parameters
      assert.throws(function () { eval("function f(a, b, a, c = 10) { }"); },               SyntaxError, "Duplicate parameters are not allowed before the default argument", "Duplicate formal parameter names not allowed in this context");
      assert.throws(function () { eval("function f(a, b = 10, a) { }"); },                  SyntaxError, "Duplicate parameters are not allolwed after the default argument", "Duplicate formal parameter names not allowed in this context");
      assert.throws(function () { eval("function f(a, b, a, c) { \"use strict\"; }"); },    SyntaxError, "When function is in strict mode duplicate parameters are not allowed for simple parameter list", "Duplicate formal parameter names not allowed in strict mode");
      assert.throws(function () { eval("function f(a, b = 1) { \"use strict\"; }"); },      SyntaxError, "Strict mode cannot be applied to functions with default parameters", "Cannot apply strict mode on functions with non-simple parameter list");
      assert.throws(function () { eval("function f() { \"use strict\"; function g(a, b, a) { } }"); },      SyntaxError, "When a function is already in strict mode duplicate parameters are not allowed for simple parameter list", "Duplicate formal parameter names not allowed in strict mode");
      assert.throws(function () { eval("function f() { \"use strict\"; function g(a, b, a = 10) { } }"); }, SyntaxError, "When a function is already in strict mode duplicate parameters are not allowed for formal parameter list", "Duplicate formal parameter names not allowed in strict mode");

      assert.doesNotThrow(function f() { "use strict"; function g(a, b = 10) { } },           "Default arguments are allowed for functions which are already in strict mode");
      assert.doesNotThrow(function f(a, b, a, c) { return a + b + c; },                       "In non-strict mode duplicate parameters are allowed");

      assert.throws(function () { eval("function foo(a *= 5)"); },                          SyntaxError, "Other assignment operators do not work");

      // Redeclaration errors - non-simple in this case means any parameter list with a default expression
      assert.doesNotThrow(function () { eval("function foo(a = 1) { var a; }"); },            "Var redeclaration with a non-simple parameter list");
      assert.doesNotThrow(function () { eval("function foo(a = 1, b) { var b; }"); },         "Var redeclaration does not throw with a non-simple parameter list on a non-default parameter");
      assert.throws(function () { function foo(a = 1) { eval('var a;'); }; foo() },     ReferenceError, "Var redeclaration throws with a non-simple parameter list inside an eval", "Let/Const redeclaration");
      assert.throws(function () { function foo(a = 1, b) { eval('var b;'); }; foo(); }, ReferenceError, "Var redeclaration throws with a non-simple parameter list on a non-default parameter inside eval", "Let/Const redeclaration");

      assert.doesNotThrow(function () { function foo(a = 1) { eval('let a;'); }; foo() },           "Let redeclaration inside an eval does not throw with a non-simple parameter list");
      assert.doesNotThrow(function () { function foo(a = 1) { eval('const a = "str";'); }; foo() }, "Const redeclaration inside an eval does not throw with a non-simple parameter list");
      assert.doesNotThrow(function () { function foo(a = 1) { eval('let a;'); }; foo() },           "Let redeclaration of a non-default parameter inside an eval does not throw with a non-simple parameter list");
      assert.doesNotThrow(function () { function foo(a = 1) { eval('const a = 0;'); }; foo() },     "Const redeclaration of a non-default parameter inside an eval does not throw with a non-simple parameter list");

      assert.throws(function () { eval("x = 3 => x"); },                                    SyntaxError, "Lambda formals without parentheses cannot have default expressions", "Expected \'(\'");
      assert.throws(function () { eval("var a = 0, b = 0; (x = ++a,++b) => x"); },          SyntaxError, "Default expressions cannot have comma separated expressions",        "Expected identifier");
    }
  },
  {
    name: "Default argument sanity checks and expressions",
    body: function () {
      function foo(a, b = 2, c = 1, d = a + b + c, e) { return d; }
      assert.areEqual(foo(),     foo(undefined, undefined, undefined, undefined, undefined), "Passing undefined has the same behavior as a default expression");
      assert.areEqual(foo(1),    foo(1, 2),    "Passing some arguments leaves default values for the rest");
      assert.areEqual(foo(1, 2), foo(1, 2, 1), "Passing some arguments leaves default values for the rest");
      assert.areEqual(foo(3, 5), foo(3, 5, 1), "Overriding default values leaves other default values in tact");

      function sideEffects(a = 1, b = ++a, c = ++b, d = ++c) { return [a,b,c,d]; }
      assert.areEqual([2,3,4,4], sideEffects(),              "Side effects chain properly");
      assert.areEqual([1,1,1,1], sideEffects(0,undefined,0), "Side effects chain properly with some arguments passed");

      function argsObj1(a = 15, b = arguments[1], c = arguments[0]) { return [a,b,c]; }
      assert.areEqual([15,undefined,undefined], argsObj1(), "Arguments object uses original parameters passed");

      function argsObj2(a, b = 5 * arguments[0]) { return arguments[1]; }
      assert.areEqual(undefined, argsObj2(5), "Arguments object does not return default expression");

      this.val = { test : "test" };
      function thisTest(a = this.val, b = this.val = 1.1, c = this.val, d = this.val2 = 2, e = this.val2) { return [a,b,c,d,e]; }
      assert.areEqual([{test:"test"}, 1.1, 1.1, 2, 2], thisTest(), "'this' is able to be referenced and modified");

      var expAValue, expBValue, expCValue;
      function f(a = 10, b = 20, c) {
          assert.areEqual(a, expAValue, "First argument");
          assert.areEqual(b, expBValue, "Second argument");
          assert.areEqual(c, expCValue, "Third argument");
      }
      expAValue = null, expBValue = 20, expCValue = 1;
      f(null, undefined, 1);
      expAValue = null, expBValue = null, expCValue = null;
      f(null, null, null);
      expAValue = 10, expBValue = null, expCValue = 3;
      f(undefined, null, 3);

      function lambdaCapture() {
        this.propA = 1;
        var lambda = (a = this.propA++) => {
          assert.areEqual(a,          1, "Lambda default parameters use 'this' correctly");
          assert.areEqual(this.propA, 2, "Lambda default parameters using 'this' support side effects");
        };
        lambda();
      }
      lambdaCapture();

      // Function length with and without default
      function length1(a, b, c) {}
      function length2(a, b, c = 1) {}
      function length3(a, b = 1, c = 1) {}
      function length4(a, b = 1, c) {}
      function length5(a = 2, b, c) {}
      function length6(a = 2, b = 5, c = "str") {}

      assert.areEqual(3, length1.length, "No default parameters gives correct length");
      assert.areEqual(2, length2.length, "One trailing default parameter gives correct length");
      assert.areEqual(1, length3.length, "Two trailing default parameters gives correct length");
      assert.areEqual(1, length4.length, "One default parameter with following non-default gives correct length");
      assert.areEqual(0, length5.length, "One default parameter with two following non-defaults gives correct length");
      assert.areEqual(0, length6.length, "All default parameters gives correct length");
    }
  },
  {
    name: "Use before declaration in parameter lists",
    body: function () {
      assert.throws(function () { eval("function foo(a = b, b = a) {}; foo();"); },   ReferenceError, "Unevaluated parameters cannot be referenced in a default initializer", "Use before declaration");
      assert.throws(function () { eval("function foo(a = a, b = b) {}; foo();"); },   ReferenceError, "Unevaluated parameters cannot be referenced in a default initializer", "Use before declaration");
      assert.throws(function () { eval("function foo(a = (b = 5), b) {}; foo();"); }, ReferenceError, "Unevaluated parameters cannot be modified in a default initializer", "Use before declaration");

      function argsFoo(a = (arguments[1] = 5), b) { return b };
      assert.areEqual(undefined, argsFoo(),     "Unevaluated parameters are referenceable using the arguments object");
      assert.areEqual(undefined, argsFoo(1),    "Side effects on the arguments object are allowed but has no effect on default parameter initialization");
      assert.areEqual(2,         argsFoo(1, 2), "Side effects on the arguments object are allowed but has no effect on default parameter initialization");
    }
  },
  {
    name: "Parameter scope does not have visibility of body declarations",
    body: function () {
      function foo1(a = x) { var x = 1; return a; }
      assert.throws(function() { foo1(); },
                    ReferenceError,
                    "Shadowed var in parameter scope is not affected by body initialization when setting the default value",
                    "'x' is undefined");

      function foo2(a = () => x) { var x = 1; return a(); }
      assert.throws(function () { foo2(); },
                    ReferenceError,
                    "Arrow function capturing var at parameter scope is not affected by body declaration",
                    "'x' is undefined");

      function foo3(a = () => x) { var x = 1; return a; } // a() undefined
      assert.throws(function () { foo3()(); },
                    ReferenceError,
                    "Attempted closure capture of body scoped var throws in an arrow function default expression",
                    "'x' is undefined");

      function foo4(a = function() { return x; }) { var x = 1; return a(); }
      assert.throws(function () { foo4(); },
                    ReferenceError,
                    "Attempted closure capture of body scoped var throws in an anonymous function default expression",
                    "'x' is undefined");

      function foo5(a = function bar() { return 1; }, b = bar()) { return [a(), b]; }
      assert.throws(function () { foo5(); },
                    ReferenceError,
                    "Named function expression does not leak name into subsequent default expressions",
                    "'bar' is undefined");
    }
  },
  {
    name: "Parameter scope shadowing tests",
    body: function () {
      // These tests exercise logic in FindOrAddPidRef for when we need to look at parameter scope

      // Original sym in parameter scope
      var test0 = function(arg1 = _strvar0) {
        for (var _strvar0 in f32) {
            for (var _strvar0 in obj1) {
            }
        }
      }

      // False positive PidRef (no decl) at parameter scope
      function test1() {
        for (var _strvar0 in a) {
            var f = function(b = _strvar0) {
                for (var _strvar0 in c) {}
            };
        }
      }

      function test2() {
        let l = (z) => {
            let  w = { z };
            assert.areEqual(10, w.z, "Identifier reference in object literal should get the correct reference from the arguments");
            var z;
        }
        l(10);
      };
      test2();

    }
  },
  {
    name: "Arrow function bodies in parameter scope",
    body: function () {
      // Nested parameter scopes
      function arrow(a = ((x = 1) => x)()) { return a; }
      assert.areEqual(1, arrow(), "Arrow function with default value works at parameter scope");

      function nestedArrow(a = (b = (x = () => 1)) => 1) { return a; }
      assert.areEqual(1, nestedArrow()(), "Nested arrow function with default value works at parameter scope");
    }
  },
  {
    name: "OS 1583694: Arguments sym is not set correctly on undo defer",
    body: function () {
      eval();
      var arguments;
    }
  },
  {
    name: "Split parameter scope",
    body: function () {
        assert.throws(function () { eval('function f(a = 1, b = function () { return a; }) { }') }, SyntaxError, "Functions that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f(a = 1, b = function () { var a1 = 10; return a; }) { var a = 20;}') }, SyntaxError, "Functions that refer to a formal are not allowed in the param scope even if it is redeclared in teh body", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('obj = { f(a = 1, b = function () { return a; }) { } }') }, SyntaxError, "Object methods that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('class c { f(a = 1, b = function () { return a; }) { } }') }, SyntaxError, "Class methods that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f(a = 1, b = function c() { return a; }) { }') }, SyntaxError, "Function expressions with name that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f(a = 1, b = 2, c = function () { return a + b; }) {}') }, SyntaxError, "Functions that refer to formals are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f(a = 1, b = function () { return function () { return a; } }) { }') }, SyntaxError, "Nested functions that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f8(a = 1, b = function () { return a; }, ...c) { }') }, SyntaxError, "Functions that refer to a formal are not allowed in the param scope when rest param is present", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f9( {a:a1, b:b1}, c = function() { return a1 + b1; } ) { }') }, SyntaxError, "Functions that refer to a formal are not allowed in the param scope even in destructured scenario", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f10({x:x = 10, y:y = function () { return x; }}) { }') }, SyntaxError, "Functions that refer to a formal are not allowed in the param scope in destructured scenario", "Formals cannot contain functions definitions that reference them");

        assert.doesNotThrow(function (a = 10, b = a++, c = a + b) {}, "Using formals without closure does not throw a syntax error");
        assert.doesNotThrow(function f1(a = 10, b = function () { return 20; }) {}, "Function definitions without any reference to the formals works");

        var a1 = 10
        function f1(b = function () { return a1; }) {
            assert.areEqual(a1, undefined, "Inside the function body the assignment hasn't happened yet");
            var a1 = 20;
            assert.areEqual(a1, 20, "Assignment to the symbol inside the function changes the value");
            return b;
        }
        assert.areEqual(f1()(), 10, "Function in the param scope correctly binds to the outer variable");

        assert.throws(function () { eval('function f(a = 1, b = () => a) { }') }, SyntaxError, "Arrow functions that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f(a = 1, b = () => { var a1 = 10; return a; }) { var a = 20;}') }, SyntaxError, "Arrow functions that refer to a formal are not allowed in the param scope even if it is redeclared in teh body", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f(a = 1, b = () => { return () => a; }) { }') }, SyntaxError, "Nested Arrow functions that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f(a = 1, b = function () { return () => a; }) { }') }, SyntaxError, "Arrow function nested inside a function  that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('(a = 1, b = () => a) => { }') }, SyntaxError, "Arrow functions that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('(a = 1, b = () => { return () => a; }) => { }') }, SyntaxError, "Nested Arrow functions that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");

        assert.throws(function () { eval('function f(a = 1, b = class c { f() { var a1 = 10; return a; }}) { }') }, SyntaxError, "Class methods that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        assert.throws(function () { eval('function f(a = 1, b = () => { return class c { f() { var a1 = 10; return a; }} }) { }') }, SyntaxError, "Nested class methods that refer to a formal are not allowed in the param scope", "Formals cannot contain functions definitions that reference them");
        
        // TODO(aneeshd): The additional block created for class seems to have the wrong scope associated with it. Will reenable this once the issue in https://github.com/Microsoft/ChakraCore/issues/299 is fixed.
        // assert.doesNotThrow(function f(a = 1, b = class c { f() { return 2; }}) { }, "Class methods that do not refer to a formal are allowed in the param scope");

        assert.throws(function () { eval("function f(a = eval('1')) { }") }, SyntaxError, "Eval is not allowed in the parameter scope", "'eval' is not allowed in the default initializer");
        assert.throws(function () { eval("function f(a, b = function () { eval('1'); }) { }") }, SyntaxError, "Evals in child functions are not allowed in the parameter scope", "'eval' is not allowed in the default initializer");
        assert.throws(function () { eval("function f(a, b = function () { function f() { eval('1'); } }) { }") }, SyntaxError, "Evals in nested child functions are not allowed in the parameter scope", "'eval' is not allowed in the default initializer");
        assert.throws(function () { eval("function f(a, b = eval('a')) { }") }, SyntaxError, "Eval is not allowed in the parameter scope", "'eval' is not allowed in the default initializer");
    }
  },
  {
    name: "Unmapped arguments - Non simple parameter list",
    body: function () {
        function f1 (x = 10, y = 20, z) {
            x += 2;
            y += 2;
            z += 2;

            assert.areEqual(arguments[0], undefined,  "arguments[0] is not mapped with first formal and did not change when the first formal changed");
            assert.areEqual(arguments[1], undefined,  "arguments[1] is not mapped with second formal and did not change when the second formal changed");
            assert.areEqual(arguments[2], 30,  "arguments[2] is not mapped with third formal and did not change when the third formal changed");

            arguments[0] = 1;
            arguments[1] = 2;
            arguments[2] = 3;

            assert.areEqual(x, 12,  "Changing arguments[0], did not change the first formal");
            assert.areEqual(y, 22,  "Changing arguments[1], did not change the second formal");
            assert.areEqual(z, 32,  "Changing arguments[2], did not change the third formal");
        }
        f1(undefined, undefined, 30);

        function f2 (x = 10, y = 20, z) {
            eval('');
            x += 2;
            y += 2;
            z += 2;

            assert.areEqual(arguments[0], undefined,  "Function has eval - arguments[0] is not mapped with first formal and did not change when the first formal changed");
            assert.areEqual(arguments[1], undefined,  "Function has eval - arguments[1] is not mapped with second formal and did not change when the second formal changed");
            assert.areEqual(arguments[2], 30,  "Function has eval - arguments[2] is not mapped with third formal and did not change when the third formal changed");

            arguments[0] = 1;
            arguments[1] = 2;
            arguments[2] = 3;

            assert.areEqual(x, 12,  "Function has eval - Changing arguments[0], did not change the first formal");
            assert.areEqual(y, 22,  "Function has eval - Changing arguments[1], did not change the second formal");
            assert.areEqual(z, 32,  "Function has eval - Changing arguments[2], did not change the third formal");
        }
        f2(undefined, undefined, 30);

        function f3 (x = 10, y = 20, z) {
            (function () {
                eval('');
            })();
            x += 2;
            y += 2;
            z += 2;

            assert.areEqual(arguments[0], undefined,  "Function's inner function has eval - arguments[0] is not mapped with first formal and did not change when the first formal changed");
            assert.areEqual(arguments[1], undefined,  "Function's inner function has eval - arguments[1] is not mapped with second formal and did not change when the second formal changed");
            assert.areEqual(arguments[2], 30,  "Function's inner function has eval - arguments[2] is not mapped with third formal and did not change when the third formal changed");

            arguments[0] = 1;
            arguments[1] = 2;
            arguments[2] = 3;

            assert.areEqual(x, 12,  "Function's inner function has eval - Changing arguments[0], did not change the first formal");
            assert.areEqual(y, 22,  "Function's inner function has eval - Changing arguments[1], did not change the second formal");
            assert.areEqual(z, 32,  "Function's inner function has eval - Changing arguments[2], did not change the third formal");
        }
        f3(undefined, undefined, 30);

    }
  },
  {
    name: "Param of lambda has default as function",
    body: function () {
        assert.doesNotThrow(function () { eval("[ (a = function () { }) => {} ];"); }, "Lambda defined, inside an array literal, has a default as a function should not assert");
    }
  }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
