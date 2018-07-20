//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "typeof of literals, built-in types and object wrappers",
        body: function () {
            var arr = [42];

            assert.areEqual("object", typeof null, "typeof null");
            assert.areEqual("undefined", typeof undefined, "typeof undefined");
            assert.areEqual("string", typeof "", "typeof empty string");
            assert.areEqual("boolean", typeof false, "typeof false");
            assert.areEqual("number", typeof 0, "typeof 0");
            assert.areEqual("number", typeof 12345.678, "typeof 12345.678");
            assert.areEqual("object", typeof {}, "typeof {}");
            assert.areEqual("object", typeof arr, "typeof array");
            assert.areEqual("number", typeof arr[0], "typeof arr[0]");
            assert.areEqual("undefined", typeof arr[1], "typeof arr[1]");
            assert.areEqual("object", typeof /abc/, "typeof /abc/");
            assert.areEqual("function", typeof (function (){}), "typeof (function (){})");
            assert.areEqual("function", typeof (() => 42), "typeof (() => 42)");
            assert.areEqual("symbol", typeof Symbol(), "typeof Symbol()");

            // built-in object and object wrappers
            assert.areEqual("object", typeof (new String("")), "typeof empty string object wrapper");
            assert.areEqual("object", typeof (new Boolean(false)), "typeof (new Boolean(false))");
            assert.areEqual("object", typeof (new Number(0)), "typeof (new Number(0))");
            assert.areEqual("object", typeof (new Number(12345.678)), "typeof (new Number(12345.678))");
            assert.areEqual("object", typeof (new Object()), "typeof (new Object())");
            assert.areEqual("object", typeof (new Array()), "typeof (new Array())");
            assert.areEqual("object", typeof (new Date(123)), "typeof (new Date(123))");
        }
    },
    {
        name: "typeof of expressions",
        body: function () {
            function f() {
                function g() { }
                return g;
            }
            assert.areEqual("function", typeof f(), "typeof of function call");
            assert.areEqual("number", typeof eval(7*6), "typeof of direct eval");
        }
    },
    {
        name: "typeof handling of undefined variables",
        body: function () {
            assert.areEqual("undefined", typeof x, "typeof of undeclaired var");
            assert.areEqual("undefined", typeof {}.x, "typeof {}.x");

            assert.areEqual("undefined", typeof hoisted, "typeof of hoisted var");
            var hoisted = 42;

            function inner() { var scoped = 42; }
            assert.areEqual("undefined", typeof scoped, "typeof of var when it's out of scope");

            assert.throws(()=>{ typeof x.y; }, ReferenceError, "typeof of property on undefined obj", "'x' is not defined");
            assert.throws(()=>{ typeof x[0]; }, ReferenceError, "typeof of property on undefined obj", "'x' is not defined");
            assert.throws(()=>{ typeof (()=>x)(); }, ReferenceError, "reference error in the function invoked by typeof", "'x' is not defined");

            assert.throws(()=>{ typeof x_let; }, ReferenceError, "typeof of 'let' variable in its dead zone", "Use before declaration");
            let x_let = 42;

            assert.throws(()=>{ typeof x_const; }, ReferenceError, "typeof of 'const' variable in its dead zone", "Use before declaration");
            const x_const = 42;
        }
    },
    {
        name: "typeof should propagate user exceptions",
        body: function () {
            function foo() { throw new Error("abc"); }
            assert.throws(()=>{typeof foo()}, Error, "exception raised from the invoked function", "abc");
            
            var obj = { get x() { throw new Error("xyz")}};
            assert.throws(()=>{typeof obj.x}, Error, "exception raised from the getter", "xyz");
        }
    },
    {
        name: "typeof should propagate stack overflow",
        body: function () {
            var obj = {};
            var handler = {
                get: function () {
                    return obj.x;
                }
            };
            obj = new Proxy(obj, handler);
            assert.throws(()=>{typeof obj.x}, Error, "recursive proxy should hit SO", "Out of stack space");
        }
    },
];
 
testRunner.runTests(tests, { verbose: false /*so no need to provide baseline*/ });