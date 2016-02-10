WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Split parameter scope in function definition",
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

            function f3(a = 10, b = function () { return a; }) {
                assert.areEqual(a, 1, "Initial value of parameter in the body scope should be the same as the one passed in");
                var a = 20;
                assert.areEqual(a, 20, "Assignment in the body scope updates the variable's value in body scope");
                return b;
            }
            assert.areEqual(f3(1)(), 1, "Function defined in the param scope captures the formals from the param scope even when the default expression is not applied for that param");

            (function (a = 10, b = a += 10, c = function () { return a + b; }) {
                assert.areEqual(a, 20, "Initial value of parameter in the body scope should be same as the corresponding symbol's final value in the param scope");
                var a2 = 40;
                (function () { assert.areEqual(a2, 40, "Symbols defined in the body scope should be unaffected by the duplicate formal symbols"); })();
                assert.areEqual(c(), 40, "Function defined in param scope uses the formals from param scope even when executed inside the body");
            })();

            (function (a = 10, b = function () { assert.areEqual(a, 10, "Function defined in the param scope captures the formals from the param scope when executed from the param scope"); }, c = b()) {
            })();

            function f4(a = 10, b = function () { return a; }) {
                a = 20;
                return b;
            }
            assert.areEqual(f4()(), 10, "Even if the formals are not redeclared in the function body the symbol in the param scope and body scope are different");

            function f5(a = 10, b = function () { return function () { return a; }; }) {
                var a = 20;
                return b;
            }
            assert.areEqual(f5()()(), 10, "Parameter scope works fine with nested functions");

            var a1 = 10;
            function f6(b = function () { return a1; }) {
                assert.areEqual(a1, undefined, "Inside the function body the assignment hasn't happened yet");
                var a1 = 20;
                assert.areEqual(a1, 20, "Assignment to the symbol inside the function changes the value");
                return b;
            }
            assert.areEqual(f6()(), 10, "Function in the param scope correctly binds to the outer variable");
        }
    },
    {
        name: "Split parameter scope in function expressions with name",
        body: function () {
            function f1(a = 10, b = function c() { return a; }) {
                assert.areEqual(a, 10, "Initial value of parameter in the body scope of the method should be the same as the one in param scope");
                var a = 20;
                assert.areEqual(a, 20, "New assignment in the body scope of the method updates the variable's value in body scope");
                return b;
            }
            assert.areEqual(f1()(), 10, "Function expression defined in the param scope captures the formals from the param scope not body scope");
            
            function f1(a = 10, b = function c(recurse = true) { return recurse ? c(false) : a; }) {
                return b;
            }
            assert.areEqual(f1()(), 10, "Function expression defined in the param scope captures the formals from the param scope not body scope");
        }
    },
    {
        name: "Split parameter scope in member functions",
        body: function () {
            var o1 = {
                f(a = 10, b = function () { return a; }) {
                    assert.areEqual(a, 10, "Initial value of parameter in the body scope of the method should be the same as the one in param scope");
                    var a = 20;
                    assert.areEqual(a, 20, "New assignment in the body scope of the method updates the variable's value in body scope");
                    return b;
                }
            }
            assert.areEqual(o1.f()(), 10, "Function defined in the param scope of the object method captures the formals from the param scope not body scope");
            
            var o2 = {
                f1(a = 10, b = function () { return { f2 () { return a; } } }) {
                    var a = 20;
                    c = function () { return { f2 () { return a; } } };
                    return [b, c];
                }
            }
            var result = o2.f1();
            assert.areEqual(result[0]().f2(), 10, "Short hand method defined in the param scope of the object method captures the formals from the param scope not body scope");
            assert.areEqual(result[1]().f2(), 20, "Short hand method defined in the param scope of the object method captures the formals from the param scope not body scope");
        }
    },
    {
        name: "Arrow functions in split param scope",
        body: function () {
            function f1(a = 10, b = () => { return a; }) {
                assert.areEqual(a, 10, "Initial value of parameter in the body scope should be the same as the one in param scope");
                var a = 20;
                assert.areEqual(a, 20, "New assignment in the body scope updates the variable's value in body scope");
                return b;
            }
            assert.areEqual(f1()(), 10, "Arrow functions defined in the param scope captures the formals from the param scope not body scope");

            function f2(a = 10, b = () => { return a; }) {
                assert.areEqual(a, 1, "Initial value of parameter in the body scope should be the same as the one in param scope");
                var a = 20;
                assert.areEqual(a, 20, "New assignment in the body scope updates the variable's value in body scope");
                return b;
            }
            assert.areEqual(f2(1)(), 1, "Arrow functions defined in the param scope captures the formals from the param scope not body scope even when value is passed");

            function f3(a = 10, b = () => a) {
                assert.areEqual(a, 10, "Initial value of parameter in the body scope should be the same as the one in param scope");
                var a = 20;
                assert.areEqual(a, 20, "New assignment in the body scope updates the variable's value in body scope");
                return b;
            }
            assert.areEqual(f3()(), 10, "Arrow functions with concise body defined in the param scope captures the formals from the param scope not body scope");

            ((a = 10, b = a += 10, c = () => { assert.areEqual(a, 20, "Value of the first formal inside the lambda should be same as the default value"); return a + b; }, d = c() * 10) => {
                assert.areEqual(d, 400, "Initial value of the formal parameter inside the body should be the same as final value from the param scope");
            })();

            function f4(a = 10, b = () => { return () => a; }) {
                a = 20;
                return b;
            }
            assert.areEqual(f4()()(), 10, "Nested lambda should capture the formal param value from the param scope");

            assert.throws(function f4(a = () => x) { var x = 1; a(); }, ReferenceError, "Lambdas in param scope shouldn't be able to access the variables from body", "'x' is undefined");
            assert.throws(function f5() { (function (a = () => x) { var x = 1; return a; })()(); }, ReferenceError, "Lambdas in param scope shouldn't be able to access the variables from body", "'x' is undefined");
            assert.throws((a = () => 10, b = a() + c, c = 10) => {}, ReferenceError, "Formals defined to the right shouldn't be usable in lambdas", "Use before declaration");
        }
    },
    {
        name: "Split parameter scope with Rest",
        body: function () {
            var arr = [2, 3, 4];
            function f1(a = 10, b = function () { return a; }, ...c) {
                assert.areEqual(arr.length, c.length, "Rest parameter should contain the same number of elements as the spread arg");
                for (i = 0; i < arr.length; i++) {
                    assert.areEqual(arr[i], c[i], "Elements in the rest and the spread should be in the same order");
                }
                return b;
            }
            assert.areEqual(f1(undefined, undefined, ...arr)(), 10, "Presence of rest parameter shouldn't affect the binding");

            ((a = 10, b = () => a, ...c) => {
                assert.areEqual(arr.length, c.length, "Rest parameter should contain the same number of elements as the spread arg");
                for (i = 0; i < arr.length; i++) {
                    assert.areEqual(arr[i], c[i], "Elements in the rest and the spread should be in the same order");
                }
                return b;
            })(undefined, undefined, ...arr);
        }
    },
    {
        name: "Split parameter scope with this",
        body: function () {
            function f1(a = this.x, b = function() { assert.areEqual(this.x, 100, "this object for the function in param scope is passed from the final call site"); return a; }) {
                assert.areEqual(this.x, 10, "this objects property retains the value from param scope");
                a = 20;
                return b;
            }
            assert.areEqual(f1.call({x : 10}).call({x : 100}), 10, "Arrow functions defined in the param scope captures the formals from the param scope not body scope");
            
            (function (a = this.x, b = function() {this.x = 20}) {
                assert.areEqual(this.x, 10, "this objects property retains the value in param scope before the inner function call");
                b.call(this);
                assert.areEqual(this.x, 20, "Update to a this's property from the param scope is reflected in the body scope");
            }).call({x : 10});
            
            this.x = 10;
            ((a = this.x, b = function() {this.x = 20}) => {
                assert.areEqual(this.x, 10, "this objects property retains the value in param scope before the inner function call in lambda");
                b.call(this);
                assert.areEqual(this.x, 20, "Update to a this's property from the param scope of lambda function is reflected in the body scope");
            })();
            
            function f2(a = function() { return this.x; }, b = this.y, c = a.call({x : 20}) + b) {
                assert.areEqual(this.x, undefined, "this object remains unaffected");
                return c;
            }
            assert.areEqual(f2.call({y : 10}), 30, "Properties are accessed from the right this object");

            var thisObj = {x : 1, y: 20};            
            function f3(b = () => { this.x = 10; return this.y; }) {
                return b;
            }
            assert.areEqual(f3.call(thisObj)(), 20, "Lambda defined in the param scope returns the right property value from thisObj");
            assert.areEqual(thisObj.x, 10, "Assignment from the param scope method updates thisObj's property");
        }
    },
    {
        name: "Split parameter scope in class methods",
        body: function () {
            class c {
                f(a = 10, d, b = function () { return a; }, c) {
                    assert.areEqual(a, 10, "Initial value of parameter in the body scope in class method should be the same as the one in param scope");
                    var a = 20;
                    assert.areEqual(a, 20, "Assignment in the class method body updates the value of the variable");
                    return b;
                }
            }
            assert.areEqual((new c()).f()(), 10, "Method defined in the param scope of the class should capture the formal from the param scope itself");

            function f1(a = 10, d, b = class { method1() { return a; } }, c) {
                var a = 20;
                assert.areEqual((new b()).method1(), 10, "Class method defined within the param scope should capture the formal from the param scope");
                return b;
            }
            var result = f1();
            assert.areEqual((new result()).method1(), 10, "Methods defined in a class defined in param scope should capture the formals form that param scope itself");

            class c2 {
                f1(a = 10, d, b = function () { a = this.f2(); return a; }, c) {
                    assert.areEqual(this.f2(), 30, "this object in the body points to the right this object");
                    return b;
                };
                f2() {
                    return 30;
                }
            }
            var f2Obj = new c2();
            assert.areEqual(f2Obj.f1().call({f2() { return 100; }}), 100, "Method defined in the param uses its own this object while updating the formal");

            function f2(a = 10, d, b = class { method1() { return class { method2() { return a; }} } }, c) {
                a = 20;
                return b;
            }
            var obj1 = f2();
            var obj2 = (new obj1()).method1();
            assert.areEqual((new obj2()).method2(), 10, "Nested class definition in the param scope should capture the formals from the param scope");

            var actualArray = [2, 3, 4];
            class c3 {
                f(a = 10, b = () => { return c; }, ...c) {
                    assert.areEqual(c.length, actualArray.length, "Rest param and the actual array should have the same length");
                    for (var i = 0; i < c.length; i++) {
                        assert.areEqual(c[i], actualArray[i], "Rest parameter should have the same value as the actual array");
                    }
                    c = [];
                    return b;
                }
            }
            result = (new c3()).f(undefined, undefined, ...[2, 3, 4])();
            assert.areEqual(result.length, actualArray.length, "The result and the actual array should have the same length");
            for (var i = 0; i < result.length; i++) {
                assert.areEqual(result[i], actualArray[i], "The result array should have the same value as the actual array");
            }

            class c4 {
                f({x:x = 10, y:y = () => { return x; }}) {
                    assert.areEqual(x, 10, "Initial value of destructure parameter in the body scope in class method should be the same as the one in param scope");
                    x = 20;
                    assert.areEqual(x, 20, "Assignment in the class method body updates the value of the variable");
                    return y;
                }
            }
            assert.areEqual((new c4()).f({})(), 10, "The method defined as the default destructured value of the parameter should capture the formal from the param scope");
        }
    },
    {
        name: "Split parameter scope in generator methods",
        body: function () {
            function *f1(a = 10, d, b = function () { return a; }, c) {
                yield a;
                var a = 20;
                yield a;
                yield b;
            }
            var f1Obj = f1();
            assert.areEqual(f1Obj.next().value, 10, "Initial value of the parameter in the body scope should be the same as the final value of the parameter in param scope");
            assert.areEqual(f1Obj.next().value, 20, "Assignment in the body scope updates the variable's value");
            assert.areEqual(f1Obj.next().value(), 10, "Function defined in the param scope captures the formal from the param scope itself");

            function *f2(a = 10, d, b = function () { return a; }, c) {
                yield a;
                a = 20;
                yield a;
                yield b;
            }
            var f2Obj = f2();
            assert.areEqual(f2Obj.next().value, 10, "Initial value of the parameter in the body scope should be the same as the final value of the parameter in param scope");
            assert.areEqual(f2Obj.next().value, 20, "Assignment in the body scope updates the variable's value");
            assert.areEqual(f2Obj.next().value(), 10, "Function defined in the param scope captures the formal from the param scope itself even if it is not redeclared in the body");

            function *f3(a = 10, d, b = function *() { yield a + c; }, c = 100) {
                a = 20;
                yield a;
                yield b;
            }
            var f3Obj = f3();
            assert.areEqual(f3Obj.next().value, 20, "Assignment in the body scope updates the variable's value");
            assert.areEqual(f3Obj.next().value().next().value, 110, "Function defined in the param scope captures the formals from the param scope");

            function *f4(a = 10, d, b = function *() { yield a; }, c) {
                var a = 20;
                yield function *() { yield a; };
                yield b;
            }
            var f4Obj = f4();
            assert.areEqual(f4Obj.next().value().next().value, 20, "Generator defined inside the body captures the symbol from the body scope");
            assert.areEqual(f4Obj.next().value().next().value, 10, "Function defined in the param scope captures the formal from param scope even if it is captured in the body scope");
        }
    },
    {
        name: "Split parameter scope with destructuring",
        body: function () {
            function f1( {a:a1, b:b1}, c = function() { return a1 + b1; } ) {
                assert.areEqual(a1, 10, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
                assert.areEqual(b1, 20, "Initial value of the second destructuring parameter in the body scope should be the same as the one in param scope");
                a1 = 1;
                b1 = 2;
                assert.areEqual(a1, 1, "New assignment in the body scope updates the first formal's value in body scope");
                assert.areEqual(b1, 2, "New assignment in the body scope updates the second formal's value in body scope");
                assert.areEqual(c(), 30, "The param scope method should return the sum of the destructured formals from the param scope");
                return c;
            }
            assert.areEqual(f1({ a : 10, b : 20 })(), 30, "Returned method should return the sum of the destructured formals from the param scope");

            function f2({x:x = 10, y:y = function () { return x; }}) {
                assert.areEqual(x, 10, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
                x = 20;
                assert.areEqual(x, 20, "Assignment in the body updates the formal's value");
                return y;
            }
            assert.areEqual(f2({ })(), 10, "Returned method should return the value of the destructured formal from the param scope");
            
            function f3({y:y = function () { return x; }, x:x = 10}) {
                assert.areEqual(x, 10, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
                x = 20;
                assert.areEqual(x, 20, "Assignment in the body updates the formal's value");
                return y;
            }
            assert.areEqual(f3({ })(), 10, "Returned method should return the value of the destructured formal from the param scope even if declared after");
            
            (({x:x = 10, y:y = function () { return x; }}) => {
                assert.areEqual(x, 10, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
                x = 20;
                assert.areEqual(y(), 10, "Assignment in the body does not affect the formal captured from the param scope");
            })({});
        }
    },
    {
      name: "Nested split scopes",
      body: function () {
            function f1(a = 10, b = function () { return a; }, c) {
                function iFnc(d = 100, e = 200, pf1 = function () { return d + e; }) {
                    d = 1000;
                    e = 2000;
                    pf2 = function () { return d + e; };
                    return [pf1, pf2];
                }
                return [b].concat(iFnc());
            }
            var result = f1();
            assert.areEqual(result[0](), 10, "Function defined in the param scope of the outer function should capture the symbols from its own param scope");
            assert.areEqual(result[1](), 300, "Function defined in the param scope of the inner function should capture the symbols from its own param scope");
            assert.areEqual(result[2](), 3000, "Function defined in the body scope of the inner function should capture the symbols from its body scope");

            function f2(a = 10, b = function () { return a; }, c) {
                a = 1000;
                c = 2000;
                function iFnc(a = 100, b = function () { return a + c; }, c = 200) {
                    a = 1000;
                    c = 2000;
                    return b;
                }
                return [b, iFnc()];
            }
            result = f2();
            assert.areEqual(result[0](), 10, "Function defined in the param scope of the outer function should capture the symbols from its own param scope even if formals are with the same name in inner function");
            assert.areEqual(result[1](), 300, "Function defined in the param scope of the inner function should capture the symbols from its own param scope  if formals are with the same name in the outer function");

            function f3(a = 10, b = function () { return a; }, c) {
                a = 1000;
                c = 2000;
                function iFnc(a = 100, b = function () { return a + c; }, c = 200) {
                    a = 1000;
                    c = 2000;
                    return b;
                }
                return [b, iFnc];
            }
            assert.areEqual(f3()[1]()(), 300, "Function defined in the param scope of the inner function should capture the right formals even if the inner function is executed outside");

            function f4(a = 10, b = function () { return a; }, c) {
                a = 1000;
                function iFnc(a = 100, b = function () { return a + c; }, c = 200) {
                    a = 1000;
                    c = 2000;
                    return b;
                }
                return [b, iFnc(undefined, b, c)];
            }
            result = f4(1, undefined, 3);
            assert.areEqual(result[0](), 1, "Function defined in the param scope of the outer function correctly captures the passed in value for the formal");
            assert.areEqual(result[1](), 1, "Function defined in the param scope of the inner function is replaced by the function definition from the param scope of the outer function");

            function f5(a = 10, b = function () { return a; }, c) {
                function iFnc(a = 100, b = function () { return a + c; }, c = 200) {
                    a = 1000;
                    c = 2000;
                    return b;
                }
                return [b, iFnc(a, undefined, c)];
            }
            result = f5(1, undefined, 3);
            assert.areEqual(result[0](), 1, "Function defined in the param scope of the outer function correctly captures the passed in value for the formal");
            assert.areEqual(result[1](), 4, "Function defined in the param scope of the inner function captures the passed values for the formals");

            function f6(a , b, c) {
                function iFnc(a = 1, b = function () { return a + c; }, c = 2) {
                    a = 10;
                    c = 20;
                    return b;
                }
                return iFnc;
            }
            assert.areEqual(f6()()(), 3, "Function defined in the param scope captures the formals when defined inside another method without split scope");

            function f7(a = 10 , b = 20, c = function () { return a + b; }) {
                return (function () {
                    function iFnc(a = 100, b = function () { return a + c; }, c = 200) {
                        a = 1000;
                        c = 2000;
                        return b;
                    }
                    return [c, iFnc];
                })();
            }
            result = f7();
            assert.areEqual(result[0](), 30, "Function defined in the param scope of the outer function should capture the symbols from its own param scope even in nested case");
            assert.areEqual(result[1]()(), 300, "Function defined in the param scope of the inner function should capture the symbols from its own param scope even when nested inside a normal method and a split scope");

            function f8(a = 1, b = function (d = 10, e = function () { return a + d; }) { assert.areEqual(d, 10, "Split scope function defined in param scope should capture the right formal value"); d = 20; return e; }, c) {
                a = 2;
                return b;
            }
            assert.areEqual(f8()()(), 11, "Split scope function defined within the param scope should capture the formals from the corresponding param scopes");

            function f9(a = 1, b = function () { return function (d = 10, e = function () { return a + d; }) { d = 20; return e; } }, c) {
                a = 2;
                return b;
            }
            assert.areEqual(f9()()()(), 11, "Split scope function defined within the param scope should capture the formals from the corresponding param scope in nested scope");

      }  
    },
    {
        name: "Split parameter scope and eval",
        body: function () {
            function g() {
                return 3 * 3;
            }

            function f(h = () => eval("g()")) { // cannot combine scopes
                function g() {
                    return 2 * 3;
                }
                return h(); // 9
            }
            // TODO(tcare): Re-enable when split scope support is implemented
            //assert.areEqual(9, f(), "Paramater scope remains split");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });