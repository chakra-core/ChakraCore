//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Split parameter scope",
        body: function () {
            function f1(a = 10, b = function () { return a; }) {
                assert.areEqual(a, 10, "Initial value of parameter in the body scope should be the same as the one in param scope");
                var a = 20;
                assert.areEqual(a, 20, "New assignment in the body scope updates the variable's value in body scope");
                return b;
            }
            assert.areEqual(f1()(), 10, "Function defined in the param scope captures the formals from the param scope not body scope");

            function f2(a = 10, b = function () { return a; }, c = b() + a) {
                assert.areEqual(a, 10, "Initial value of parameter in the body scope should be the same as the one in param scope");
                assert.areEqual(c, 20, "Initial value of the third parameter in the body scope should be twice the value of the first parameter");
                var a = 20;
                assert.areEqual(a, 20, "New assignment in the body scope updates the variable's value in body scope");
                return b;
            }
            assert.areEqual(f2()(), 10, "Function defined in the param scope captures the formals from the param scope not body scope");

            var obj = {
                f(a = 10, b = function () { return a; }) {
                    assert.areEqual(a, 10, "Initial value of parameter in the body scope of the method should be the same as the one in param scope");
                    var a = 20;
                    assert.areEqual(a, 20, "New assignment in the body scope of the method updates the variable's value in body scope");
                    return b;
                }
            }
            assert.areEqual(obj.f()(), 10, "Function defined in the param scope of the object method captures the formals from the param scope not body scope");

            function f3(a = 10, b = function c() { return a; }) {
                assert.areEqual(a, 10, "Initial value of parameter in the body scope of the method should be the same as the one in param scope");
                var a = 20;
                assert.areEqual(a, 20, "New assignment in the body scope of the method updates the variable's value in body scope");
                return b;
            }
            assert.areEqual(f3()(), 10, "Function expression defined in the param scope captures the formals from the param scope not body scope");

            function f4(a = 10, b = function () { return a; }) {
            assert.areEqual(a, 1, "Initial value of parameter in the body scope should be the same as the one passed in");
            var a = 20;
            assert.areEqual(a, 20, "Assignment in the body scope updates the variable's value in body scope");
            return b;
            }
            assert.areEqual(f4(1)(), 1, "Function defined in the param scope captures the formals from the param scope even when the default expression is not applied for that param");

            (function (a = 10, b = a += 10, c = function () { return a + b; }) {
            assert.areEqual(a, 20, "Initial value of parameter in the body scope should be same as the corresponding symbol's final value in the param scope");
            var a2 = 40;
            (function () { assert.areEqual(a2, 40, "Symbols defined in the body scope should be unaffected by the duplicate formal symbols"); })();
            assert.areEqual(c(), 40, "Function defined in param scope uses the formals from param scope even when executed inside the body");
            })();

            (function (a = 10, b = function () { assert.areEqual(a, 10, "Function defined in the param scope captures the formals from the param scope when exectued from the param scope"); }, c = b()) {
            })();

            function f5(a = 10, b = function () { return a; }) {
                a = 20;
                return b;
            }
            assert.areEqual(f5()(), 10, "Even if the formals are not redeclared in the function body the symbol in the param scope and body scope are different");

            function f6(a = 10, b = function () { return function () { return a; } }) {
            var a = 20
            return b;
            }
            assert.areEqual(f6()()(), 10, "Parameter scope works fine with nested functions");

            var a1 = 10
            function f7(b = function () { return a1; }) {
            assert.areEqual(a1, undefined, "Inside the function body the assignment hasn't happened yet");
            var a1 = 20;
            assert.areEqual(a1, 20, "Assignment to the symbol inside the function changes the value");
            return b;
            }
            assert.areEqual(f7()(), 10, "Function in the param scope correctly binds to the outer variable");

            var arr = [2, 3, 4];
            function f8(a = 10, b = function () { return a; }, ...c) {
                assert.areEqual(arr.length, c.length, "Rest parameter should contain the same number of elements as the spread arg");
                for (i = 0; i < arr.length; i++) {
                    assert.areEqual(arr[i], c[i], "Elements in the rest and the spread should be in the same order");
                }
                return b;
            }
            assert.areEqual(f8(undefined, undefined, ...arr)(), 10, "Presence of rest parameter shouldn't affect the binding");

            function f9( {a:a1, b:b1}, c = function() { return a1 + b1; } ) {
                assert.areEqual(a1, 10, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
                assert.areEqual(b1, 20, "Initial value of the second destructuring parameter in the body scope should be the same as the one in param scope");
                a1 = 1;
                b1 = 2;
                assert.areEqual(a1, 1, "New assignment in the body scope updates the first formal's value in body scope");
                assert.areEqual(b1, 2, "New assignment in the body scope updates the second formal's value in body scope");
                assert.areEqual(c(), 30, "The param scope method should return the sum of the destructured formals from the param scope");
                return c;
            }
            assert.areEqual(f9({ a : 10, b : 20 })(), 30, "Returned method should return the sum of the destructured formals from the param scope");

            function f10({x:x = 10, y:y = function () { return x; }}) {
                assert.areEqual(x, 10, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
                x = 20;
                assert.areEqual(x, 20, "Assignment in the body updates the formal's value");
                return y;
            }
            assert.areEqual(f10({ })(), 10, "Returned method should return the value of the destructured formal from the param scope");

            function g() {
            return 3 * 3;
            }

            function f(h = () => eval("g()")) { // cannot combine scopes
            function g() {
                return 2 * 3;
                }
            return h(); // 9
            }
        }
        // TODO(tcare): Re-enable when split scope support is implemented
        //assert.areEqual(9, f(), "Parameter scope remains split");
    },
    {
        name: "Basic eval in parameter scope",
        body: function () {
            assert.areEqual(1,
                            function (a = eval("1")) { return a; }(),
                            "Eval with static constant works in parameter scope");

            {
                let b = 2;
                assert.areEqual(2,
                                function (a = eval("b")) { return a; }(),
                                "Eval with parent var reference works in parameter scope");
            }

            assert.areEqual(1,
                            function (a, b = eval("arguments[0]")) { return b; }(1),
                            "Eval with arguments reference works in parameter scope");

            function testSelf(a = eval("testSelf(1)")) {
                return a;
            }
            assert.areEqual(1, testSelf(1), "Eval with reference to the current function works in parameter scope");

            var testSelfExpr = function (a = eval("testSelfExpr(1)")) {
                return a;
            }
            assert.areEqual(1, testSelfExpr(), "Eval with reference to the current function expression works in parameter scope");

            {
                let a = 1, b = 2, c = 3;
                function testEvalRef(a = eval("a"), b = eval("b"), c = eval("c")) {
                return [a, b, c];
                }
                assert.throws(function () { testEvalRef(); },
                            ReferenceError,
                            "Eval with reference to the current formal throws",
                            "Use before declaration");

                function testEvalRef2(x = eval("a"), y = eval("b"), z = eval("c")) {
                return [x, y, z];
                }
                assert.areEqual([1, 2, 3], testEvalRef2(), "Eval with references works in parameter scope");
            }
        }
    },
    {
        name: "Eval declarations in parameter scope",
        body: function() {
            // Redeclarations of formals - var
            assert.throws(function () { return function (a = eval("var a = 2"), b = a) { return [a, b]; }() },
                            ReferenceError,
                            "Redeclaring the current formal using var inside an eval throws",
                            "Let/Const redeclaration");
            assert.doesNotThrow(function () { "use strict"; return function (a = eval("var a = 2"), b = a) { return [a, b]; }() },
                                "Redeclaring the current formal using var inside a strict mode eval does not throw");
            assert.doesNotThrow(function () { "use strict"; return function (a = eval("var a = 2"), b = a) { return [a, b]; }() },
                                "Redeclaring the current formal using var inside a strict mode function eval does not throw");

            assert.throws(function () { function foo(a = eval("var b"), b, c = b) { return [a, b, c]; } foo(); },
                            ReferenceError,
                            "Redeclaring a future formal using var inside an eval throws",
                            "Let/Const redeclaration");

            assert.throws(function () { function foo(a, b = eval("var a"), c = a) { return [a, b, c]; } foo(); },
                            ReferenceError,
                            "Redeclaring a previous formal using var inside an eval throws",
                            "Let/Const redeclaration");

            // Let and const do not leak outside of an eval, so the test cases below should never throw.

            // Redeclarations of formals - let
            assert.doesNotThrow(function (a = eval("let a")) { return a; },
                                "Attempting to redeclare the current formal using let inside an eval does not leak");

            assert.doesNotThrow(function (a = eval("let b"), b) { return [a, b]; },
                                "Attempting to redeclare a future formal using let inside an eval does not leak");

            assert.doesNotThrow(function (a, b = eval("let a")) { return [a, b]; },
                                "Attempting to redeclare a previous formal using let inside an eval does not leak");

            // Redeclarations of formals - const
            assert.doesNotThrow(function (a = eval("const a = 1")) { return a; },
                                "Attempting to redeclare the current formal using const inside an eval does not leak");

            assert.doesNotThrow(function (a = eval("const b = 1"), b) { return [a, b]; },
                                "Attempting to redeclare a future formal using const inside an eval does not leak");

            assert.doesNotThrow(function (a, b = eval("const a = 1")) { return [a, b]; },
                                "Attempting to redeclare a previous formal using const inside an eval does not leak");

            // Conditional declarations
            function test(x = eval("var a = 1; let b = 2; const c = 3;")) {
                if (x === undefined) {
                // only a should be visible
                assert.areEqual(1, a, "Var declarations leak out of eval into parameter scope");
                } else {
                // none should be visible
                assert.throws(function () { a }, ReferenceError, "Ignoring the default value does not result in an eval declaration leaking", "'a' is undefined");
                }

                assert.throws(function () { b }, ReferenceError, "Let declarations do not leak out of eval to parameter scope",   "'b' is undefined");
                assert.throws(function () { c }, ReferenceError, "Const declarations do not leak out of eval to parameter scope", "'c' is undefined");
            }
            test();
            test(123);

            // Redefining locals
            function foo(a = eval("var x = 1;")) {
                assert.areEqual(1, x, "Shadowed parameter scope var declaration retains its original value before the body declaration");
                var x = 10;
                assert.areEqual(10, x, "Shadowed parameter scope var declaration uses its new value in the body declaration");
            }

            assert.doesNotThrow(function() { foo(); }, "Redefining a local var with an eval var does not throw");
            assert.throws(function() { return function(a = eval("var x = 1;")) { let x = 2; }(); },   ReferenceError, "Redefining a local let declaration with a parameter scope eval var declaration leak throws",   "Let/Const redeclaration");
            assert.throws(function() { return function(a = eval("var x = 1;")) { const x = 2; }(); }, ReferenceError, "Redefining a local const declaration with a parameter scope eval var declaration leak throws", "Let/Const redeclaration");

            // Function bodies defined in eval
            function funcArrow(a = eval("() => 1"), b = a()) { return [a(), b]; }
            assert.areEqual([1,1], funcArrow(), "Defining an arrow function body inside an eval works at default parameter scope");

            function funcDecl(a = eval("function foo() { return 1; }"), b = foo()) { return [a, b, foo()]; }
            assert.areEqual([undefined, 1, 1], funcDecl(), "Declaring a function inside an eval works at default parameter scope");

            function genFuncDecl(a = eval("function * foo() { return 1; }"), b = foo().next().value) { return [a, b, foo().next().value]; }
            assert.areEqual([undefined, 1, 1], genFuncDecl(), "Declaring a generator function inside an eval works at default parameter scope");

            function funcExpr(a = eval("var f = function () { return 1; }"), b = f()) { return [a, b, f()]; }
            assert.areEqual([undefined, 1, 1], funcExpr(), "Declaring a function inside an eval works at default parameter scope");
            
            assert.throws(function () { eval("function foo(a = eval('b'), b) {}; foo();"); }, ReferenceError, "Future default references using eval are not allowed", "Use before declaration");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
