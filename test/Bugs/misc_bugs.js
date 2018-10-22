//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "calling Symbol.toPrimitive on Date prototype should not AV",
    body: function () {
         Date.prototype[Symbol.toPrimitive].call({},'strin' + 'g');
    }
  },
  {
    name: "updated stackTraceLimit should not fire re-entrancy assert",
    body: function () {
        Error.__defineGetter__('stackTraceLimit', function () { return 1;});
        assert.throws(()=> Array.prototype.map.call([]));
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
        assert.throws(()=> { eval('function foo ([ [] = () => { } = {a2:z2}]) { };'); });
    }
  },
  {
    name: "Token left after parsing lambda in ternary operator should not throw",
    body: function () {
        assert.doesNotThrow(()=> { eval('function foo () {  true ? e => {} : 1};'); });
    }
  },
  {
    name: "ArrayBuffer.slice with proxy constructor should not fail fast",
    body: function () {
      let arr = new ArrayBuffer(10);
      arr.constructor = new Proxy(ArrayBuffer, {});
      
      arr.slice(1,2);
    }
  },
  {
    name: "Large proxy chain should not cause IsConstructor to crash on stack overflow",
    body: function () {
      let p = new Proxy(Object, {});
      for (let  i=0; i<20000; ++i)
      {
          p = new Proxy(p, {});
      }
      try
      {
          let a = new p();
      }
      catch(e)
      {
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
          get: function () { }, set : function(b) {  }
            }
        );

        assert.throws(()=> {arr.splice(4294967290, 0, 200, 201, 202, 203, 204, 205, 206);});
    }
  },
  {
    name: "Passing args count near 2**16 should not fire assert (OS# 17406027)",
    body: function () {
      try {
        eval.call(...(new Array(2**16)));
      } catch (e) { }
      
      try {
        eval.call(...(new Array(2**16+1)));
      } catch (e) { }
      
      try {
        var sc1 = WScript.LoadScript(`function foo() {}`, "samethread");
        sc1.foo(...(new Array(2**16)));
      } catch(e) { }
      
      try {
        var sc2 = WScript.LoadScript(`function foo() {}`, "samethread");
        sc2.foo(...(new Array(2**16+1)));
      } catch(e) { }

      try {
        function foo() {}
        Reflect.construct(foo, new Array(2**16-3));
      } catch(e) { }

      try {
        function foo() {}
        Reflect.construct(foo, new Array(2**16-2));
      } catch(e) { }

      try {
        function foo() {}
        var bar = foo.bind({}, 1);
        new bar(...(new Array(2**16+1)))
      } catch(e) { }
    }
  },
  {
    name: "getPrototypeOf Should not be called when set as prototype",
    body: function () {
      var p = new Proxy({}, { getPrototypeOf: function() {
          assert.fail("this should not be called")
          return {};
        }});

        var obj = {};
        obj.__proto__ = p; // This should not call the getPrototypeOf

        var obj1 = {};
        Object.setPrototypeOf(obj1, p); // This should not call the getPrototypeOf

        var obj2 = {__proto__ : p}; // This should not call the getPrototypeOf
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
        assert.throws(() => {"use strict"; let p1 = new Proxy({}, { set() {}}); p1.foo = 1;}, TypeError, "returning undefined on set handler is return false which will throw type error",  "Proxy set handler returned false");
        assert.throws(() => {"use strict"; let p1 = new Proxy({}, { deleteProperty() {}}); delete p1.foo;}, TypeError, "returning undefined on deleteProperty handler is return false which will throw type error",  "Proxy deleteProperty handler returned false");
        assert.throws(() => {"use strict"; let p1 = new Proxy({}, { set() {return false;}}); p1.foo = 1;}, TypeError, "set handler is returning false which will throw type error",  "Proxy set handler returned false");
        assert.throws(() => {"use strict"; let p1 = new Proxy({}, { deleteProperty() {return false;}}); delete p1.foo;}, TypeError, "deleteProperty handler is returning false which will throw type error",  "Proxy deleteProperty handler returned false");
      }
  },
  {
    name: "Generator : testing recursion",
    body: function () {
        // This will throw out of stack error
        assert.throws(() => {
            function foo() {
                function *f() {
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
  }

];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
