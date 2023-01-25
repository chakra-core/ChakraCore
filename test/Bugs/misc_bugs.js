//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check
/// <reference path="..\UnitTestFramework\UnitTestFramework.js" />

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "calling Symbol.toPrimitive on Date prototype should not AV",
        body: function () {
            Date.prototype[Symbol.toPrimitive].call({}, 'strin' + 'g');
        }
    },
    {
        name: "updated stackTraceLimit should not fire re-entrancy assert",
        body: function () {
            Error.__defineGetter__('stackTraceLimit', function () { return 1; });
            assert.throws(() => Array.prototype.map.call([]));
        }
    },
    {
        name: "Array.prototype.slice should not fire re-entrancy error when the species returns proxy",
        body: function () {
            let arr = [1, 2];
            arr.__proto__ = {
                constructor: {
                    [Symbol.species]: function () {
                        return new Proxy({}, {
                            defineProperty(...args) {
                                return Reflect.defineProperty(...args);
                            }
                        });
                    }
                }
            }
            Array.prototype.slice.call(arr);
        }
    },
    {
        name: "rest param under eval with arguments usage in the body should not fail assert",
        body: function () {
            f();
            function f() {
                eval("function bar(...x){arguments;}")
            }
        }
    },
    {
        name: "Token left after parsing lambda result to the syntax error",
        body: function () {
            assert.throws(() => { eval('function foo ([ [] = () => { } = {a2:z2}]) { };'); });
        }
    },
    {
        name: "Token left after parsing lambda in ternary operator should not throw",
        body: function () {
            assert.doesNotThrow(() => { eval('function foo () {  true ? e => {} : 1};'); });
        }
    },
    {
        name: "ArrayBuffer.slice with proxy constructor should not fail fast",
        body: function () {
            let arr = new ArrayBuffer(10);
            arr.constructor = new Proxy(ArrayBuffer, {});

            arr.slice(1, 2);
        }
    },
    {
        name: "Large proxy chain should not cause IsConstructor to crash on stack overflow",
        body: function () {
            let p = new Proxy(Object, {});
            for (let i = 0; i < 20000; ++i) {
                p = new Proxy(p, {});
            }
            try {
                let a = new p();
            }
            catch (e) {
            }
        }
    },
    {
        name: "splice an array which has getter/setter at 4294967295 should not fail due to re-entrancy error",
        body: function () {
            var base = 4294967290;
            var arr = [];
            for (var i = 0; i < 10; i++) {
                arr[base + i] = 100 + i;
            }
            Object.defineProperty(arr, 4294967295, {
                get: function () { }, set: function (b) { }
            }
            );

            assert.throws(() => { arr.splice(4294967290, 0, 200, 201, 202, 203, 204, 205, 206); });
        }
    },
    {
        name: "Passing args count near 2**16 should not fire assert (OS# 17406027)",
        body: function () {
            try {
                eval.call(...(new Array(2 ** 16)));
            } catch (e) { }

            try {
                eval.call(...(new Array(2 ** 16 + 1)));
            } catch (e) { }

            try {
                var sc1 = WScript.LoadScript(`function foo() {}`, "samethread");
                sc1.foo(...(new Array(2 ** 16)));
            } catch (e) { }

            try {
                var sc2 = WScript.LoadScript(`function foo() {}`, "samethread");
                sc2.foo(...(new Array(2 ** 16 + 1)));
            } catch (e) { }

            try {
                function foo() { }
                Reflect.construct(foo, new Array(2 ** 16 - 3));
            } catch (e) { }

            try {
                function foo() { }
                Reflect.construct(foo, new Array(2 ** 16 - 2));
            } catch (e) { }

            try {
                function foo() { }
                var bar = foo.bind({}, 1);
                new bar(...(new Array(2 ** 16 + 1)))
            } catch (e) { }
        }
    },
    {
        name: "Using RegExp as newTarget should not assert",
        body: function () {
            var v0 = function () { this.a; };
            var v1 = class extends v0 { constructor() { super(); } };
            Reflect.construct(v1, [], RegExp);
        }
    },
    {
        name: "getPrototypeOf Should not be called when set as prototype",
        body: function () {
            var p = new Proxy({}, {
                getPrototypeOf: function () {
                    assert.fail("this should not be called")
                    return {};
                }
            });

            var obj = {};
            obj.__proto__ = p; // This should not call the getPrototypeOf

            var obj1 = {};
            Object.setPrototypeOf(obj1, p); // This should not call the getPrototypeOf

            var obj2 = { __proto__: p }; // This should not call the getPrototypeOf
        }
    },
    {
        name: "Cross-site activation object",
        body: function () {
            var tests = [0, 0];
            tests.forEach(function () {
                var eval = WScript.LoadScript(0, "samethread").eval;
                eval(0);
            });
        }
    },
    {
        name: "Destructuring declaration should return undefined",
        body: function () {
            assert.areEqual(undefined, eval("var {x} = {};"));
            assert.areEqual(undefined, eval("let {x,y} = {};"));
            assert.areEqual(undefined, eval("const [z] = [];"));
            assert.areEqual(undefined, eval("let {x} = {}, y = 1, {z} = {};"));
            assert.areEqual([1], eval("let {x} = {}; [x] = [1]"));
        }
    },
    {
        name: "Strict Mode : throw type error when the handler returns falsy value",
        body: function () {
            assert.throws(() => { "use strict"; let p1 = new Proxy({}, { set() { } }); p1.foo = 1; }, TypeError, "returning undefined on set handler is return false which will throw type error", "Proxy 'set' handler returned falsish for property 'foo'");
            assert.throws(() => { "use strict"; let p1 = new Proxy({}, { deleteProperty() { } }); delete p1.foo; }, TypeError, "returning undefined on deleteProperty handler is return false which will throw type error", "Proxy 'deleteProperty' handler returned falsish for property 'foo'");
            assert.throws(() => { "use strict"; let p1 = new Proxy({}, { set() { return false; } }); p1.foo = 1; }, TypeError, "set handler is returning false which will throw type error", "Proxy 'set' handler returned falsish for property 'foo'");
            assert.throws(() => { "use strict"; let p1 = new Proxy({}, { deleteProperty() { return false; } }); delete p1.foo; }, TypeError, "deleteProperty handler is returning false which will throw type error", "Proxy 'deleteProperty' handler returned falsish for property 'foo'");

            const proxy = new Proxy({}, {
                defineProperty() {
                    return false;
                }
            });
            assert.doesNotThrow(() => {
                proxy.a = {};
            }, "Set property in NON-strict mode does NOT throw if trap returns falsy");
            assert.throws(() => {
                "use strict";
                proxy.b = {};
            }, TypeError, "Set property in strict mode does DOES throw if trap returns falsy");
            assert.throws(() => {
                Object.defineProperty(proxy, "c", {
                    value: {}
                });
            }, TypeError, "Calling 'Object.defineProperty' throws if trap returns falsy");
        }
    },
    {
        name: "Generator : testing recursion",
        body: function () {
            // This will throw out of stack error
            assert.throws(() => {
                function foo() {
                    function* f() {
                        yield foo();
                    }
                    f().next();
                }
                foo();
            });
        }
    },
    {
        name: "destructuring : testing recursion",
        body: function () {
            try {
                eval(`
            var ${'['.repeat(6631)}
          `);
                assert.fail();
            }
            catch (e) {
            }

            try {
                eval(`
               var {${'a:{'.repeat(6631)}
            `);
                assert.fail();
            }
            catch (e) {
            }
        }
    },
    {
        name: "CrossSite issue while array concat OS: 18874745",
        body: function () {
            function test0() {
                var IntArr0 = Array();
                var sc0Code = `
              Object.defineProperty(Array, Symbol.species, { value : function() {
                      return IntArr0;
                      }
                  }
              );
            test = function(a, list) {
                return [a].concat(list);
            }
            function out() {
              test({}, [1]);
            }
            `;
                var sc0 = WScript.LoadScript(sc0Code, 'samethread');
                sc0.IntArr0 = IntArr0;
                sc0.out();
            }
            test0();
        }
    },
    {
        name: "Init box javascript array : OS : 20517662",
        body: function () {
            var obj = {};
            obj[0] = 11;
            obj[1] = {};
            obj[17] = 222;
            obj[35] = 333; // This is will increase the size past the inline segment

            Object.assign({}, obj); // The InitBoxedInlineSegments will be called due to this call.
        }
    },
    {
        name: "calling promise's function as constructor should not be allowed",
        body: function () {
            var var_0 = new Promise(function () { });
            var var_1 = function () { };

            var_0.then = function (a, b) {
                var_2 = b;
            };
            var_3 = Promise.prototype.finally.call(var_0, var_1);
            assert.throws(() => { new var_2([]).var_3(); }, TypeError);
        }
    },
    {
        name: "issue : 6174 : calling ToNumber for the fill value for typedarray.prototype.fill",
        body: function () {
            let arr1 = new Uint32Array(5);
            let valueOfCalled = false;
            let p1 = new Proxy([], {
                get: function (oTarget, sKey) {
                    if (sKey.toString() == 'valueOf') {
                        valueOfCalled = true;
                    }
                    return Reflect.get(oTarget, sKey);
                }
            });
            Uint32Array.prototype.fill.call(arr1, p1, 5, 1);
            assert.isTrue(valueOfCalled);
        }
    },
    {
        name: "class name should not change if calling multiple times",
        body: function () {
            function getClass() {
                class A {
                    constructor() {

                    }
                };
                return A;
            }
            let f1 = getClass();
            let f2 = getClass();
            let f3 = getClass();
            assert.areEqual("A", f1.name);
            assert.areEqual("A", f2.name);
            assert.areEqual("A", f3.name);
        }
    },
    {
        name: "should throw syntax error when expression within if is comma-terminated",
        body: function () {
            assert.throws(() => { eval('var f = function () { var f = 0; if (f === 0,) print("run_here"); }; f();'); }, SyntaxError);
        }
    },
    {
        name: "Correct parsing of (missing) expression bodies",
        body() {
            assert.throws(() => eval("if(true)"), SyntaxError);
            assert.doesNotThrow(() => eval("if(true);"));
            assert.doesNotThrow(() => eval("if(true){}"));

            assert.throws(() => eval("if(true){}else"), SyntaxError);
            assert.doesNotThrow(() => eval("if(true){}else;"));
            assert.doesNotThrow(() => eval("if(true){}else{}"));

            assert.throws(() => eval("while(false)"), SyntaxError);
            assert.doesNotThrow(() => eval("while(false);"));
            assert.doesNotThrow(() => eval("while(false){}"));

            assert.throws(() => eval("for(;false;)"), SyntaxError);
            assert.doesNotThrow(() => eval("for(;false;);"));
            assert.doesNotThrow(() => eval("for(;false;){}"));

            assert.throws(() => eval("do"), SyntaxError);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
