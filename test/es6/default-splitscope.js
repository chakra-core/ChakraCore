//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

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

        function f4(a = 10, b = function () { return a; }) { 
            a = 20; 
            return b; 
        } 
        assert.areEqual(10, f4()(), "Even if the formals are not redeclared in the function body the symbol in the param scope and body scope are different"); 

        function f5(a = 10, b = function () { return function () { return a; }; }) { 
            var a = 20; 
            return b; 
        } 
        assert.areEqual(10, f5()()(), "Parameter scope works fine with nested functions"); 

        var a1 = 10; 
        function f6(b = function () { return a1; }) { 
            assert.areEqual(undefined, a1, "Inside the function body the assignment hasn't happened yet"); 
            var a1 = 20; 
            assert.areEqual(20, a1, "Assignment to the symbol inside the function changes the value"); 
            return b; 
        } 
        assert.areEqual(10, f6()(), "Function in the param scope correctly binds to the outer variable"); 
         
        function f7(a = 10, b = { iFnc () { return a; } }) { 
            a = 20; 
            return b; 
        } 
        assert.areEqual(10, f7().iFnc(), "Function definition inside the object literal should capture the formal from the param scope"); 
    } 
 }, 
 { 
    name: "Split parameter scope in function expressions with name", 
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
            a = 20; 
            return b; 
        } 
        assert.areEqual(10, f4()()(), "Nested lambda should capture the formal param value from the param scope"); 

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
        function f1(a = this.x, b = function() { assert.areEqual(100, this.x, "this object for the function in param scope is passed from the final call site"); return a; }) { 
            assert.areEqual(10, this.x, "this objects property retains the value from param scope"); 
            a = 20; 
            return b; 
        } 
        assert.areEqual(10, f1.call({x : 10}).call({x : 100}), "Arrow functions defined in the param scope captures the formals from the param scope not body scope"); 
         
        (function (a = this.x, b = function() {this.x = 20; return a;}) { 
            assert.areEqual(10, this.x, "this objects property retains the value in param scope before the inner function call"); 
            b.call(this); 
            assert.areEqual(20, this.x, "Update to a this's property from the param scope is reflected in the body scope"); 
        }).call({x : 10}); 
         
        this.x = 10; 
        ((a = this.x, b = function() {this.x = 20}) => { 
            assert.areEqual(10, this.x, "this objects property retains the value in param scope before the inner function call in lambda"); 
            b.call(this); 
            assert.areEqual(20, this.x, "Update to a this's property from the param scope of lambda function is reflected in the body scope"); 
        })(); 
         
        function f2(a = function() { return this.x; }, b = this.y, c = a.call({x : 20}) + b) { 
            assert.areEqual(undefined, this.x, "this object remains unaffected"); 
            return c; 
        } 
        assert.areEqual(30, f2.call({y : 10}), "Properties are accessed from the right this object"); 

        var thisObj = {x : 1, y: 20};             
        function f3(b = () => { this.x = 10; return this.y; }) { 
            return b; 
        } 
        assert.areEqual(20, f3.call(thisObj)(), "Lambda defined in the param scope returns the right property value from thisObj"); 
        assert.areEqual(10, thisObj.x, "Assignment from the param scope method updates thisObj's property"); 
    } 
  },
  { 
    name: "Split parameter scope in class methods", 
    body: function () { 
        class c { 
            f(a = 10, d, b = function () { return a; }, c) { 
                assert.areEqual(10, a, "Initial value of parameter in the body scope in class method should be the same as the one in param scope"); 
                var a = 20; 
                assert.areEqual(20, a, "Assignment in the class method body updates the value of the variable"); 
                return b; 
            } 
        } 
        assert.areEqual(10, (new c()).f()(), "Method defined in the param scope of the class should capture the formal from the param scope itself"); 

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
            a = 20; 
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
                c = []; 
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
                x = 20; 
                assert.areEqual(20, x, "Assignment in the class method body updates the value of the variable"); 
                return y; 
            } 
        } 
        assert.areEqual(10, (new c4()).f({})(), "The method defined as the default destructured value of the parameter should capture the formal from the param scope"); 
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
        assert.areEqual(10, f2Obj.next().value(), "Function defined in the param scope captures the formal from the param scope itself even if it is not redeclared in the body"); 

        function *f3(a = 10, d, b = function *() { yield a + c; }, c = 100) { 
            a = 20; 
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
            a1 = 1; 
            b1 = 2; 
            assert.areEqual(1, a1, "New assignment in the body scope updates the first formal's value in body scope"); 
            assert.areEqual(2, b1, "New assignment in the body scope updates the second formal's value in body scope"); 
            assert.areEqual(30, c(), "The param scope method should return the sum of the destructured formals from the param scope"); 
            return c; 
        } 
        assert.areEqual(30, f1({ a : 10, b : 20 })(), "Returned method should return the sum of the destructured formals from the param scope"); 

        function f2({x:x = 10, y:y = function () { return x; }}) { 
            assert.areEqual(10, x, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope"); 
            x = 20; 
            assert.areEqual(20, x, "Assignment in the body updates the formal's value"); 
            return y; 
        } 
        assert.areEqual(10, f2({ })(), "Returned method should return the value of the destructured formal from the param scope"); 
         
        function f3({y:y = function () { return x; }, x:x = 10}) { 
            assert.areEqual(10, x, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope"); 
            x = 20; 
            assert.areEqual(20, x, "Assignment in the body updates the formal's value"); 
            return y; 
        } 
        assert.areEqual(10, f3({ })(), "Returned method should return the value of the destructured formal from the param scope even if declared after"); 
         
        (({x:x = 10, y:y = function () { return x; }}) => { 
            assert.areEqual(10, x, "Initial value of the first destructuring parameter in the body scope should be the same as the one in param scope"); 
            x = 20; 
            assert.areEqual(10, y(), "Assignment in the body does not affect the formal captured from the param scope"); 
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
          assert.areEqual(10, result[0](), "Function defined in the param scope of the outer function should capture the symbols from its own param scope"); 
          assert.areEqual(300, result[1](), "Function defined in the param scope of the inner function should capture the symbols from its own param scope"); 
          assert.areEqual(3000, result[2](), "Function defined in the body scope of the inner function should capture the symbols from its body scope"); 

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
          assert.areEqual(300, f3()[1]()(), "Function defined in the param scope of the inner function should capture the right formals even if the inner function is executed outside"); 

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
          assert.areEqual(1, result[0](), "Function defined in the param scope of the outer function correctly captures the passed in value for the formal"); 
          assert.areEqual(1, result[1](), "Function defined in the param scope of the inner function is replaced by the function definition from the param scope of the outer function"); 

          function f5(a = 10, b = function () { return a; }, c) { 
              function iFnc(a = 100, b = function () { return a + c; }, c = 200) { 
                  a = 1000; 
                  c = 2000; 
                  return b; 
              } 
              return [b, iFnc(a, undefined, c)]; 
          } 
          result = f5(1, undefined, 3); 
          assert.areEqual(1, result[0](), "Function defined in the param scope of the outer function correctly captures the passed in value for the formal"); 
          assert.areEqual(4, result[1](), "Function defined in the param scope of the inner function captures the passed values for the formals"); 

          function f6(a , b, c) { 
              function iFnc(a = 1, b = function () { return a + c; }, c = 2) { 
                  a = 10; 
                  c = 20; 
                  return b; 
              } 
              return iFnc; 
          } 
          assert.areEqual(3, f6()()(), "Function defined in the param scope captures the formals when defined inside another method without split scope"); 

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
          assert.areEqual(30, result[0](), "Function defined in the param scope of the outer function should capture the symbols from its own param scope even in nested case"); 
          assert.areEqual(300, result[1]()(), "Function defined in the param scope of the inner function should capture the symbols from its own param scope even when nested inside a normal method and a split scope"); 

          function f8(a = 1, b = function (d = 10, e = function () { return a + d; }) { assert.areEqual(d, 10, "Split scope function defined in param scope should capture the right formal value"); d = 20; return e; }, c) { 
              a = 2; 
              return b; 
          } 
          assert.areEqual(11, f8()()(), "Split scope function defined within the param scope should capture the formals from the corresponding param scopes"); 

          function f9(a = 1, b = function () { return function (d = 10, e = function () { return a + d; }) { d = 20; return e; } }, c) { 
              a = 2; 
              return b; 
          } 
          assert.areEqual(11, f9()()()(), "Split scope function defined within the param scope should capture the formals from the corresponding param scope in nested scope"); 
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
            if (x === undefined) { 
                // only a should be visible 
                assert.areEqual(1, a1, "Var declarations leak out of eval into parameter scope"); 
            } else { 
                // none should be visible 
                assert.throws(function () { a1 }, ReferenceError, "Ignoring the default value does not result in an eval declaration leaking", "'a1' is undefined"); 
            } 

            assert.throws(function () { b1 }, ReferenceError, "Let declarations do not leak out of eval to parameter scope",   "'b1' is undefined"); 
            assert.throws(function () { c1 }, ReferenceError, "Const declarations do not leak out of eval to parameter scope when x is ", "'c1' is undefined"); 
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

        // TODO(aneeshdk): Will revisit this when enabling the eval cases
        // assert.throws(function() { return function(a = eval("var x = 1;")) { let x = 2; }; },   ReferenceError, "Redefining a local let declaration with a parameter scope eval var declaration leak throws",   "Let/Const redeclaration"); 
        // assert.throws(function() { return (function(a = eval("var x = 1;")) { const x = 2; })(); }, ReferenceError, "Redefining a local const declaration with a parameter scope eval var declaration leak throws", "Let/Const redeclaration"); 

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
