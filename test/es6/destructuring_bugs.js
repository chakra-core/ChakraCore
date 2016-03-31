//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Destructuring bug fixes validation",
    body: function () {
      assert.throws(function () { eval("function f1() { var a = 10; [a+2] = []; }; f1();"); }, SyntaxError, "var empty object declaration pattern without an initializer is not valid syntax", "Unexpected operator in destructuring expression");
      assert.throws(function () { eval("function f2() { var a = 10; ({x:a+2} = {x:2}); }; f2();"); }, SyntaxError, "var empty object declaration pattern without an initializer is not valid syntax", "Unexpected operator in destructuring expression");
      assert.throws(function () { eval("function f3() { var a = 10; for ([a+2] in []) { } }; f3();"); }, SyntaxError, "var empty object declaration pattern without an initializer is not valid syntax", "Unexpected operator in destructuring expression");
      assert.doesNotThrow(function () { eval("(function () { var x; for ({x:x}.x of [1,2]) {}; })();"); }, "for..of initializer start with left curly but not a pattern is valid syntax");
      assert.throws(function () { eval("(function () { 'use strict'; [x] = [1]; let x = 2; })();"); }, ReferenceError, "A variable, defined in pattern, is used before its declaration in not valid syntax", "Use before declaration");
      assert.throws(function () { eval("(function () { 'use strict'; let x1 = 1; ({x:x1, y:{y1:y1}} = {x:11, y:{y1:22}}); let y1 = 2; })();"); }, ReferenceError, "A variable, defined in nested pattern, is used before its declaration is not valid syntax", "Use before declaration");
      assert.throws(function () { eval("(function () { 'use strict'; [eval] = []; })();"); }, SyntaxError, "variable name 'eval' defined in array pattern is not valid in strict mode", "Invalid usage of 'eval' in strict mode");
      assert.throws(function () { eval("let a = [a] = [10]"); }, ReferenceError, "A let variable is used in the array pattern in the same statement where it is declared", "Use before declaration");
      assert.throws(function () { eval("let a = {a:a} = {}"); }, ReferenceError, "A let variable is used in object pattern in the same statement where it is declared", "Use before declaration");
      assert.throws(function () { eval("var a = 1; (delete [a] = [2]);"); }, ReferenceError, "Array literal in unary expression should not be converted to array pattern", "Invalid left-hand side in assignment");
      assert.throws(function () { eval("var x, b; for ([x] = [((b) = 1)] of ' ') { }"); }, ReferenceError, "Initializer in for..in is not valid but no assert should be thrown", "Invalid left-hand side in assignment");
      assert.throws(function () { eval("for (let []; ;) { }"); }, SyntaxError, "Native for loop's head has one destructuring pattern without initializer", "Destructuring declarations must have an initializer");
      assert.throws(function () { eval("for (let a = 1, []; ;) { }"); }, SyntaxError, "Native for loop's head has second param as destructuring pattern without initializer", "Destructuring declarations must have an initializer");
      assert.throws(function () { eval("for (let [] = [], a = 1, {}; ;) { }"); }, SyntaxError, "Native for loop's head has third param as object destructuring pattern without initializer", "Destructuring declarations must have an initializer");
      assert.throws(function () { eval("for (let [[a] = []]; ;) { }"); }, SyntaxError, "Native for loop's head as destructuring pattern without initializer", "Destructuring declarations must have an initializer");
      assert.doesNotThrow(function () { eval("for ([]; ;) { break; }"); }, "Native for loop's head is an expression without initializer is valid syntax");
      assert.doesNotThrow(function () { eval("try { y; } catch({}) { }"); }, "Catching exception to empty pattern should not assert and is a valid syntax");
      assert.doesNotThrow(function () { eval("for({} = function (...a) { } in '' ) { }"); }, "Having a function with rest parameter as initializer should not assert and is a valid syntax");
      assert.doesNotThrow(function () { eval("for([] = ((...a) => {}) in '' ) { }"); }, "Having a lambda function with rest parameter as initializer should not assert and is a valid syntax");
      assert.doesNotThrow(function () { eval("[[[] = [function () { }] ] = []]"); }, "Nested array has array pattern which has function expression is a valid syntax");
    }
  },
  {
    name: "Destructuring bug fix - function expression under destructuring pattern does not crash",
    body: function () {
      var a = [];
      var b = 2;
      function f() {
        [ a [ function () { }, b ] ] = [2] ;
      }
      f();
    }
  },
  {
    name: "Destructuring bug fix - rest operator in for loop",
    body: function () {
      assert.throws(function () { eval("for (var {a: ...a1} = {}; ; ) { } "); }, SyntaxError, "Native for loop - usage of '...' in object destructuring pattern in not valid", "Unexpected ... operator");
      assert.throws(function () { eval("for (var {a: ...[]} = {}; ; ) { } "); }, SyntaxError, "Native for loop - usage of '...' before an array in object destructuring pattern in not valid", "Unexpected ... operator");
      assert.throws(function () { eval("for (var {a: ...[]} of '' ) { } "); }, SyntaxError, "for.of loop - usage of '...' before an array in object destructuring pattern in not valid", "Unexpected ... operator");
    }
  },
  {
    name: "Destructuring bug fix - call expression instead of reference node",
    body: function () {
      assert.throws(function () { eval("for (var a of {b: foo()} = {}) { }"); }, SyntaxError, "for.of loop has destructuring pattern which has call expression instead of a reference node", "Invalid destructuring assignment target");
      assert.throws(function () { eval("for ([{b: foo()} = {}] of {}) { }"); }, SyntaxError, "for.of loop has destructuring pattern on head which has call expression instead of a reference node", "Invalid destructuring assignment target");
      function foo() {
          return { bar : function() {} };
      }

      assert.throws(function () { eval("for (var a in {b: foo().bar()} = {}) { }"); }, SyntaxError, "for.in loop has destructuring pattern which has linked call expression instead of a reference node", "Invalid destructuring assignment target");
      assert.doesNotThrow(function () { eval("for (var a in {b: foo().bar} = {}) { }"); }, "for.in loop has destructuring pattern which has a reference node is valid syntax", );
    }
  },
  {
    name: "Destructuring bug fix - object coercible",
    body: function () {
      assert.throws(function () { eval("var {} = undefined"); }, TypeError, "Object declaration - RHS cannot be be undefined", "Cannot convert null or undefined to object");
      assert.throws(function () { eval("var {} = null"); }, TypeError, "Object declaration - RHS cannot be be null", "Cannot convert null or undefined to object");
      assert.throws(function () { eval("({} = undefined);"); }, TypeError, "Object assignment pattern - RHS cannot be be undefined", "Cannot convert null or undefined to object");
      assert.throws(function () { eval("([{}] = []);"); }, TypeError, "Object assignment pattern nested with array pattern has evaluated to have undefined as RHS", "Cannot convert null or undefined to object");
      assert.throws(function () { eval("function f({}){}; f();"); }, TypeError, "Object pattern on function - evaluated to have undefined from assignment expression", "Cannot convert null or undefined to object");
      assert.throws(function () { eval("function f({}){}; f(null);"); }, TypeError, "Object pattern on function - evaluated to have null from assignment expression", "Cannot convert null or undefined to object");
    }
  },
  {
    name: "Destructuring bug fix - a variable in body has the same name as param should not throw in the defer parse mode",
    body: function () {
      assert.doesNotThrow(function () { eval("function foo() { function bar([a]) { var a = 1; } }"); }, "variable 'a' is not a re-declaration" );
      assert.doesNotThrow(function () { eval("function foo() { function bar([a], {b, b1}, [c]) { var b1 = 1; } }"); }, "variable 'b1' is not a re-declaration" );
      assert.doesNotThrow(function () { eval("function foo() { ({c}) => { var c = 1; } }"); }, "variable 'c' is not a re-declaration" );
    }
  },
  {
    name: "Destructuring bug fix - assign to const",
    body: function () {
      assert.throws(function () { const c = 10; ({c} = {c:11}); }, TypeError, "Cannot assign to const", "Assignment to const");
      assert.throws(function () { eval("const c = 10; ({c} = {c:11});"); }, TypeError, "Cannot assign to const in eval", "Assignment to const");
      assert.throws(function () { const c = 10; eval("({c} = {c:11});"); }, TypeError, "Cannot assign to const in eval, where const is defined outsdie of eval", "Assignment to const");
    }
  },
  {
    name: "Destructuring bug fix - pattern with rest parameter",
    body: function () {
      assert.doesNotThrow(function () { eval("function foo({a}, ...b) { if (b) { } }; foo({});"); } );
      assert.doesNotThrow(function () { eval("function foo([], ...b) { if (b) { } }; foo([]);"); });
    }
  },
  {
    name: "Object Destructuring with empty identifier/reference",
    body: function () {
      assert.throws(function () { eval("var {x :  } = {};"); }, SyntaxError);
      assert.throws(function () { eval("var {x :  , } = {};"); }, SyntaxError);
      assert.throws(function () { eval("var {x :  , y} = {};"); }, SyntaxError);
      assert.throws(function () { eval("({x : , y} = {});"); }, SyntaxError);
    }
  },
  {
    name: "Destructuring pattern at param has arguments as declaration",
    body: function () {
      assert.doesNotThrow(function () { eval("function foo([arguments]) { arguments; }; foo([1]);"); });
      assert.doesNotThrow(function () { eval("function foo({arguments}) { arguments; }; foo({arguments:1});"); });
      assert.doesNotThrow(function () { eval("function foo({x:{arguments}}) { arguments; }; foo({x:{arguments:1}});"); });
    }
  },
  {
    name: "Destructuring pattern at param has function as default value",
    body: function () {
      assert.doesNotThrow(function () { eval("function foo(x = ({y = function (p) {}} = 'bar')) {}; foo();"); });
      assert.doesNotThrow(function () { eval("var foo = (x = ({y = function (p) {}} = 'bar')) => {}; foo();"); });
    }
  }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
