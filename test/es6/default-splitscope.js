//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var v15 = 1;
var tests = [
  {
    name: "Split parameter scope in function definition",
    body: function () {
        function f1(a = 10, b = function () { return a; }) {
            assert.areEqual(10, a, "Initial value of parameter in the body scope should be the same as the one in param scope");
            var a = 20;
            assert.areEqual(20, a, "New assignment in the body scope updates the variable's value in body scope");
            return b;
        }
        assert.areEqual(10, f1()(), "Function defined in the param scope captures the formals from the param scope not body scope");

        function f2(a = 10, b = function () { return a; }, c = b() + a) {
            assert.areEqual(10, a, "Initial value of parameter in the body scope should be the same as the one in param scope");
            assert.areEqual(20, c, "Initial value of the third parameter in the body scope should be twice the value of the first parameter");
            var a = 20;
            assert.areEqual(20, a, "New assignment in the body scope updates the variable's value in body scope");
            return b;
        }
        assert.areEqual(10, f2()(), "Function defined in the param scope captures the formals from the param scope not body scope");

        function f3(a = 10, b = function () { return a; }) {
            assert.areEqual(1, a, "Initial value of parameter in the body scope should be the same as the one passed in");
            var a = 20;
            assert.areEqual(20, a, "Assignment in the body scope updates the variable's value in body scope");
            return b;
        }
        assert.areEqual(f3(1)(), 1, "Function defined in the param scope captures the formals from the param scope even when the default expression is not applied for that param");

        (function (a = 10, b = a += 10, c = function () { return a + b; }) {
            assert.areEqual(20, a, "Initial value of parameter in the body scope should be same as the corresponding symbol's final value in the param scope");
            var a2 = 40;
            (function () { assert.areEqual(40, a2, "Symbols defined in the body scope should be unaffected by the duplicate formal symbols"); })();
            assert.areEqual(40, c(), "Function defined in param scope uses the formals from param scope even when executed inside the body");
        })();

        (function (a = 10, b = function () { assert.areEqual(10, a, "Function defined in the param scope captures the formals from the param scope when executed from the param scope"); }, c = b()) {
        })();

        function f4(a = 10, b = function () { return function () { return a; }; }) {
            var a = 20;
            return b;
        }
        assert.areEqual(10, f4()()(), "Parameter scope works fine with nested functions");

        var a1 = 10;
        function f5(a, b = function () { a; return a1; }) {
            assert.areEqual(undefined, a1, "Inside the function body the assignment hasn't happened yet");
            var a1 = 20;
            assert.areEqual(20, a1, "Assignment to the symbol inside the function changes the value");
            return b;
        }
        assert.areEqual(10, f5()(), "Function in the param scope correctly binds to the outer variable");

        function f6(a = 10, b = { iFnc () { return a; } }) {
            var a = 20;
            return b;
        }
        assert.areEqual(10, f6().iFnc(), "Function definition inside the object literal should capture the formal from the param scope");

        function f7(a = 10, b = function () { return a; }) {
            assert.areEqual(10, a, "Initial value of parameter in the body scope should be the same as the one in param scope");
            a = 100;
            assert.areEqual(100, a, "New assignment in the body scope updates the value of the formal scope");
            return b;
        }
        assert.areEqual(100, f7()(), "Chnage in the value of the formal is reflected in the param scoped function's capture");

        var f8 = function (a, b = ((function() { assert.areEqual('string1', a, "First argument receives the right value"); })(), 1), c) {
            var d = 'string3';
            (function () { assert.areEqual('string3', d, "Var declaration in the body is initialized properly"); })();
            return c;
        };
        assert.areEqual('string2', f8('string1', undefined, 'string2'), "Function returns the third argument properly");

        function f9() {
            var f10 = function (a = function () { c; }, b, c) {
                assert.areEqual(1, c, "Third argument is properly populated");
                arguments;
                function f11() {};
            };
            f10(undefined, undefined, 1);
        }
        f9();
        f9();

        function f10() {
            var result = ((a = (w = a => a * a) => w) => a)()()(10);

            assert.areEqual(100, result, "The inner lambda function properly maps to the right symbol for a");
        };
        f10();

        function f11(a = 10, b = 20, c = () => [a, b]) {
            assert.areEqual(10, c()[0], "Before the assignment in the body the value of the symbol is the body is same as the formal formal");
            assert.areEqual(20, c()[1], "Before the assignment in the body updates the value of the formal is retained");
            var a = 20;
            b = 200;
            assert.areEqual(20, a, "New assignment in the body scope updates the variable's value in body scope");
            return c;
        }
        assert.areEqual(10, f11()()[0], "Assignment in the body does not affect the value of the formal");
        assert.areEqual(200, f11()()[1], "Assignment in the body updates the value of the formal");
    }
 },
 {
    name: "Split parameter scope and function expressions with name",
    body: function () {
        function f1(a = 10, b = function c() { return a; }) {
            assert.areEqual(10, a, "Initial value of parameter in the body scope of the method should be the same as the one in param scope");
            var a = 20;
            assert.areEqual(20, a, "New assignment in the body scope of the method updates the variable's value in body scope");
            return b;
        }
        assert.areEqual(10, f1()(), "Function expression defined in the param scope captures the formals from the param scope not body scope");

        function f2(a = 10, b = function c(recurse = true) { return recurse ? c(false) : a; }) {
            return b;
        }
        assert.areEqual(10, f2()(), "Recursive function expression defined in the param scope captures the formals from the param scope not body scope");

        var f3 = function f4 (a = function ( ) { b; return f4(20); }, b) {
            if (a == 20) {
                return 10;
            }
            return a;
        }
        assert.areEqual(10, f3()(), "Recursive call to the function from the param scope returns the right value");

        var f5 = function f6 (a = function ( ) { b; return f6; }, b) {
            if (a == 20) {
                return 10;
            }
            return a;
        }
        assert.areEqual(10, f5()()(20), "Recursive call to the function from the param scope returns the right value");

        var f7 = function f8 (a = function ( ) { b; }, b) {
            if (a == 20) {
                return 10;
            }
            var a = function () { return f8(20); };
            return a;
        }
        assert.areEqual(10, f7()(), "Recursive call to the function from the body scope returns the right value");

        var f9 = function f10 (a = function ( ) { b; return f10(20); }, b) {
            eval("");
            if (a == 20) {
                return 10;
            }
            return a;
        }
        assert.areEqual(10, f9()(), "Recursive call to the function from the param scope returns the right value when eval is there in the body");

        var f11 = function f12 (a = function ( ) { b; }, b) {
            eval("");
            if (a == 20) {
                return 10;
            }
            var a = function () { return f12(20); };
            return a;
        }
        assert.areEqual(10, f11()(), "Recursive call to the function from the body scope returns the right value when eval is there in the body");

        function f13() {
            var a = function jnvgfg(sfgnmj = function ccunlk() { jnvgfg(undefined, 1); }, b) {
                if (b) {
                    assert.areEqual(undefined, jnvgfg, "This refers to the instance in the body and the value of the function expression is not copied over");
                }
                var jnvgfg = 10;
                if (!b) {
                    sfgnmj();
                    return 100;
                }
            };
            assert.areEqual(100, a(), "After the recursion the right value is returned by the split scoped function");
        };
        f13();

        var f14 = function f15(a = (function() {
                return f15(1);
            })()) {
                with({}) {
                };
                return a === 1 ? 10 : a;
        };
        assert.areEqual(10, f14(), "Function expresison is captured in the param scope when no other formals are captured");
    }
 },
 {
    name: "Split parameter scope in member functions",
    body: function () {
       var o1 = {
           f(a = 10, b = function () { return a; }) {
               assert.areEqual(10, a, "Initial value of parameter in the body scope of the method should be the same as the one in param scope");
               var a = 20;
               assert.areEqual(20, a, "New assignment in the body scope of the method updates the variable's value in body scope");
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
        assert.areEqual(10, result[0]().f2(), "Short hand method defined in the param scope of the object method captures the formals from the param scope not body scope");
        assert.areEqual(20, result[1]().f2(), "Short hand method defined in the param scope of the object method captures the formals from the param scope not body scope");

        var o3 = {
           f(a = 10, b = function () { return a; }) {
               assert.areEqual(10, a, "Initial value of the formal in the method should be the same as the one in param scope");
               a = 20;
               assert.areEqual(20, a, "New assignment in the body of the method updates the formal's value in body scope");
                return b;
            }
        }
        assert.areEqual(o3.f()(), 20, "Update to the value of the formal in the method's body is reflected in the capture");
    }
  },
  {
    name: "Arrow functions in split param scope",
    body: function () {
        function f1(a = 10, b = () => { return a; }) {
            assert.areEqual(10, a, "Initial value of parameter in the body scope should be the same as the one in param scope");
            var a = 20;
            assert.areEqual(20, a, "New assignment in the body scope updates the variable's value in body scope");
            return b;
        }
        assert.areEqual(10, f1()(), "Arrow functions defined in the param scope captures the formals from the param scope not body scope");

        function f2(a = 10, b = () => { return a; }) {
            assert.areEqual(1, a, "Initial value of parameter in the body scope should be the same as the one in param scope");
            var a = 20;
            assert.areEqual(20, a, "New assignment in the body scope updates the variable's value in body scope");
            return b;
        }
        assert.areEqual(1, f2(1)(), "Arrow functions defined in the param scope captures the formals from the param scope not body scope even when value is passed");

        function f3(a = 10, b = () => a) {
            assert.areEqual(10, a, "Initial value of parameter in the body scope should be the same as the one in param scope");
            var a = 20;
            assert.areEqual(20, a, "New assignment in the body scope updates the variable's value in body scope");
            return b;
        }
        assert.areEqual(10, f3()(), "Arrow functions with concise body defined in the param scope captures the formals from the param scope not body scope");

        ((a = 10, b = a += 10, c = () => { assert.areEqual(20, a, "Value of the first formal inside the lambda should be same as the default value"); return a + b; }, d = c() * 10) => {
            assert.areEqual(d, 400, "Initial value of the formal parameter inside the body should be the same as final value from the param scope");
        })();

        function f4(a = 10, b = () => { return () => a; }) {
            var a = 20;
            return b;
        }
        assert.areEqual(10, f4()()(), "Nested lambda should capture the formal param value from the param scope");

        assert.throws(function f5(a = () => x) { var x = 1; a(); }, ReferenceError, "Lambdas in param scope shouldn't be able to access the variables from body", "'x' is not defined");
        assert.throws(function f6() { (function (a = () => x) { var x = 1; return a; })()(); }, ReferenceError, "Lambdas in param scope shouldn't be able to access the variables from body", "'x' is not defined");
        assert.throws((a = () => 10, b = a() + c, c = 10) => {}, ReferenceError, "Formals defined to the right shouldn't be usable in lambdas", "Use before declaration");

        function f7(a = 10, b = () => { return a; }) {
            assert.areEqual(10, a, "Initial value of parameter in the body scope should be the same as the one in param scope");
            a = 20;
            assert.areEqual(20, a, "New assignment in the body scope updates the formal's value in body scope");
            return b;
        }
        assert.areEqual(20, f7()(), "Assignment in the function body is reflected in the arrow function's capture");
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
        function f1(a = this.x, b = function() { assert.areEqual(100, this.x, "this object for the function in param scope is passed from the final call site"); return a; }) {
            assert.areEqual(10, this.x, "this objects property retains the value from param scope");
            var a = 20;
            return b;
        }
        assert.areEqual(10, f1.call({x : 10}).call({x : 100}), "Arrow functions defined in the param scope captures the formals from the param scope not body scope");

        (function (a = this.x, b = function() {this.x = 20; return a;}) {
            assert.areEqual(10, this.x, "this objects property retains the value in param scope before the inner function call");
            b.call(this);
            assert.areEqual(20, this.x, "Update to a this's property from the param scope is reflected in the body scope");
        }).call({x : 10});

        this.x = 10;
        ((a = this.x, b = function() { a; this.x = 20; }) => {
            assert.areEqual(10, this.x, "this objects property retains the value in param scope before the inner function call in lambda");
            b.call(this);
            assert.areEqual(20, this.x, "Update to a this's property from the param scope of lambda function is reflected in the body scope");
        })();

        function f2(a = function() { return this.x; }, b = this.y, c = a.call({x : 20}) + b) {
            assert.areEqual(undefined, this.x, "this object remains unaffected");
            return c;
        }
        assert.areEqual(30, f2.call({y : 10}), "Properties are accessed from the right this object");

        var thisObj = {x : 1, y : 20 };
        function f3(a, b = () => { a; this.x = 10; return this.y; }) {
            assert.areEqual(1, this.x, "Assignment from the param scope has not happened yet");
            assert.areEqual(20, this.y, "y property of the this object is not affected");
            return b;
        }
        assert.areEqual(20, f3.call(thisObj)(), "Lambda defined in the param scope returns the right property value from thisObj");
        assert.areEqual(10, thisObj.x, "Assignment from the param scope method updates thisObj's property");

        function f4(a, b = () => { a; return this; }) {
            return b;
        }
        assert.areEqual(thisObj, f4.call(thisObj)(), "Lambda defined in the param scope returns the right this object");

        var thisObj = { x : 1 };
        function f5() {
            return (a = this, b = function() { return a; }) => b;
        }
        assert.areEqual(thisObj, f5.call(thisObj)()(), "This object is returned properly from the inner lambda method's child function");

        function f6(a, b = function () { return a; }) {
            return (a = this, b = function() { return a; }) => b;
        }
        assert.areEqual(thisObj, f6.call(thisObj)()(), "This object is returned properly from the inner lambda defnied inside a split scoped function");

        function f7(a, b = function () { return a; }) {
            function f8() {
                return (a = this, b = function() { return a; }) => b;
            }
            return f8.call(this);
        }
        assert.areEqual(thisObj, f7.call(thisObj)()(), "This object is returned properly from the inner lambda defnied inside a nested split scoped function");

        function f9(a, b = function () { return a; }) {
            function f10(c, d = function () { c; }) {
                return (a = this, b = function() { return a; }) => b;
            }
            return f10.call(this);
        }
        assert.areEqual(thisObj, f9.call(thisObj)()(), "This object is returned properly from the inner lambda defnied inside a double nested split scoped function");

        function f11(a = this.x * 10, b = () => { a; return this; }) {
            assert.areEqual(10, a, "this should be accessible in the parameter scope");
            assert.areEqual(thisObj, this, "Body scope should get the right value for this object");
            assert.isTrue(eval("thisObj == this"), "Eval should be able to access the this object properly");
            return b;
        }
        assert.areEqual(thisObj, f11.call(thisObj)(), "Lambda defined in the param scope returns the right this object");

        function f12(a = this.x * 10, b = () => { a; return this; }) {
            var c = 100;
            assert.areEqual(10, a, "this should be accessible in the parameter scope");
            assert.areEqual(thisObj, this, "Body scope should get the right value for this object");
            assert.isTrue(eval("thisObj == this"), "Eval should be able to access the this object properly");
            assert.areEqual(thisObj, (() => this)(), "Lambda should capture the this object from body properly");
            assert.areEqual(100, c, "Body variable should be unaffected by the slot allocation of this object");
            return b;
        }
        assert.areEqual(thisObj, f12.call(thisObj)(), "Lambda defined in the param scope returns the right this object");

        function f13(a = 10, b = () => { a; return this; }) {
            var c = 100;
            assert.areEqual(thisObj, this, "Body scope should get the right value for this object");
            var d = () => this;
            this.x = 5;
            assert.isTrue(eval("this.x == 5"), "Eval should be able to access the this object properly after the field is updated");
            assert.isTrue(eval("d().x == 5"), "Lambda should capture the this symbol from the body properly");
            assert.isTrue(eval("a == 10"), "Eval should be able to access the first parameter properly");
            assert.isTrue(eval("b().x == 5"), "Lambda from the param scope should capture the this symbol properly");
            assert.isTrue(eval("d().x == 5"), "Lambda should capture the this symbol from the body properly");
            return b;
        }
        assert.areEqual(5, f13.call(thisObj)().x, "Lambda defined in the param scope returns the same this object as the one in body");

        function f14(a = this.x, b = function() { assert.areEqual(100, this.x, "this object for the function in param scope is passed from the final call site"); return a; }) {
            assert.areEqual(10, this.x, "this objects property retains the value from param scope when body does not have any duplicate formals");
            a = 20;
            return b;
        }
        assert.areEqual(20, f14.call({x : 10}).call({x : 100}), "Update to the value of the formal in the body scope is reflected in the param scope function's capture");
    }
  },
  {
    name: "Split parameter scope and class",
    body: function () {
        class c1 {
            f(a = 10, d, b = function () { return a; }, c) {
                assert.areEqual(10, a, "Initial value of parameter in the body scope in class method should be the same as the one in param scope");
                var a = 20;
                assert.areEqual(20, a, "Assignment in the class method body updates the value of the variable");
                return b;
            }
        }
        assert.areEqual(10, (new c1()).f()(), "Method defined in the param scope of the class should capture the formal from the param scope itself");

        function f1(a = 10, d, b = class { method1() { return a; } }, c) {
            var a = 20;
            assert.areEqual(10, (new b()).method1(), "Class method defined within the param scope should capture the formal from the param scope");
            return b;
        }
        var result = f1();
        assert.areEqual(10, (new result()).method1(), "Methods defined in a class defined in param scope should capture the formals form that param scope itself");

        class c2 {
            f1(a = 10, d, b = function () { a = this.f2(); return a; }, c) {
                assert.areEqual(30, this.f2(), "this object in the body points to the right this object");
                return b;
            };
            f2() {
                return 30;
            }
        }
        var f2Obj = new c2();
        assert.areEqual(100, f2Obj.f1().call({f2() { return 100; }}), "Method defined in the param uses its own this object while updating the formal");

        function f2(a = 10, d, b = class { method1() { return class { method2() { return a; }} } }, c) {
            var a = 20;
            return b;
        }
        var obj1 = f2();
        var obj2 = (new obj1()).method1();
        assert.areEqual(10, (new obj2()).method2(), "Nested class definition in the param scope should capture the formals from the param scope");

        var actualArray = [2, 3, 4];
        class c3 {
            f(a = 10, b = () => { return c; }, ...c) {
                assert.areEqual(actualArray.length, c.length, "Rest param and the actual array should have the same length");
                for (var i = 0; i < c.length; i++) {
                    assert.areEqual(actualArray[i], c[i], "Rest parameter should have the same value as the actual array");
                }
                var c = [];
                return b;
            }
        }
        result = (new c3()).f(undefined, undefined, ...[2, 3, 4])();
        assert.areEqual(actualArray.length, result.length, "The result and the actual array should have the same length");
        for (var i = 0; i < result.length; i++) {
            assert.areEqual(actualArray[i], result[i], "The result array should have the same value as the actual array");
        }

        class c4 {
            f({x:x = 10, y:y = () => { return x; }}) {
                assert.areEqual(10, x, "Initial value of destructure parameter in the body scope in class method should be the same as the one in param scope");
                var x = 20;
                assert.areEqual(20, x, "Assignment in the class method body updates the value of the variable");
                return y;
            }
        }
        assert.areEqual(10, (new c4()).f({})(), "The method defined as the default destructured value of the parameter should capture the formal from the param scope");

        function f3(a = 10, d, b = (function () { return a; }, class { method1() { return a; } }), c) {
            var a = 20;
            assert.areEqual(10, (new b()).method1(), "Class method defined within the param scope should capture the formal from the param scope");
            return b;
        }
        result = f3();
        assert.areEqual(10, (new result()).method1(), "Methods defined in a class defined, after another function definition, in the param scope should capture the formals form that param scope itself");

        function f4(a = 10, d, b = (function () { return a; }, class {}, class { method1() { return a; } }), c) {
            var a = 20;
            return b;
        }
        result = f4();
        assert.areEqual(10, (new result()).method1(), "Methods defined in a class defined, after another class definition, in the param scope should capture the formals form that param scope itself");

        function f5(a = 10, d, b = (function () { return a; }, class {}, function () {}, class { method1() { return a; } }), c) {
            var a = 20;
            return b;
        }
        result = f5();
        assert.areEqual(10, (new result()).method1(), "Methods defined in a class defined, after a function and class, in the param scope should capture the formals form that param scope itself");

        function f6(a = 10, d, b = (function () { return a; }, class {}, function (a, b = () => a) {}, class { method1() { return a; } }), c) {
            var a = 20;
            return b;
        }
        result = f6();
        assert.areEqual(10, (new result()).method1(), "Methods defined in a class defined, after a split scope function, in the param scope should capture the formals form that param scope itself");

        function f7(a = 10, d, b = (function () { return a; }, class c1 { method1() { return a; } }), c) {
            var a = 20;
            assert.areEqual(10, (new b()).method1(), "Class method defined within the param scope should capture the formal from the param scope");
            return b;
        }
        result = f7();
        assert.areEqual(10, (new result()).method1(), "Methods defined in a class with name defined, after another function definition, in the param scope should capture the formals form that param scope itself");

        function f8(a = 10, d, b = class c1 { method1() { return a; } }, c = (function () { return a; }, class c2 extends b { method2() { return a * a; } })) {
            var a = 20;
            assert.areEqual(10, (new b()).method1(), "Class method defined within the param scope should capture the formal from the param scope");
            return c;
        }
        result = f8();
        assert.areEqual(10, (new result()).method1(), "Methods defined in a class extending another class defined, after another function definition, in the param scope should capture the formals form that param scope itself");
        assert.areEqual(100, (new result()).method2(), "Method in the derived class returns the right value");

        class c5 {
            f(a = 10, d, b = function () { return a; }, c) {
                assert.areEqual(10, a, "Initial value of parameter in the body scope in class method should be the same as the one in param scope");
                a = 20;
                assert.areEqual(20, a, "Assignment in the class method body updates the value of the variable");
                return b;
            }
        }
        assert.areEqual(20, (new c5()).f()(), "Update to the value of the formal in the body is reflected in the param function's capture");

        class c6 {
            f(a = 10, b = () => { return c; }, ...c) {
                assert.areEqual(actualArray.length, c.length, "Rest param and the actual array should have the same length");
                for (var i = 0; i < c.length; i++) {
                    assert.areEqual(actualArray[i], c[i], "Rest parameter should have the same value as the actual array");
                }
                c = [];
                return b;
            }
        }
        result = (new c6()).f(undefined, undefined, ...[2, 3, 4])();
        assert.areEqual(0, result.length, "Assignment to the rest formal in the body is reflected in the param scoped function's capture");
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
        assert.areEqual(10, f1Obj.next().value, "Initial value of the parameter in the body scope should be the same as the final value of the parameter in param scope");
        assert.areEqual(20, f1Obj.next().value, "Assignment in the body scope updates the variable's value");
        assert.areEqual(10, f1Obj.next().value(), "Function defined in the param scope captures the formal from the param scope itself");

        function *f2(a = 10, d, b = function () { return a; }, c) {
            yield a;
            a = 20;
            yield a;
            yield b;
        }
        var f2Obj = f2();
        assert.areEqual(10, f2Obj.next().value, "Initial value of the parameter in the body scope should be the same as the final value of the parameter in param scope");
        assert.areEqual(20, f2Obj.next().value, "Assignment in the body scope updates the variable's value");
        assert.areEqual(20, f2Obj.next().value(), "Function defined in the param scope captures the formal from the param scope which gets updated by the assignment in the body");

        function *f3(a = 10, d, b = function *() { yield a + c; }, c = 100) {
            var a = 20;
            yield a;
            yield b;
        }
        var f3Obj = f3();
        assert.areEqual(20, f3Obj.next().value, "Assignment in the body scope updates the variable's value");
        assert.areEqual(110, f3Obj.next().value().next().value, "Function defined in the param scope captures the formals from the param scope");

        function *f4(a = 10, d, b = function *() { yield a; }, c) {
            var a = 20;
            yield function *() { yield a; };
            yield b;
        }
        var f4Obj = f4();
        assert.areEqual(20, f4Obj.next().value().next().value, "Generator defined inside the body captures the symbol from the body scope");
        assert.areEqual(10, f4Obj.next().value().next().value, "Function defined in the param scope captures the formal from param scope even if it is captured in the body scope");
    }
  },
  {
    name: "Split parameter scope with destructuring",
    body: function () {
        function f1( {a:a1, b:b1}, c = function() { return a1 + b1; } ) {
            assert.areEqual(10, a1, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
            assert.areEqual(20, b1, "Initial value of the second destructuring parameter in the body scope should be the same as the one in param scope");
            var a1 = 1;
            var b1 = 2;
            assert.areEqual(1, a1, "New assignment in the body scope updates the first formal's value in body scope");
            assert.areEqual(2, b1, "New assignment in the body scope updates the second formal's value in body scope");
            assert.areEqual(30, c(), "The param scope method should return the sum of the destructured formals from the param scope");
            return c;
        }
        assert.areEqual(30, f1({ a : 10, b : 20 })(), "Returned method should return the sum of the destructured formals from the param scope");

        function f2({x:x = 10, y:y = function () { return x; }}) {
            assert.areEqual(10, x, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
            var x = 20;
            assert.areEqual(20, x, "Assignment in the body updates the formal's value");
            return y;
        }
        assert.areEqual(10, f2({ })(), "Returned method should return the value of the destructured formal from the param scope");

        function f3({y:y = function () { return x; }, x:x = 10}) {
            assert.areEqual(10, x, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
            var x = 20;
            assert.areEqual(20, x, "Assignment in the body updates the formal's value");
            return y;
        }
        assert.areEqual(10, f3({ })(), "Returned method should return the value of the destructured formal from the param scope even if declared after");

        (({x:x = 10, y:y = function () { return x; }}) => {
            assert.areEqual(10, x, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
            var x = 20;
            assert.areEqual(10, y(), "Assignment in the body does not affect the formal captured from the param scope");
        })({});

        function f4( {a:a1, b:b1}, c = function() { return a1 + b1; } ) {
            assert.areEqual(10, a1, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
            assert.areEqual(20, b1, "Initial value of the second destructuring parameter in the body scope should be the same as the one in param scope");
            a1 = 1;
            b1 = 2;
            assert.areEqual(1, a1, "New assignment in the body scope updates the first formal's value in body scope");
            assert.areEqual(2, b1, "New assignment in the body scope updates the second formal's value in body scope");
            assert.areEqual(3, c(), "The param scope method should return the sum of the destructured formals from the param scope and reflect the value update in the body");
            return c;
        }
        assert.areEqual(3, f4({ a : 10, b : 20 })(), "Returned method should return the sum of the destructured formals which are updated in the body");
    }
  },
  {
    name: "Nested split scopes",
    body: function () {
          function f1(a = 10, b = function () { return a; }, c) {
              function iFnc(d = 100, e = 200, pf1 = function () { return d + e; }) {
                  var d = 1000;
                  var e = 2000;
                  pf2 = function () { return d + e; };
                  return [pf1, pf2];
              }
              return [b].concat(iFnc());
          }
          var result = f1();
          assert.areEqual(10, result[0](), "Function defined in the param scope of the outer function should capture the symbols from its own param scope");
          assert.areEqual(300, result[1](), "Function defined in the param scope of the inner function should capture the symbols from its own param scope");
          assert.areEqual(3000, result[2](), "Function defined in the body scope of the inner function should capture the symbols from its body scope");

          function f2(a = 10, b = function () { return a; }, c) {
              var a = 1000;
              var c = 2000;
              function iFnc(a = 100, b = function () { return a + c; }, c = 200) {
                  var a = 1000;
                  var c = 2000;
                  return b;
              }
              return [b, iFnc()];
          }
          result = f2();
          assert.areEqual(10, result[0](), "Function defined in the param scope of the outer function should capture the symbols from its own param scope even if formals are with the same name in inner function");
          assert.areEqual(300, result[1](), "Function defined in the param scope of the inner function should capture the symbols from its own param scope  if formals are with the same name in the outer function");

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
          result = f3();
          assert.areEqual(1000, result[0](), "Assignments in the body of the outer function updates the formal's value");
          assert.areEqual(3000, result[1]()(), "Assignments in the body of the inner function updates the formal values");

          function f4(a = 10, b = function () { return a; }, c) {
              var a = 1000;
              function iFnc(a = 100, b = function () { return a + c; }, c = 200) {
                  var a = 1000;
                  var c = 2000;
                  return b;
              }
              return [b, iFnc(undefined, b, c)];
          }
          result = f4(1, undefined, 3);
          assert.areEqual(1, result[0](), "Function defined in the param scope of the outer function correctly captures the passed in value for the formal");
          assert.areEqual(1, result[1](), "Function defined in the param scope of the inner function is replaced by the function definition from the param scope of the outer function");

          function f5(a = 10, b = function () { return a; }, c) {
              function iFnc(a = 100, b = function () { return a + c; }, c = 200) {
                  var a = 1000;
                  var c = 2000;
                  return b;
              }
              return [b, iFnc(a, undefined, c)];
          }
          result = f5(1, undefined, 3);
          assert.areEqual(1, result[0](), "Function defined in the param scope of the outer function correctly captures the passed in value for the formal");
          assert.areEqual(4, result[1](), "Function defined in the param scope of the inner function captures the passed values for the formals");

          function f6(a , b, c) {
              function iFnc(a = 1, b = function () { return a + c; }, c = 2) {
                  var a = 10;
                  var c = 20;
                  return b;
              }
              return iFnc;
          }
          assert.areEqual(3, f6()()(), "Function defined in the param scope captures the formals when defined inside another method without split scope");

          function f7(a = 10 , b = 20, c = function () { return a + b; }) {
              return (function () {
                  function iFnc(a = 100, b = function () { return a + c; }, c = 200) {
                      var a = 1000;
                      var c = 2000;
                      return b;
                  }
                  return [c, iFnc];
              })();
          }
          result = f7();
          assert.areEqual(30, result[0](), "Function defined in the param scope of the outer function should capture the symbols from its own param scope even in nested case");
          assert.areEqual(300, result[1]()(), "Function defined in the param scope of the inner function should capture the symbols from its own param scope even when nested inside a normal method and a split scope");

          function f8(a = 1, b = function (d = 10, e = function () { return a + d; }) { assert.areEqual(d, 10, "Split scope function defined in param scope should capture the right formal value"); d = 20; return e; }, c) {
              var a = 2;
              return b;
          }
          assert.areEqual(21, f8()()(), "Split scope function defined within the param scope should capture the formals from the corresponding param scopes");

          function f9(a = 1, b = function () { return function (d = 10, e = function () { return a + d; }) { d = 20; return e; } }, c) {
              var a = 2;
              return b;
          }
          assert.areEqual(21, f9()()()(), "Split scope function defined within the param scope should capture the formals from the corresponding param scope in nested scope");

          function f10(a = 10, b = () => eval("a"), c) {
              function iFnc(d = 100, e = 200, pf1 = function () { return d + e; }) {
                  d = 1000;
                  e = 2000;
                  pf2 = function () { return d + e; };
                  return [pf1, pf2];
              }
              return [b].concat(iFnc());
          }
          result = f10();
          assert.areEqual(10, result[0](), "Function with eval defined in the param scope of the outer function should capture the symbols from its own param scope");
          assert.areEqual(3000, result[1](), "Function defined in the param scope of the inner function should capture the symbols from its own param scope");
          assert.areEqual(3000, result[2](), "Function defined in the body scope of the inner function should capture the symbols from its body scope");

          function f11(a = 10, b = () => eval("a"), c) {
              function iFnc(d = 100, e = 200, pf1 = eval("d + e")) {
                  var d = 1000;
                  var e = 2000;
                  pf2 = function () { return d + e; };
                  return [pf1, pf2];
              }
              return [b].concat(iFnc());
          }
          result = f11();
          assert.areEqual(10, result[0](), "Function with eval defined in the param scope of the outer function should capture the symbols from its own param scope");
          assert.areEqual(300, result[1], "Second formal of the inner function should be able capture the symbols from its own param scope");
          assert.areEqual(3000, result[2](), "Function defined in the body scope of the inner function should capture the symbols from its body scope");

          function f12(a = 10, b = function () { return a; }, c) {
              function iFnc(d = 100, e = 200, pf1 = () => { return eval("d + e"); }) {
                  var d = 1000;
                  var e = 2000;
                  pf2 = function () { return d + e; };
                  return [pf1, pf2];
              }
              return [b].concat(iFnc());
          }
          result = f12();
          assert.areEqual(10, result[0](), "Function with eval defined in the param scope of the outer function should capture the symbols from its own param scope");
          assert.areEqual(300, result[1](), "Function with eval in the param defined in the param scope of the inner function should capture the symbols from its own param scope");
          assert.areEqual(3000, result[2](), "Function with eval in the param defined in the body scope of the inner function should capture the symbols from its body scope");

          function f13(a = 10, b = function () { return a; }, c) {
              function iFnc(d = 100, e = 200, pf1 = () => { return eval("d + e"); }) {
                  d = 1000;
                  var e = 2000;
                  pf2 = function () { return d + e; };
                  a = 100;
                  return [pf1, pf2];
              }
              return [b].concat(iFnc());
          }
          result = f13();
          assert.areEqual(100, result[0](), "Function with eval defined in the param scope of the outer function should capture the symbols from its own param scope");
          assert.areEqual(1200, result[1](), "The first formal is updated in the function body but the second formal is duplicated");
          assert.areEqual(3000, result[2](), "Function with eval in the param defined in the body scope of the inner function should capture the symbols from its body scope and the param scope");
    }
  },
  {
    name: "Split scope with symbol shadowing",
    body: function () {
        function f1(a = 10, b = function () { return a; }) {
            assert.areEqual(100, a(), "Function definition inside the body is hoisted");
            function a () {
                return 100;
            }
            return b;
        }
        assert.areEqual(10, f1()(), "Function definition in the param scope captures the symbol from the param scope");

        function f2(a = 10, b = function () { return a; }, c = b) {
            var a = 20;
            assert.areEqual(20, b(), "Function definition in the body scope captures the body symbol");
            function b() {
                return a;
            }
            return [c, b];
        }
        var result = f2();
        assert.areEqual(10, result[0](), "Function definition in the param scope captures the param scope symbol");
        assert.areEqual(20, result[1](), "Function definition in the body captures the body symbol");

        var g = 1;
        function f3(a = 10, b = function () { a; return g;}) {
            assert.areEqual(10, g(), "Function definition inside the body is unaffected by the outer variable");
            function g() {
                return 10;
            }
            return b;
        }
        assert.areEqual(1, f3()(), "Function definition in the param scope captures the outer scoped var");

        function f4(a = x1, b = function g() {
            a;
            return function h() {
                assert.areEqual(10, x1, "x1 is captured from the outer scope");
            };
        }) {
            var x1 = 100;
            b()();
        };
        var x1 = 10;
        f4();

        var x2 = 1;
        function f5(a = x2, b = function() { a; return x2; }) {
            {
                function x2() {
                }
            }
            var x2 = 2;
            return b;
        }
        assert.areEqual(1, f5()(), "Symbol capture at the param scope is unaffected by the inner definitions");

        var x3 = 1;
        function f6(a = x3, b = function(_x) { a; return x3; }) {
            var x3 = 2;
            return b;
        }
        assert.areEqual(1, f6()(), "Symbol capture at the param scope is unaffected by other references in the body and param");

        function f7(a = 10, b = function () { return a; }, c) {
            assert.areEqual(100, a(), "Function definition inside the body is hoisted");
            function a () {
                return c;
            }
            return [b, c];
        }
        result = f7(undefined, undefined, 100);
        assert.areEqual(10, result[0](), "Function definition in the param scope captures the symbol from the param scope");
        assert.areEqual(100, result[1], "Function defined in the body overriding the formal should be able to capture another formal");

        function f8(a = 10, b = function () { return a; }, c) {
            var a = function a () {
                return c;
            }
            assert.areEqual(100, a(), "Function definition inside the body is hoisted");
            return [b, c];
        }
        result = f8(undefined, undefined, 100);
        assert.areEqual(10, result[0](), "Function definition in the param scope captures the symbol from the param scope");
        assert.areEqual(100, result[1], "Function expression with name defined in the body overriding the formal should be able to capture another formal");
    }
  },
  {
    name : "Split scope and arguments symbol in the body",
    body : function () {
        function f1(a, b = () => a) {
            eval("");
            b = () => { return arguments; };
            assert.areEqual(1, arguments[0], "Arguments object receives the first parameter properly");
            assert.areEqual(1, b()[0], "First argument receives the right value passed in");
            assert.areEqual(undefined, b()[1], "Second argument receives the right value passed in");
            assert.areEqual(2, arguments.length, "Arguments should have only two elements in it");
        }
        f1(1, undefined);

        function f2(a, b = () => { return a; }) {
            a = 10;
            assert.areEqual(1, arguments[0], "First argument is properly received");
            assert.areEqual(2, arguments[2], "Third argument is properly received");
            assert.areEqual(3, arguments.length, "Only three arguments are passed in");
            (() => { arguments = [3, 4]; a; })();
            assert.areEqual(3, arguments[0], "Arguments symbol is updated with the new value when the lambda is executed");
            assert.areEqual(4, arguments[1], "New array is properly assigned to arguments symbol");
            assert.areEqual(2, arguments.length, "New array has only elements");

            return b;
        }
        assert.areEqual(10, f2(1, undefined, 2)(), "Param scope method properly captures the first parameter");

        function f3(a, b = () => { return a; }) {
            eval("");
            var a = 10;
            assert.areEqual(1, arguments[0], "First argument is properly received");
            assert.areEqual(2, arguments[2], "Third argument is properly received");
            assert.areEqual(3, arguments.length, "Only three arguments are passed in");
            (() => { arguments = [3, 4]; a; })();
            assert.areEqual(3, arguments[0], "Arguments symbol is updated with the new value when the lambda is executed");
            assert.areEqual(4, arguments[1], "New array is properly assigned to arguments symbol");
            assert.areEqual(2, arguments.length, "New array has only elements");

            return b;
        }
        assert.areEqual(1, f3(1, undefined, 2)(), "Param scope method properly captures the first parameter, with eval in the body");

        function f4(a, b = function () { a; } ) {
            var c = 10;
            assert.areEqual(1, arguments[0], "Arguments symbol properly receives the passed in values");
            eval("");
        }
        f4(1);

        function f5(a, b = function () { a; } ) {
            var c = 10;
            assert.areEqual(1, arguments[0], "Arguments symbol properly receives the passed in values");
            arguments = 100;
            assert.areEqual(100, arguments, "Arguments is updated after the assignment");
            eval("");
        }
        f5(1);

        function f6(a, b = function () { a; } ) {
            assert.areEqual(1, arguments[0], "Arguments symbol properly receives the passed in values");
            arguments = 100;
            assert.areEqual(100, arguments, "Arguments is updated after the assignment");
        }
        f6(1);

        function f7(a, b = function () { a; } ) {
            assert.areEqual(5, arguments(), "Function definition is hoisted");
            function arguments() { return 5; }
        }
        f7(1);

        function f8(a, b = function () { a; } ) {
            assert.areEqual(5, arguments(), "Function definition is hoisted");
            function arguments() { return 5; }
            eval("");
        }
        f8(1);

        function f9(a, b = function () { a; } ) {
            assert.areEqual(1, eval("a"), "Eval should be able to access the first argument properly");
            assert.areEqual(1, eval("arguments[0]"), "Eval should be able to access the first argument properly from arguments object");
            assert.areEqual(1, arguments[0], "Arguments symbol properly receives the passed in values");
            arguments = 100;
            assert.areEqual(100, arguments, "Arguments is updated after the assignment");
            assert.areEqual(100, eval("arguments"), "Updated value of arguments is visible in eval");
            assert.areEqual(1, eval("a"), "First argument remains unchanged after the arguments are updated");
        }
        f9(1);

        function f10(a, b = function () { a; } ) {
            assert.areEqual(1, arguments[0], "Arguments symbol properly receives the passed in values");
            var arguments = 100;
            assert.areEqual(100, arguments, "Arguments is updated after the assignment");
        }
        f10(1);

        function f11(a, b = function () { a; } ) {
            assert.areEqual(1, arguments[0], "Arguments symbol properly receives the passed in values");
            var arguments = 100;
            assert.areEqual(100, arguments, "Arguments is updated after the assignment");
            eval("");
        }
        f11(1);

        function f12(a, b = function () { a; } ) {
            assert.areEqual(1, arguments[0], "Arguments symbol properly receives the passed in values");
            b = () => arguments;
            assert.areEqual(1, b()[0], "Lambda captures the right arguments symbol");
            var arguments = 100;
            assert.areEqual(100, arguments, "Arguments is updated after the assignment");
            assert.areEqual(100, b(), "Lambda now gives the updated value");
            eval("");
        }
        f12(1);

        function f13(a, b = () => { return a; }) {
            var a = 10;
            assert.areEqual(1, arguments[0], "First argument is properly received");
            assert.areEqual(2, arguments[2], "Third argument is properly received");
            assert.areEqual(3, arguments.length, "Only three arguments are passed in");
            ((c = arguments = [3, 4]) => { a; })();
            assert.areEqual(3, arguments[0], "Arguments symbol is updated with the new value when the lambda is executed");
            assert.areEqual(4, arguments[1], "New array is properly assigned to arguments symbol");
            assert.areEqual(2, arguments.length, "New array has only elements");

            return b;
        }
        assert.areEqual(1, f13(1, undefined, 2)(), "Param scope method properly captures the first parameter");

        function f14(a, b = () => { return a; }) {
            eval("");
            var a = 10;
            assert.areEqual(1, arguments[0], "First argument is properly received");
            assert.areEqual(2, arguments[2], "Third argument is properly received");
            assert.areEqual(3, arguments.length, "Only three arguments are passed in");
            ((c = arguments = [3, 4]) => { a; })();
            assert.areEqual(3, arguments[0], "Arguments symbol is updated with the new value when the lambda is executed");
            assert.areEqual(4, arguments[1], "New array is properly assigned to arguments symbol");
            assert.areEqual(2, arguments.length, "New array has only elements");

            return b;
        }
        assert.areEqual(1, f14(1, undefined, 2)(), "Param scope method properly captures the first parameter, with eval in the body");

        function f15(a, b = function () { a; }, ...c) {
            assert.areEqual(1, arguments[0], "Checking first argument");
            assert.areEqual(undefined, arguments[1], "Checking second argument");
            assert.areEqual(2, arguments[2], "Checking third argument");
            assert.areEqual(3, arguments[3], "Checking fourth argument");
            assert.areEqual([2, 3], c, "Rest argument should get the trailing parameters properly");
            var arguments = 100;
            assert.areEqual(100, arguments, "Arguments is updated after the assignment");
            assert.areEqual([2, 3], c, "Rest should remain unaffected when arguments is updated");
            eval("");
        }
        f15(1, undefined, 2, 3);

        var f16 = function f17(a, b = function () { a; }, ...c) {
            if (a === 1) {
                assert.areEqual(1, arguments[0], "Checking first argument");
                assert.areEqual(undefined, arguments[1], "Checking second argument");
                assert.areEqual(2, arguments[2], "Checking third argument");
                assert.areEqual(3, arguments[3], "Checking fourth argument");
                assert.areEqual([2, 3], c, "Rest argument should get the trailing parameters properly");
                return f17(undefined, undefined, ...c);
            } else {
                assert.areEqual(undefined, arguments[0], "Checking first argument on the recursive call");
                assert.areEqual(undefined, arguments[1], "Checking second argument on the recursive call");
                assert.areEqual(2, arguments[2], "Checking third argument on the recursive call");
                assert.areEqual(3, arguments[3], "Checking fourth argument on the recursive call");
                assert.areEqual([2, 3], c, "Rest argument should get the trailing parameters properly");
                var arguments = 100;
                assert.areEqual(100, arguments, "Arguments is updated after the assignment");
                assert.areEqual([2, 3], c, "Rest should remain unaffected when arguments is updated");
                return eval("c");
            }
        }
        assert.areEqual([2, 3], f16(1, undefined, 2, 3), "Rest should remain unaffected when arguments is updated");

        function f18(a, b = () => { return a; }) {
            assert.areEqual(1, arguments[0], "First argument is properly received");
            assert.areEqual(2, arguments[2], "Third argument is properly received");
            assert.areEqual(3, arguments.length, "Only three arguments are passed in");
            assert.areEqual(1, ((c = arguments = [3, 4]) => { return a; })(), "Function defined in the body is able to capture a formal from the param scope");
            assert.areEqual(3, arguments[0], "Arguments symbol is updated with the new value when the lambda is executed");
            assert.areEqual(4, arguments[1], "New array is properly assigned to arguments symbol");
            assert.areEqual(2, arguments.length, "New array has only elements");

            return b;
        }
        assert.areEqual(1, f18(1, undefined, 2)(), "Param scope method properly captures the first parameter");

        function f19(a, b = () => { return a; }) {
            eval("");
            a = 10;
            assert.areEqual(1, arguments[0], "First argument is properly received");
            assert.areEqual(2, arguments[2], "Third argument is properly received");
            assert.areEqual(3, arguments.length, "Only three arguments are passed in");
            assert.areEqual(10, ((c = arguments = [3, 4]) => { return a; })(), "Function defined in the body is able to capture a formal from the param scope");
            assert.areEqual(3, arguments[0], "Arguments symbol is updated with the new value when the lambda is executed");
            assert.areEqual(4, arguments[1], "New array is properly assigned to arguments symbol");
            assert.areEqual(2, arguments.length, "New array has only elements");

            return b;
        }
        assert.areEqual(10, f19(1, undefined, 2)(), "Param scope method properly captures the first parameter, with eval in the body, and reflects the value update from the body");

        function f20(a, b = () => { return a; }) {
            eval("");
            var a = 10;
            assert.areEqual(1, arguments[0], "First argument is properly received");
            assert.areEqual(2, arguments[2], "Third argument is properly received");
            assert.areEqual(3, arguments.length, "Only three arguments are passed in");
            assert.areEqual(10, ((c = arguments = [3, 4]) => { return a; })(), "Function defined in the body is able to capture a formal from the param scope");
            assert.areEqual(3, arguments[0], "Arguments symbol is updated with the new value when the lambda is executed");
            assert.areEqual(4, arguments[1], "New array is properly assigned to arguments symbol");
            assert.areEqual(2, arguments.length, "New array has only elements");

            return b;
        }
        assert.areEqual(1, f20(1, undefined, 2)(), "Param scope method properly captures the first parameter, with eval in the body");

        function f21(a, b = function arguments(c) {
            if (!c) {
                return arguments.callee(a, 10, 20);
            }
            return arguments;
        }) {
            assert.areEqual(10, b()[1], "Function defined in the param scope can be called recursively");
            assert.areEqual(1, arguments[0], "Arguments symbol is unaffected by the function expression");
        }
        f21(1);

        function f22(a, b = arguments) {
            var c = function arguments(c) {
                if (!arguments.length) {
                    return arguments.callee(a, 10, 20, 30);
                }
                return arguments;
            }
            assert.areEqual(30, c()[3], "In the function body the arguments function expression with name is not visible");
            assert.areEqual(1, b[0], "In the param scope arguments symbol referes to the passed in values");
        }
        f22(1, undefined, 2, 3, 4);

        function f23(a, b = function arguments(c) {
            if (!c) {
                return arguments.callee(a, 10, 20);
            }
            return eval("arguments");
        }) {
            assert.areEqual(1, b()[0], "Function defined in the param scope can be called recursively when eval occurs in its body");
            assert.areEqual(1, arguments[0], "Arguments symbol is unaffected by the function expression");
        }
        f23(1);

        function f24(a, b = arguments) {
            var c = function arguments(c) {
                if (!arguments.length) {
                    return arguments.callee(a, 10, 20, 30);
                }
                return arguments;
            }
            assert.areEqual(30, c()[3], "In the function body the arguments function expression with name is not visible when eval is there in the body");
            assert.areEqual(3, eval("b[3]"), "In the param scope arguments symbol referes to the passed in values");
        }
        f24(1, undefined, 2, 3, 4);

        function f25(a, b = () => a) {
            assert.areEqual(1, arguments[0], "Function in block causes a var declaration to be hoisted and the initial value should be same as the arguments symbol");
            {
                {
                    function arguments() {
                        return 10;
                    }
                }
            }
            assert.areEqual(1, b(), "Function defined in the param scope should be able to capture the formal even when arguments in overwritten the body");
            assert.areEqual(10, arguments(), "Hoisted var binding is updated after the block is exected");
        }
        f25(1);

        function f26(a, b = () => a) {
            function f27() {
                eval("");
                this.arguments = 1;
            }

            var a = 10;
            var obj = new f27();

            function arguments() {
                return 10;
            }
            assert.areEqual(1, obj.arguments, "Inner function with eval should add the property named arguments when duplicate arguments definition occurs in the parent body");
            assert.areEqual(1, b(), "Formal captured from the param scope should be constrained to the param scope");
        };
        f26(1);

        function f27(a, b = () => a) {
            let arguments = 10;
            assert.areEqual(10, arguments, "Body should use the let declaration");
            assert.areEqual(1, b(), "Function in the param scope should capture the formal");
        }
        f27(1);

        function f28(a, b = () => a) {
            let arguments = 10;
            assert.areEqual(10, eval("arguments"), "Body should use the let declaration when eval is present");
            assert.areEqual(1, b(), "Function in the param scope should capture the formal");
        }
        f28(1);

        function f29(a, b = () => arguments) {
            let arguments = 10;
            assert.areEqual(10, arguments, "Body should use the let declaration");
            assert.areEqual(1, b()[0], "Function in the param scope should capture the arguments symbol from the param scope");
        }
        f29(1);

        function f30(a, b = () => arguments) {
            let arguments = 10;
            assert.areEqual(10, eval("arguments"), "Body should use the let declaration when eval is present");
            assert.areEqual(1, b()[0], "Function in the param scope should capture the arguments symbol from the param scope when eval is present");
        }
        f30(1);
    }
  },
  {
    name: "Split scope and arguments symbol in the param scope",
    body: function () {
        function f1(a = arguments[1], b, c = () => a) {
            assert.areEqual(1, c(), "Arguments symbol is accessible in the param scope");
        }
        f1(undefined, 1);

        function f2(a, b = () => a + arguments[0]) {
            assert.areEqual(2, b(), "An arrow function  in the param scope can capture the Arguments symbol");
        }
        f2(1);

        function f3(a, b = () => () => a + arguments[0]) {
            assert.areEqual(2, b()(), "A nested arrow function  in the param scope can capture the Arguments symbol");
        }
        f3(1);

        function f4() {
            (function (a = arguments[1], b, c = () => a) {
            assert.areEqual(1, c(), "Arguments symbol is accessible in the param scope of a nested function");
            })(undefined, 1);
        }
        f4();

        function f5(a = arguments[1], b, c = () => a) {
            assert.areEqual(1, c(), "Arguments symbol is accessible in the param scope");
            assert.areEqual(1, arguments[1], "Arguments access in the body works fine when arguments is accessed in the param scope also");
        }
        f5(undefined, 1);

        function f6(a = arguments[2], b = eval("arguments")) {
            assert.areEqual(1, a, "Arguments should be accessible in the param scope when eval is present in the param scope")
            assert.areEqual(1, b[2], "Eval in param scope is able to access arguments");
        }
        f6(undefined, undefined, 1);

        function f7(a = arguments[2], b = () => a) {
            assert.areEqual(1, a, "Arguments should be accessible in the param scope when eval is present in the param scope")
            assert.areEqual(1, b(), "Function in the param scope should be able to access the formal properly");
            assert.areEqual(1, eval("arguments[2]"), "Eval in body should be able to access arguments properly");
        }
        f7(undefined, undefined, 1);

        function f8(a, b = function() { return eval("arguments"); }) {
            assert.areEqual(1, b(a)[0], "Eval inside a function in the param scope should be able to access the formal properly");
        }
        f8(1);

        function f9(x, a = arguments, b = arguments = [10, 20], c = () => b) {
            assert.areEqual(1, a[0], "First formal initializer accesses the arguments before assignment");
            assert.areEqual(10, c()[0], "Formal captured correctly reflects the updated arguments value");
            assert.areEqual(10, arguments[0], "Arguments value is updated when the second formal is evaluated");
        }
        f9(1);

        function f10(x, a = arguments = [10, 20], b = () => { assert.areEqual(10, a[0], "Formals is assigned the new array"); return arguments; }) {
            assert.areEqual(10, b()[0], "Arguments captured in the param scope reflects the assignment");
            assert.areEqual(20, b()[1], "Arguments captured in the param scope reflects the assignment");
            assert.areEqual(10, arguments[0], "Arguments value is updated when the formal is evaluated");
            assert.areEqual(20, arguments[1], "Arguments value is updated when the formal is evaluated");
        }
        f10(1);

        function f11(a, b = 10, c = () => {
            assert.areEqual(10, b, "Second formal has the value from initializer");
            assert.areEqual(1, arguments[0], "Arguments is not updated yet");
            arguments = [100, 200, 300, 400, 500];
        }) {
            assert.areEqual(1, arguments[0], "In the body arguments initial value is the same as the one from param scope");
            c();
            assert.areEqual(500, arguments[4], "Arguments value is updated in the param scope and that reflects in the body scope");
        }
        f11(1);

        function f12(a, b = 10, c = () => {
            assert.areEqual(1, eval("arguments[0]"), "Arguments is not updated yet");
            arguments = [100, 200, 300, 400, 500];
        }) {
            assert.areEqual(1, arguments[0], "In the body arguments initial value is the same as the one from param scope");
            c();
            assert.areEqual(500, arguments[4], "With eval in the param scope arguments value is updated in the param scope and that reflects in the body scope");
        }
        f12(1);

        function f13(a, b = 10, c = () => {
            assert.areEqual(10, b, "Second formal has the value from initializer");
            return arguments;
        }) {
            assert.areEqual(1, arguments[0], "In the body arguments initial value is the same as the one from param scope");
            assert.areEqual(1, c()[0], "Param scope captures the arguments symbol properly");
            arguments = 100;
            assert.areEqual(100, c(), "Update to the arguments symbol's value is reflected by the param scope function too");
            assert.areEqual(100, arguments, "Update to the arguments symbol's value is reflected in the body too");
        }
        f13(1);

        function f14(a, b = 10, c = () => {
            assert.areEqual(10, eval("b"), "Second formal has the value from initializer");
            return arguments;
        }) {
            assert.areEqual(1, arguments[0], "In the body arguments initial value is the same as the one from param scope");
            assert.areEqual(1, c()[0], "Param scope captures the arguments symbol properly");
            arguments = 100;
            assert.areEqual(100, c(), "With eval in the param scope update to the arguments symbol's value is reflected by the param scope function too");
            assert.areEqual(100, arguments, "With eval in the param scope update to the arguments symbol's value is reflected in the body too");
        }
        f14(1);

        function f15(a, b = 10, c = () => {
            assert.areEqual(10, b, "Second formal has the value from initializer");
            return arguments;
        }) {
            assert.areEqual(1, arguments[0], "In the body arguments initial value is the same as the one from param scope");
            assert.areEqual(1, c()[0], "Param scope captures the arguments symbol properly");
            arguments = 100;
            assert.areEqual(100, eval("c()"), "With eval in the body scope update to the arguments symbol's value is reflected by the param scope function too");
            assert.areEqual(100, arguments, "With eval in the body scope update to the arguments symbol's value is reflected in the body too");
        }
        f15(1);

        function f16(a, b = 10, c = () => {
            assert.areEqual(10, b, "Second formal has the value from initializer");
            return arguments;
        }) {
            d = () => arguments;
            assert.areEqual(1, d()[0], "In the body arguments initial value is the same as the one from param scope");
            assert.areEqual(1, c()[0], "Param scope captures the arguments symbol properly");
            arguments = 100;
            assert.areEqual(100, c(), "Update to the arguments symbol's value is reflected by the param scope function too");
            assert.areEqual(100, d(), "Update to the arguments symbol's value is reflected in functions defined in the body too");
        }
        f16(1);

        function f17(a, b = () => a, c = (x = arguments = [10, 20]) => x) {
            d = () => arguments;
            assert.areEqual(1, d()[0], "Arguments initial value is properly reflected in the body");
            assert.areEqual(1, b(), "Formal captured by lambda should have the right value");
            assert.areEqual(10, c()[0], "Lambda captures the first argument of the lambda");
            assert.areEqual(20, d()[1], "Arguments value is updated when the lambda from param scope is executed");
            assert.areEqual(1, b(), "Value of the formal is not affected");
        }
        f17(1);

        function f18(a, b = () => a, c = (x = 10, y = () => x + (arguments = [10, 20])[1]) => y) {
            d = (x = arguments, y = () => x) => y;
            assert.areEqual(1, d()()[0], "Arguments initial value is properly reflected in the body");
            assert.areEqual(1, b(), "Formal captured by lambda should have the right value");
            assert.areEqual(30, c()(), "Split scoped lambda captures and updates the arguments symbol");
            assert.areEqual(20, d()()[1], "Arguments value is updated when the lambda from param scope is executed");
            assert.areEqual(1, b(), "Value of the formal is not affected");
        }
        f18(1);

        function f19({a = arguments[1]}, b, c = () => a) {
            assert.areEqual(1, c(), "Arguments symbol is accessible in a destructured pattern in the param scope");
        }
        f19({}, 1);

        function f20({a = arguments[1], b = () => a + arguments[2]}, c, ...d) {
            assert.areEqual(3, b(), "Arguments symbol is accessible in a destructured pattern in the param scope");
            assert.areEqual("2,3,4", d.toString(), "Rest parameter gets the right value when arguments is captured");
        }
        f20({}, 1, 2, 3, 4);

        function f21(a, b = () => arguments) {
            function arguments() {
                return 100;
            }
            assert.areEqual(10, b()[0], "Param scope function captures the arguments symbol");
            assert.areEqual(100, arguments(), "In the body arguments in the newly defined function");
        }
        f21(10);
    }
  },
  {
      name: "Splits scope with formals named arguments",
      body: function () {
        function f1(a = 10, arguments = () => a) {
            assert.areEqual(10, arguments(), "Arguments is a function defined in the param scope");
        }
        f1(undefined, undefined, 1, 2, 3);

        function f2(arguments = 10, a = () => arguments) {
            assert.areEqual(10, a(), "Function defined in the param scope captures the formal named arguments");
        }
        f2(undefined, undefined, 1, 2, 3);

        function f3(arguments = 10, a = () => arguments) {
            assert.areEqual(10, eval("a()"), "Function defined in the param scope captures the formal named arguments when eval is present in the body");
        }
        f3(undefined, undefined, 1, 2, 3);

        function f4(arguments = 10, a = () => eval("arguments")) {
            assert.areEqual(10, a(), "Function defined in the param scope captures the formal named arguments when eval is present in its body");
        }
        f4(undefined, undefined, 1, 2, 3);

        function f5(arguments = 10, a = () => arguments) {
            function arguments() {
                return 100;
            }
            assert.areEqual(10, a(), "Param scope function properly captures the formal");
            assert.areEqual(100, arguments(), "In the body arguments in the newly defined function");
        }
        f5();

        function f6(arguments = 10, a = arguments, b = () => arguments + eval("a")) {
            assert.areEqual(100, arguments(), "In the body arguments in the newly defined function");
            function arguments() {
                return 100;
            }
            assert.areEqual(10, a, "Second formals accesses the arguments symbol from param scope");
            assert.areEqual(20, b(), "Function defined in the param scope can access arguments and second formal");
        }
        f6();

        function f7(arguments = 10, a = () => arguments) {
            function arguments() {
                return 100;
            }
            assert.areEqual(10, a(), "Param scope function properly captures the formal");
            assert.areEqual(100, eval("arguments()"), "In the body arguments in the newly defined function");
        }
        f7();

        function f8(arguments = 10, a = () => arguments) {
            assert.areEqual(10, arguments, "In the body arguments has the initial value from the param scope");
            var arguments= 100;
            assert.areEqual(10, a(), "Param scope function properly captures the formal");
            assert.areEqual(100, arguments, "In the body arguments in the newly defined var");
        }
        f8();

        function f9(arguments = [10, 20], a = () => arguments, b = arguments[1], c = () => a) {
            function arguments() {
                return 100;
            }
            assert.areEqual(100, arguments(), "In the body arguments in the newly defined function");
            assert.areEqual(20, b, "Other formals can access the arguments formal");
            assert.areEqual("10,20", c()().toString(), "Formal capturing works fine with arguments defined as a formal");
        }
        f9();

        function f10({arguments = [10, 20], a = () => arguments}, b = () => a) {
            function arguments() {
                return 100;
            }
            assert.areEqual(100, arguments(), "In the body arguments in the newly defined function");
            assert.areEqual("10,20", b()().toString(), "Formal capturing works fine with arguments defined as a formal");
        }
        f10({}, undefined, 1, );
      }
  },
  {
    name: "Split scope and super call",
    body: function () {
        class c1 {
            constructor() {
                return { x : 1 };
            }
        };

        class c2 extends c1 {
            constructor(a = 1, b = () => { assert.areEqual(1, super().x, "Super is accessible in the param scope"); return a; }) {
                var c = 10;
                var a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function defined in the param scope should capture the formal");
                return {};
            }
        }
        new c2();

        class c3 extends c1 {
            constructor(a = 1, b = () => { return a; }) {
                (() => assert.areEqual(1, super().x, "Lambda should be able to access the super method properly in the body"))();
                var a = 10;
                assert.areEqual(1, b(), "Function defined in the param scope should capture the formal");
            }
        }
        new c3();

        class c4 extends c1 {
            constructor(a = 1, b = () => { return a; }) {
                var c = 10;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function defined in the param scope should capture the formal");
                assert.areEqual(1, eval("super().x"), "Eval should be able to access the super property properly");
            }
        }
        new c4();

        class c5 extends c1 {
            constructor(a = super().x, b = () => { return a; }) {
                assert.areEqual(1, a, "First formal calls the super from the param scope");
                var c = 10;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function defined in the param scope should capture the formal");
            }
        }
        new c5();

        class c6 extends c1 {
            constructor(a = 1, b = () => { assert.areEqual(1, super().x, "Super is accessible in the param scope"); return a; }) {
                var c = 10;
                a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(20, b(), "Assignment in the body updates the value of the formal");
                return {};
            }
        }
        new c6();

        class c7 extends c1 {
            constructor(a = () => super(), b = () => a) {
                assert.areEqual(1, a().x, "Super is captured in a lambda in param scope and executed in the body scope to make sure the same super slot is used in both body and param scope");
            }
        }
        new c7();
    }
  },
  {
    name: "Split scope and super property",
    body: function () {
        class c1 {
            foo () {
                return 1;
            }
        };

        class c2 extends c1 {
            foo(a = 1, b = () => { assert.areEqual(1, super.foo(), "Super property access works fine from a lambda defined in the param scope"); return a; }) {
                var a = 20;
                var c = 10;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function defined in the param scope should capture the formal");
            }
        }
        (new c2()).foo();

        class c3 extends c1 {
            foo(a = 1, b = () => { return a; }) {
                var c = 10;
                var a = 20;
                (() => assert.areEqual(1, super.foo(), "Super property access works fine from a lambda defined in the body scope"))();
                assert.areEqual(1, b(), "Function defined in the param scope should capture the formal");
            }
        }
        (new c3()).foo();

        class c4 extends c1 {
            foo(a = 1, b = () => { return a; }) {
                var c = 10;
                var a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function defined in the param scope should capture the formal");
                assert.areEqual(1, eval("super.foo()"), "Eval should be able to access the super property properly from the body scope");
            }
        }
        (new c4()).foo();

        class c5 extends c1 {
            foo(a = super.foo(), b = () => { return a; }) {
                assert.areEqual(1, a, "First formal uses the super property from the param scope");
                var c = 10;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                var a = 20;
                assert.areEqual(1, b(), "Function defined in the param scope should capture the formal");
            }
        }
        (new c5()).foo();

        class c6 extends c1 {
            foo(a = 1, b = () => { assert.areEqual(1, super.foo(), "Super property access works fine from a lambda defined in the param scope"); return a; }) {
                a = 20;
                var c = 10;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(20, b(), "Assignment in the body updates the value of the formal");
            }
        }
        (new c6()).foo();
    }
  },
  {
    name: "Split scope and new.target",
    body: function () {
        class c1 {
            constructor(newTarget) {
                assert.isTrue(newTarget == new.target, "Base class should receive the right value for new.target");
            }
        };

        class c2 extends c1 {
            constructor(a = 1, b = () => { assert.isTrue(new.target == c2, "new.target should have the derived class value in the param scope"); return a; }) {
                super(c2);
                var c = 10;
                var a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function defined in the param scope should capture the formal");
            }
        }
        new c2();

        class c3 extends c1 {
            constructor(a = 1, b = () => { return a; }) {
                super(c3);
                var c = 10;
                (() => assert.isTrue(new.target == c3, "new.target should be the derived class in the body scope when captured by lambda"))();
                assert.isTrue(new.target == c3, "new.target should be the derived class in the body scope");
            }
        }
        new c3();

        class c4 extends c1 {
            constructor(a = 1, b = () => { return a; }) {
                super(c4);
                assert.isTrue(eval("new.target == c4"), "new.target should be the derived class inside eval");
                assert.isTrue(new.target == c4, "new.target should be the derived class in the body scope");
            }
        }
        new c4();

        class c5 extends c1 {
            constructor(a = new.target, b = () => { return a; }) {
                super(c5);
                assert.isTrue(a == c5, "new.target accessed from the param scope should work fine");
            }
        }
        new c5();

        class c6 extends c1 {
            constructor(a = 1, b = () => { assert.isTrue(new.target == c6, "new.target should have the derived class value in the param scope"); return a; }) {
                super(c6);
                var c = 10;
                a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(20, b(), "Assignment in the body updates the value of the captured formal");
            }
        }
        new c6();
    }
  },
  {
    name: "Split parameter scope and eval",
    body: function () {
        function g() {
            return 3 * 3;
        }

        function f1(h = () => eval("g()")) {
            assert.areEqual(6, g(), "Right method is called in the body scope");
            function g() {
                return 2 * 3;
            }
            return h();
        }
        assert.areEqual(9, f1(), "Paramater scope remains split");

        function f2(h = () => eval("g()")) {
            assert.areEqual(6, eval("g()"), "Right method is called in the body scope");
            function g() {
                return 2 * 3;
            }
            return h();
        }
        assert.areEqual(9, f2(), "Paramater scope remains split");
    }
  },
  {
    name: "Split parameter scope with eval in body",
    body: function () {
        function f1(a = 10, b = function () { return a; }) {
            assert.areEqual(10, a, "Initial value of parameter in the body scope should be the same as the one in param scope");
            assert.areEqual(10, eval('a'), "Initial value of parameter in the body scope in eval should be the same as the one in param scope");
            var a = 20;
            assert.areEqual(20, a, "New assignment in the body scope updates the variable's value in body scope");
            assert.areEqual(20, eval('a'), "New assignment in the body scope updates the variable's value when evaluated through eval in body scope");
            return b;
        }
        assert.areEqual(10, f1()(), "Function defined in the param scope captures the formals from the param scope not body scope with eval");

        function f2(a = 10, b = function () { return a; }) {
            assert.areEqual(10, eval('b()'), "Eval of the function from param scope should return the right value for the formal");
            var a = 20;
            assert.areEqual(10, eval('b()'), "Eval of the function from param scope should return the right value for the formal even after assignment to the corresponding body symbol");
            return b;
        }
        assert.areEqual(10, f2()(), "Function defined in the param scope captures the formals from the param scope not body scope with eval");

        function f3(a = 10, b = function () { return a; }) {
            assert.areEqual(100, eval('b()'), "Eval of the function from body scope should return the right value for the formal");
            var a = 20;
            function b () { return a * a; }
            assert.areEqual(400, eval('b()'), "Eval of the function from body scope should return the right value after assignment to the corresponding body symbol");
            return b;
        }
        assert.areEqual(400, f3()(), "Function defined in the body scope captures the symbol from the body scope with eval");

        function f4 (a, b, c = function () { b; }, d = 1) {
            var e = 10;
            assert.areEqual(2, arguments[0], "Unmapped arguments value has the expected value in the body");
            (function () {
                eval('');
            }());
        };
        f4.call(1, 2);

        function f5(a = 10, b = 20, c = () => [a, b]) {
            assert.areEqual(10, c()[0], "Before the assignment in the body the value of the symbol is the body is same as the formal formal with eval in the body");
            assert.areEqual(20, eval("c()[1]"), "With eval in the body, the value of the formal is retained before the assignment");
            var a = 20;
            b = 200;
            assert.areEqual(20, a, "New assignment in the body scope updates the variable's value in body scope with eval in the body");
            return c;
        }
        assert.areEqual(10, f5()()[0], "Assignment in the body does not affect the value of the formal with eval in the body");
        assert.areEqual(200, f5()()[1], "Assignment in the body updates the value of the formal with eval in the body");

        function f6(a = 10, b = function () { return a; }) {
            assert.areEqual(10, a, "Initial value of parameter in the body scope should be the same as the one in param scope");
            assert.areEqual(10, eval('a'), "Initial value of parameter in the body scope in eval should be the same as the one in param scope");
            a = 20;
            assert.areEqual(20, a, "New assignment in the body scope updates the variable's value in body scope");
            assert.areEqual(20, eval('a'), "New assignment in the body scope updates the variable's value when evaluated through eval in body scope");
            return b;
        }
        assert.areEqual(20, f6()(), "Function defined in the param scope captures the formals from the param scope which is updated in the body");

        function f7(a, b = ()=> a) {
            eval("var a = 100");
            assert.areEqual(100, a, "Body gets the variable declaration leaked from eval");
            assert.areEqual(10, b(), "Param scope function still points to the formal symbol");
        }
        f7(10);

        var x = 1;
        var f8 = (a = 10, b = () => a) => {
            eval("var a = 100");
            return function f9()
                {
                    function abc() { return 1000; }
                    function c(foo) {
                        return abc();
                    }
                    return c() + x + b() + a;
                };
        }
        assert.areEqual(1111, f8()(), "The new definition of variable in the body, when there are no scope slots intiially allocated for the body, should be captured by the inner function");
    }
  },
  {
    name: "Split scope and with",
    body: function () {
          function f1(a, b, c = function () { a; }) {
            with ({}) {
                var d = function () {
                    return 10;
                };
                assert.areEqual(10, d(), "With inside a split scope function should work fine");
            }
          }
          f1();

          function f2(a, b, c = function () { a; }) {
            var d = function () {
                return 10;
            };
            with ({}) {
                assert.areEqual(10, d(), "With inside a split scope function should be able to access the function definition from the body");
            }
          }
          f2();

          function f3(a, b = function () { return 10; }, c = function () { a; }) {
            with ({}) {
                assert.areEqual(10, b(), "With inside a split scope function should be able to access the function definition from the param scope");
            }
          }
          f3();

          function f4(a, b = function () { return 10; }, c = function () { a; }) {
            var d = {
                e : function () { return 10; }
            };
            e = function () { return 100; };
            with (d) {
                assert.areEqual(10, e(), "With should use the function definition inside the object not the one from body");
            }
          }
          f4();

          function f5(a, b = { d : function () { return 10; } }, c = function () { a; }) {
            var d = { };
            with (b) {
                assert.areEqual(10, d(), "With should use the function definition inside the object from the param scope not the one from body");
            }
          }
          f5();

          var v6 = 100
          function f6(a, b, c = function () { a; }, e = function () { with({}) { assert.areEqual(100, v6, "With inside param scope should be able to access var from outside"); } }, f = e()) {
            var v6 = { };
          }
          f6();

          function f7(a, b, c = function () { a; }) {
            with ({}) {
                assert.areEqual(100, v6, "With inside body scope should be able to access var from outside");
            }
          }
          f7();

          function f8() {
            function f9() {
                return 1;
            }
            var v1 = 10;
            function f10(a = 10, b = function f11() {
                a;
                assert.areEqual(10, v1, "Function in the param scope should be able to access the outside variable");
                with ({}) {
                    assert.areEqual(1, f9(), "With construct inside a param scoped function should be able to execute functions from outside");
                }
            }) {
                b();
            };
            f10();
          }
          f8();
          f8();

          function f12() {
            function f13() {
                return 1;
            }
            var v2 = 100;
            function f14(a = 10, b = function () {
                assert.areEqual(10, a, "Function in the param scope should be able to access the formal from parent");
                return function () {
                    assert.areEqual(10, a, "Function nested in the param scope should be able to access the formal from the split scoped function");
                    assert.areEqual(100, v2, "Function in the param scope should be able to access the outside variable");
                    with ({}) {
                        assert.areEqual(1, f13(), "With construct inside a param scoped function should be able to execute functions from outside");
                    }
                };
            }) {
                b()();
            };
            f14();
          }
          f12();
          f12();

          function f13() {
            var protoObj1 = { x : 1 };
            var f14 = function () {
                return 100;
            };
            var f15 = function (a = 10, b = function () {
                    with (protoObj1) {
                        assert.areEqual(1, x, "With inside a param scope function should be able to retrieve the object properties");
                    }
                    assert.areEqual(10, a, "Param scope function containing with construct should be able to access the first parameter");
                }) {
                function func5() {
                    return f14;
                }
                assert.areEqual(100, func5()(), "Function in the body scope which has 0 scope slots accesses an outside function");
                b();
            };
            f15();
          }
          f13();

    }
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

        function f1(a = 10, b = () => eval("a")) {
            assert.areEqual(10, eval("a"), "In the body initial value of the symbol should be same as the final value from param scope");
            var a = 20;
            assert.areEqual(20, eval("a"), "In the body after assignment the symbol value is updated");
            assert.areEqual(10, b(), "Eval in the param scope captures the symbol from the param scope");
        }
        f1();

        function f2(a = 10, b = () => eval("a")) {
            var a = 20;
            assert.areEqual(10, b(), "Eval in the param scope captures the symbol from the param scope even when there is no eval in the body");
        }
        f2();

        function f3(a = 10, b = function () { return eval("a"); }) {
            var a = 20;
            assert.areEqual(10, b(), "Eval in the child function of a param scope captures the symbol from the param scope even when there is no eval in the body");
        }
        f3();

        function f4(a = 10, b = () => eval("a"), c = a = 30) {
            assert.areEqual(30, eval("a"), "In the body initial value of the symbol should be same as the final value from param scope");
            var a = 20;
            assert.areEqual(20, eval("a"), "In the body after assignment the symbol value is updated");
            assert.areEqual(30, b(), "Eval in the param scope captures the symbol from the param scope");
        }
        f4();

        function f5(a = 10, b = () => eval("a")) {
            assert.areEqual(30, eval("a"), "In the body initial value of the symbol should be same as the final value from param scope");
            var a = 20;
            assert.areEqual(20, eval("a"), "In the body after assignment the symbol value is updated");
            assert.areEqual(30, b(), "Eval in the param scope captures the symbol from the param scope");
        }
        f5(30);

        var f6 = function f7(a = 10, b = () => eval("a")) {
            var a = 20;
            assert.areEqual(10, b(), "Eval in the param scope captures the symbol from the param scope for a function expression with name");
        }
        f6();

        var f8 = function f9(a = 10, b = () => eval("a"), c = b) {
            var a = 20;
            function b() {
                return a;
            }
            assert.areEqual(20, eval("b()"), "Eval in the body uses the function definition from body");
            assert.areEqual(10, c(), "Eval in the param scope captures the symbol from the param scope for a function expression with name even with eval in the body");
        }
        f8();

        function f9(a = 10, b = () => () => eval("a")) {
            var a = 20;
            assert.areEqual(10, b()(), "Eval in a nested function in the param scope captures the symbol from the param scope even when there is no eval in the body");
        }
        f9();

        function f10(a = 10, b = () => () => eval("a")) {
            var a = 20;
            assert.areEqual(10, eval("b()()"), "Eval in a nested function in the param scope captures the symbol from the param scope even when there is no eval in the body");
        }
        f10();

        var obj = {
            mf1(a = 10, b = () => () => eval("a")) {
                var a = 20;
                assert.areEqual(10, eval("b()()"), "Eval in a nested function in the param scope of a method definition captures the symbol from the param scope");
            }
        }
        obj.mf1();

        var arr = [2, 3, 4];
        function f11(a = 10, b = function () { return eval("a"); }, ...c) {
            assert.areEqual(arr.length, c.length, "Rest parameter should contain the same number of elements as the spread arg");
            for (i = 0; i < arr.length; i++) {
                assert.areEqual(arr[i], c[i], "Elements in the rest and the spread should be in the same order");
            }
            return b;
        }
        assert.areEqual(f11(undefined, undefined, ...arr)(), 10, "Presence of rest parameter shouldn't affect the binding");

        function f12(a = 10, b = eval("a"), ...c) {
            assert.areEqual(arr.length, c.length, "Rest parameter should contain the same number of elements as the spread arg");
            for (i = 0; i < arr.length; i++) {
                assert.areEqual(arr[i], c[i], "Elements in the rest and the spread should be in the same order");
            }
            return b;
        }
        assert.areEqual(f12(undefined, undefined, ...arr), 10, "Presence of rest parameter shouldn't affect the binding");

        function f13(x = 1) {
            (function () {
                function f14 (a = () => eval("x"), b = a) {
                    var a = 20;
                    assert.areEqual(1, b(), "Eval in param scope should be able to access formals from an outer function");
                };
                f14();
            })()
        }
        f13();

        function f15(a = eval) {
            assert.areEqual(1, a("v15"), "Indirect eval should be able to access the global variables properly");
        }
        f15();

        function f16(a = 10, b = () => eval("a")) {
            assert.areEqual(10, eval("a"), "In the body initial value of the symbol should be same as the final value from param scope");
            a = 20;
            assert.areEqual(20, eval("a"), "In the body after assignment the symbol value is updated");
            assert.areEqual(20, b(), "Eval in the param scope captures the symbol from the param scope which is updated in the body");
        }
        f16();

        function f17(a = 10, b = () => eval("a")) {
            a = 20;
            assert.areEqual(20, b(), "Eval in the param scope captures the symbol from the param scope even when there is no eval in the body");
        }
        f17();

        function f18(a = 10, b = function () { return eval("a"); }) {
            a = 20;
            assert.areEqual(20, b(), "Eval in the param scope captures the symbol from the param scope even when there is no eval in the body");
        }
        f18();

        var x = 10;
        var f19 = (d = eval("10")) => {
            function f20()
            {
                function abc() { return 1; }
                function c() {
                    return abc();
                }
                return c() + x + d;
            };
            return f20();
        }
        assert.areEqual(21, f19(), "Method inside a split scoped function is able to access the outside variable");

        var x = 1;
        var f20 = (d = eval("10")) => {
            return function f21()
                {
                    function abc() { return 100; }
                    function c(foo) {
                        return abc();
                    }
                    return c() + x + d;
                };
        }
        assert.areEqual(111, f20()(), "When there are no scope slots allocated in the body we should still be creating the inner closure for the body.");
    }
  },
  {
      name: "Split scoped functions inside eval",
      body: function () {
            var c = 10;
            var result = eval(`(function (a = 1, b = () => a + c) {
                var a = 2 + c;
                return [a, b()];
            })()`);
            assert.areEqual([12, 11].toString(), result.toString(), "Split scope function defined inside an eval should work fine");

            c = 10;
            result = eval(`(function (a = 1, b = () => a + c) {
                a = 2 + c;
                return [a, b()];
            })()`);
            assert.areEqual([12, 22].toString(), result.toString(), "Split scope function defined inside an eval captures the formal whose value is updated in the body");

            c = 10;
            result = eval(`(function (a = 1, b = () => eval('a + c')) {
                var a = 2 + c;
                return [a, b()];
            })()`);
            assert.areEqual([12, 11].toString(), result.toString(), "Split scope function with eval defined inside an eval should work fine");

            c = 10;
            result = (function (b = eval(`((a = 1, b = () => a + c) => {
                a = 2 + c;
                return [a, b()];
            })()`)) {
                a = c;
                result = eval(`((a1 = 1, b1 = () => a1 + a + c) => {
                    a = 2 + a1 + c;
                    return [a, b1()];
                })()`);
                return [a, result, b];
            })();
            assert.areEqual([13, 13, 24, 12, 22].toString(), result.toString(), "Split scope functions defined in both body and the param scope should work fine with eval");
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
        function test(x = eval("var a1 = 1; let b1 = 2; const c1 = 3;")) {
            // none should be visible
            assert.throws(function () { a1 }, ReferenceError, "Ignoring the default value does not result in an eval declaration leaking", "'a1' is not defined");
            assert.throws(function () { b1 }, ReferenceError, "Let declarations do not leak out of eval to parameter scope",   "'b1' is not defined");
            assert.throws(function () { c1 }, ReferenceError, "Const declarations do not leak out of eval to parameter scope when x is ", "'c1' is not defined");
        }
        test();

        // Redefining locals
        function foo(a = eval("var x = 1; assert.areEqual(1, x, 'Variable declared inside eval is accessible within eval');")) {
            assert.areEqual(undefined, x, "Var declaration from eval is not visible in the body");
            var x = 10;
            assert.areEqual(10, x, "Var declaration from eval uses its new value in the body declaration");
        }
        assert.doesNotThrow(function() { foo(); }, "Redefining a local var with an eval var does not throw");

        // Function bodies defined in eval
        function funcArrow(a = eval("() => 1"), b = a) { function a() { return 10; }; return [a(), b()]; }
        assert.areEqual([10,1], funcArrow(), "Defining an arrow function body inside an eval works at default parameter scope");

        function f1(a = eval("(function f11() { return 1; })"), b = a()) { return [a(), b]; }
        assert.areEqual([1, 1], f1(), "Defining a function inside an eval works at default parameter scope");

        function f2(a = eval("function f21() { return 1; }; f21"), b = a()) { return [a(), b]; }
        assert.areEqual([1, 1], f2(), "Defining a function inside an eval works at default parameter scope");

        function f3(a = eval("(f31 = function () { return 1; }, f31())")) { return a; }
        assert.areEqual(1, f3(), "Defining a function inside an eval works at default parameter scope");

        function f4(a = eval("(function *f41() { yield 1; return 2; })"), b = a(), c = b.next()) { return [c, b.next()]; }
        assert.areEqual([{value : 1, done : false}, {value : 2, done : true}], f4(), "Declaring a generator function inside an eval works at default parameter scope");

        function f5(a = eval("f = function f51() { return 1; }"), b = f()) { return [a(), b, f()]; }
        assert.areEqual([1, 1, 1], f5(), "Declaring a function inside an eval works at default parameter scope");

        assert.throws(function () { eval("function f6(a = eval('b'), b) {}; f6();"); }, ReferenceError, "Future default references using eval are not allowed", "Use before declaration");
    }
  },
  {
    name: "Eval and with",
    body: function () {
        function f1(a = { x : 10 }, b = () => eval("a")) {
            var a = 20
            with (b()) {
                return b().x + a;
            }
        }
        assert.areEqual(30, f1(), "With in the body of a eval split scoped function should not affect the usage of eval in the param scope");

        function f2(a = { x : 10 }, b = function () { return eval("a.x"); }) {
            var a = 20;
            with ({}) {
                return b() + a;
            }
        }
        assert.areEqual(30, f2(), "With in the param scope of a eval split scoped function should not affect the parameter capture");

        function f3(a = { x : 10 }, b = function () { return eval("a.x"); }, c = b) {
            var a = 20;
            with ({}) {
                return c() + b();
            }
            function b() {
                return a;
            }
        }
        assert.areEqual(30, f3(), "With in the param scope of a eval split scoped function should work fine even with shadowing in body");

        function f4(a = { x : 10 }, b = () => eval("a")) {
             a = 20
            with (b()) {
                return b() + a;
            }
        }
        assert.areEqual(40, f4(), "With eval in the param scope capturing should reflect the update to the formal's value from the body");
    }
  },
  {
    name: "Eval and this",
    body: function () {
        function f1(a, b = () => eval("this.x")) {
            return b();
        }
        assert.areEqual(10, f1.call({x : 10}), "Eval in the param scope should be able to access this");

        function f2(a, b = () => () => eval("this.x()")) {
            return b();
        }
        assert.areEqual(10, f2.call({x () { return 10; }})(), "Eval in a nested function in param should be able to access this");

        function f3(b = () => eval("this"), c = () => b) {
            function b() {
                return 20;
            }
            return b() + c()().x();
        }
        assert.areEqual(30, f3.call({x () { return 10; }}), "Eval in param scope can access this with function definition in body");

        function f4(a, b = () => eval("this")) {
            return eval("b().x() + a");
        }
        assert.areEqual(30, f4.call({x () { return 10; }}, 20), "Eval in param scope can access this when eval is present in the body");
    }
  },
  {
    name: "Class definitions in eval",
    body: function () {
        function f1(a = eval("(class c1 { mf1 () { return b; } })"), b = 10) {
            var b = new a();
            assert.areEqual(10, b.mf1(), "Class defined in an eval in a param scope should work fine");
        }
        f1();

        var f2 = function f3(a = eval("(class c1 { mf1 () { return b; } })"), b = 10) {
            var b = new a();
            assert.areEqual(10, b.mf1(), "Class defined in an eval in a param scope of a function expression with name should work fine");
        }
        f2();

        function f4(a = eval("(class c1 { mf1 () { return b; } })"), b = 10) {
            assert.areEqual(10, eval("(new a()).mf1()"), "Class defined in an eval in a param scope should work fine with eval in the body");
        }
        f4();
    }
  },
  {
    name: "Eval and new.target",
    body: function () {
        class c1 {
            constructor(newTarget) {
                assert.isTrue(newTarget == new.target, "Base class should receive the right value for new.target");
            }
        };

        class c2 extends c1 {
            constructor(a = 1, b = () => { assert.isTrue(eval("new.target") == c2, "new.target should have the derived class value in the param scope"); return a; }) {
                super(c2);
                var c = 10;
                var a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function with eval defined in the param scope should be abel to access new.target properly");
            }
        }
        new c2();

        class c3 extends c1 {
            constructor(a = 1, b = () => { assert.isTrue(eval("new.target") == c3, "new.target should have the derived class value in the param scope"); return a; }) {
                super(c3);
                var c = 10;
                var a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, eval("b()"), "Function defined in the param scope should capture the formal");
            }
        }
        new c3();

        class c4 extends c1 {
            constructor(a = new.target, b = () => eval("a")) {
                super(c4);
                assert.isTrue(b() == c4, "new.target accessed from the param scope should work fine");
            }
        }
        new c4();

        class c5 extends c1 {
            constructor(a, b = eval("a = new.target"), c = () => a) {
                super(c5);
                assert.isTrue(c() == c5, "new.target accessed from the param scope should work fine");
            }
        }
        new c5();

        class c6 extends c1 {
            constructor(a = 1, b = () => { assert.isTrue(eval("new.target") == c6, "new.target should have the derived class value in the param scope"); return a; }) {
                super(c6);
                var c = 10;
                a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(20, b(), "The value of the formal is updated by the assignment in the body");
            }
        }
        new c6();
    }
  },
  {
    name: "Eval and super call",
    body: function () {
        class c1 {
            constructor() {
                return { x : 1 };
            }
        };

        class c2 extends c1 {
            constructor(a = 1, b = () => { assert.areEqual(1, eval("super().x"), "Super is accessible in eval in the param scope"); return a; }) {
                var c = 10;
                var a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables with eval in the param scope"))();
                assert.areEqual(1, b(), "Function with eval defined in the param scope should capture the formal");
                return {};
            }
        }
        new c2();

        class c3 extends c1 {
            constructor(a = 1, b = eval("a")) {
                assert.areEqual(1, b, "Eval should be able to capture the formal even when there is no eval in the body");
                assert.areEqual(1, super().x, "Super call should work fine in the body with eval in the param scope");
            }
        }
        new c3();

        class c4 extends c1 {
            constructor(a = 1, b = () => eval("a")) {
                var c = 10;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function with eval defined in the param scope should capture the formal the right way");
                assert.areEqual(1, eval("super().x"), "Eval should be able to access the super property properly in the body");
            }
        }
        new c4();

        class c5 extends c1 {
            constructor(a = super().x, b = eval("a")) {
                assert.areEqual(1, a, "First formal calls the super from the param scope should work fine with eval in the param scope");
                assert.areEqual(1, b, "Eval should work fine with super in the param");
            }
        }
        new c5();

        class c6 extends c1 {
            constructor(a = 1, b = () => { assert.areEqual(1, eval("super().x"), "Super is accessible in eval in the param scope"); return a; }) {
                var c = 10;
                a = 20;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables with eval in the param scope"))();
                assert.areEqual(20, b(), "Function with eval defined in the param scope should capture the formal which is updated in the body");
                return {};
            }
        }
        new c6();

        class c7 extends c1 {
            constructor(a = 1, b = eval("a")) {
                assert.areEqual(1, a, "Default value is assigned to the first formal");
                assert.areEqual(1, b, "Eval of a formal works fine in param scope when body has super call");
                assert.areEqual(1, super().x, "Super call is executed in the body when param scope has eval");
            }
        }
        new c7();

        class c8 extends c1 {
            constructor(a = super().x, b = eval("a")) {
                assert.areEqual(1, b, "First formal is assigned the value returned from super constructor");
            }
        }
        assert.areEqual(1, (new c8()).x, "Super call from the formal with eval in the param returns the correct value");

        class c9 extends c1 {
            constructor(a = eval("super().x")) {
                assert.areEqual(1, a, "First formal is assigned the value returned from super constructor through eval");
            }
        }
        assert.areEqual(1, (new c9()).x, "Super call from the formal with eval in the param returns the correct value");
    }
  },
  {
    name: "Eval and super property",
    body: function () {
        class c1 {
            foo () {
                return 1;
            }
        };

        class c2 extends c1 {
            foo(a = 1, b = () => { assert.areEqual(1, eval(super.foo()), "Super property access works fine inside eval in a lambda defined in the param scope"); return a; }) {
                var a = 20;
                var c = 10;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function with eval defined in the param scope should capture the formal");
            }
        }
        (new c2()).foo();

        class c3 extends c1 {
            foo(a = 1, b = eval("a")) {
                assert.areEqual(1, b, "Eval should be able to capture the formal even when there is no eval in the body");
                assert.areEqual(1, super.foo(), "Super property access should work fine in the body with eval in the param scope");
            }
        }
        (new c3()).foo();

        class c4 extends c1 {
            foo(a = 1, b = () => eval("a")) {
                var c = 10;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(1, b(), "Function with eval defined in the param scope should capture the formal the right way");
                assert.areEqual(1, eval("super.foo()"), "Eval should be able to access the super property properly in the body with eval");
            }
        }
        (new c4()).foo();

        class c5 extends c1 {
            foo(a = super.foo(), b = eval("a")) {
                assert.areEqual(1, a, "First formal's super property access from the param scope should work fine with eval in the param scope");
                assert.areEqual(1, b, "Eval should work fine with super in the param");
            }
        }
        (new c5()).foo();

        class c6 extends c1 {
            foo(a = 1, b = () => { assert.areEqual(1, eval(super.foo()), "Super property access works fine inside eval in a lambda defined in the param scope"); return a; }) {
                a = 20;
                var c = 10;
                (() => assert.areEqual(10, c, "Allocation of scope slot for super property shouldn't affect the body variables"))();
                assert.areEqual(20, b(), "Function with eval defined in the param scope should capture the formal and reflect the assignment from the body");
            }
        }
        (new c6()).foo();
    }
  },
  {
      name: "Eval and destructuring",
      body: function () {
        function f1({a:a1, b:b1}, c = eval("a1 + b1")) {
            assert.areEqual(10, a1, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
            assert.areEqual(20, b1, "Initial value of the second destructuring parameter in the body scope should be the same as the one in param scope");
            var a1 = 1;
            var b1 = 2;
            assert.areEqual(1, a1, "New assignment in the body scope updates the first formal's value in body scope");
            assert.areEqual(2, b1, "New assignment in the body scope updates the second formal's value in body scope");
            assert.areEqual(30, c, "The param scope method should return the sum of the destructured formals from the param scope");
            return c;
        }
        assert.areEqual(30, f1({ a : 10, b : 20 }), "Method should return the sum of the destructured formals from the param scope");

        function f2( {a:a1, b:b1}, c = function () { return eval("a1 + b1"); }) {
            var a1 = 1;
            var b1 = 2;
            eval("");
            return c;
        }
        assert.areEqual(30, f2({ a : 10, b : 20 })(), "Returned method should return the sum of the destructured formals from the param scope even with eval in the body");

        function f3({x:x = 10, y:y = eval("x")}) {
            assert.areEqual(10, x, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
            var x = 20;
            assert.areEqual(20, x, "Assignment in the body updates the formal's value");
            return y;
        }
        assert.areEqual(10, f3({ }), "Returned method should return the value of the destructured formal from the param scope");

        (({x:x = 10, y:y = () => { return eval("x"); }}) => {
            assert.areEqual(10, x, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
            var x = 20;
            assert.areEqual(10, y(), "Assignment in the body does not affect the formal captured from the param scope");
        })({});

        function f4({a:a1, b:b1}, c = () => eval("a1 + b1")) {
            assert.areEqual(10, a1, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope");
            assert.areEqual(20, b1, "Initial value of the second destructuring parameter in the body scope should be the same as the one in param scope");
            a1 = 1;
            b1 = 2;
            assert.areEqual(1, a1, "New assignment in the body scope updates the first formal's value in body scope");
            assert.areEqual(2, b1, "New assignment in the body scope updates the second formal's value in body scope");
            assert.areEqual(3, c(), "The param scope method should return the sum of the destructured formals from the param scope");
            return c;
        }
        assert.areEqual(3, f4({ a : 10, b : 20 })(), "Method should return the sum of the destructured formals which are updated in the body");
      }
  },
  {
      name: "Eval and arguments in param scope",
      body: function () {
        function f1(a, b = eval("arguments[0]")) {
            assert.areEqual(1, b, "arguments should be accessible from an eval inside the param scope");
            assert.areEqual(1, arguments[0], "arguments should work fine in the body when eval present in the param scope");
        }
        f1(1);

        function f2(a, b = eval("arguments[0] = 100")) {
            assert.areEqual(100, b, "Assignment to arguments inside the eval should be reflected in the body");
            assert.areEqual(100, arguments[0], "arguments is updated after the assignment");
        }
        f2(1);

        function f3(a, b = function () { return eval("arguments[0]"); }) {
            assert.areEqual(1, b(1), "Nested function in the param scope should be able to use its own arguments object properly");
        }
        f3();
      }
  }
];


testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
