//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Assigning a bound function to the proxy's prototype should not fire the assert",
    body: function () {
        var pr = new Proxy({}, {
                getPrototypeOf: function() {
                    return;
                }
            }).__proto__ = Float64Array.bind()
    }
  },
  {
    name: "a function and it's bind function are one context and invoked from a different context",
    body: function () {
        var sc1 = WScript.LoadScript(`
        function assertAreEqual(a, b) { if (a != b) { throw new Error('expected : ' + a + ', actual : ' + b) } };
        function foo(a, b, c) {
            assertAreEqual(undefined, a);
            assertAreEqual(1, b.x);
            assertAreEqual('three', c);
            return {d:10};
        };
    
        function test() {
            var bf = foo.bind(undefined, undefined, {x:1}, 'three');
            assertAreEqual(10, bf().d);
        
            var bf1 = foo.bind(undefined, undefined);
            assertAreEqual(10, bf1({x:1}, 'three').d);
            }
        `,
        "samethread");
    
        sc1.test();
    }
  },
  {
    name: "a bound function is passed to first context and called from there",
    body: function () {
        var sc1 = WScript.LoadScript(`
        function assertAreEqual(a, b) { if (a != b) { throw new Error('expected : ' + b + ', actual : ' + a) } };
        function foo(a, b, c) {
            assertAreEqual(undefined, a);
            assertAreEqual(1, b.x);
            assertAreEqual('three', c);
            return {d:10};
        };
    
        var bf1 = foo.bind(undefined, undefined, {x:1}, 'three');
        var bf2 = foo.bind(undefined, undefined);
    
        function test() {
            return foo.bind(undefined, undefined, {x:1}, 'three');
        }
        function test1() {
            return foo.bind(undefined, undefined);
        }
        `,
        "samethread");
    
        assert.areEqual(10, sc1.bf1().d);
        assert.areEqual(10, sc1.bf2({x:1}, 'three').d);
        assert.areEqual(10, sc1.test()().d);
        assert.areEqual(10, sc1.test1()({x:1}, 'three').d);
        }
  },
  {
    name: "bound function is created on second context on the function passed from the first context",
    body: function () {
        function foo(a, b, c) {
            assert.areEqual(undefined, a);
            assert.areEqual(1, b.x);
            assert.areEqual('three', c);
            return {d:10};
        };

        var sc1 = WScript.LoadScript(`
            var bf1;
            var bf2;
            function setup(func) {
                bf1 = func.bind(undefined, undefined, {x:1}, 'three');
            }
            function setup1(func, a) {
                bf2 = func.bind(func, a);
            }
        
            function test() {
                return bf1();
            }
            function test1(a, b) {
                return bf2(a, b);
            }   
        `,
        "samethread");

        sc1.setup(foo);
        sc1.setup1(foo, undefined);
    
        assert.areEqual(10, sc1.test().d);
        assert.areEqual(10, sc1.test({x:1}, 'three').d);
    
    }
  },
  {
    name: "bound function is created on second context on the function passed from the first context and invoked directly from first context",
    body: function () {
        function foo(a, b, c) {
            assert.areEqual(undefined, a);
            assert.areEqual(1, b.x);
            assert.areEqual('three', c);
            return {d:10};
        };

        var sc1 = WScript.LoadScript(`
            function test(func) {
                return func.bind(undefined, undefined, {x:1}, 'three');
            }
            function test1(func, a) {
                return func.bind(func, a);
            }
            `,
        "samethread");

        assert.areEqual(10, sc1.test(foo)().d);
        assert.areEqual(10, sc1.test1(foo, undefined)({x:1}, 'three').d);
        
    }
  },
  {
    name: "bound function is created on second context on the function passed from the third context",
    body: function () {
        var sc1 = WScript.LoadScript(`
            function assertAreEqual(a, b) { if (a != b) { throw new Error('expected : ' + b + ', actual : ' + a) } };
            function foo(a, b, c) {
                assertAreEqual(undefined, a);
                assertAreEqual(1, b.x);
                assertAreEqual('three', c);
                return {d:10};
            };
        `,
        "samethread");
    
        function foo(a, b, c) {
            assert.areEqual(undefined, a);
            assert.areEqual(1, b.x);
            assert.areEqual('three', c);
            return {d:10};
        };

        var sc2 = WScript.LoadScript(`
            var bf1, bf2;
        
            function setup(obj, a) {
                bf1 = obj.foo.bind(undefined, undefined, {x:1}, 'three');
                bf2 = obj.foo.bind(obj.foo, a);
            }
        
            function test() {
                return bf1();
            }
            function test1(a, b) {
                return bf2(a, b);
            }
        `,
        "samethread");
    
        sc2.setup(sc1, undefined);
        assert.areEqual(10, sc2.test().d);
        assert.areEqual(10, sc2.test1({x:1}, 'three').d);
    }
  },
  
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
