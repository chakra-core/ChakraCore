//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests optional catch binding syntax

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Try-catch with no catch binding",
    body: function() {
      try {} catch {}
    },
  },
  {
    name: "Try-catch-finally with no catch binding",
    body: function() {
      try {} catch {} finally {}
    },
  },
  {
    name: "Try-catch with no catching binding lexical scope",
    body: function() {
      let x = 1;
      let ranCatch = false;

      try {
        x = 2;
        throw new Error();
      } catch {
        let x = 3;
        let y = true;
        ranCatch = true;
      }

      assert.isTrue(ranCatch, 'executed `catch` block');
      assert.areEqual(x, 2);

      assert.throws(function() { y; }, ReferenceError);
    },
  },
  {
    name: "Optional catch must not have empty parens",
    body: function() {
      assert.throws(function() { eval("try {} catch () {}"); }, SyntaxError);
    },
  },
  {
    name: "Errors are correctly thrown from catch",
    body: function() {
      class Err {}
      assert.throws(function() {
        try {
          throw new Error();
        } catch {
          throw new Err();
        }
      }, Err);
    },
  },
  {
    name: "Variables in catch block are properly scoped",
    body: function() {
      let x = 1;
      try {
        throw 1;
      } catch {
        let x = 2;
        var f1 = function () { return 'f1'; }
        function f2() { return 'f2'; }
      }
      assert.areEqual(x, 1);
      assert.areEqual(f1(), 'f1');
      assert.areEqual(f2(), 'f2');
    },
  },
  {
    name: "With scope in catch block",
    body: function() {
      function f() {
        try {
          throw 1;
        } catch {
          with ({ x: 1 }) {
            return function() { return x };
          }
        }
      }
      assert.areEqual(f()(), 1);
    },
  },
  {
    name: "Eval in catch block",
    body: function() {
      function f() {
        let x = 1;
        try {
          throw 1;
        } catch {
          let x = 2;
          return eval('function g() { return x }; g');
        }
      }
      assert.areEqual(f()(), 2);
    },
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
