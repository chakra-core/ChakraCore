//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Various object literal method deferral",
        body: function () {
            var sym = Symbol();
            let out = 'nothing';
            var obj = {
                get a() { return 'get a'; },
                set a(v) { out = 'set a'; },
                b() { return 'b'; },
                ['c']() { return 'c'; },
                [sym]() { return 'sym'; },
                async d() { return 'd'; },
                *e() { yield 'e'; },
                get ['f']() { return 'get f'; },
                set ['f'](v) { out = 'set f'; },
                async ['g']() { return 'g'; },
                *['h']() { yield 'h'; },
                async async() { return 'async async'; },
            }
            var obj2 = {
                async() { return 'async'; }
            }
            var obj3 = {
                get async() { return 'get async'; },
                set async(v) { out = 'set async'; }
            }
            var obj4 = {
                *async() { yield 'generator async'; }
            }

            assert.areEqual('get a', obj.a, "Simple named getter");
            obj.a = 123;
            assert.areEqual('set a', out, "Simple named setter");
            assert.areEqual('b', obj.b(), "Simple method");

            assert.areEqual('c', obj.c(), "Method with computed property name");
            assert.areEqual('sym', obj[sym](), "Method with computed property name (key is not string)");

            assert.isTrue(obj.d() instanceof Promise, "Async method");

            assert.areEqual('e', obj.e().next().value, "Generator method");

            assert.areEqual('get f', obj.f, "Getter method with computed name");
            obj.f = 123;
            assert.areEqual('set f', out, "Setter method with computed name");

            assert.isTrue(obj.g() instanceof Promise, "Async method with computed name");

            assert.areEqual('h', obj.h().next().value, "Generator method with computed name");
            
            assert.isTrue(obj.async() instanceof Promise, "Async method named async");
            assert.areEqual('async', obj2.async(), "Method named async");
            assert.areEqual('get async', obj3.async, "Getter named async");
            obj3.async = 123;
            assert.areEqual('set async', out, "Setter named async");
            assert.areEqual('generator async', obj4.async().next().value, "Generator method named async");
        }
    },
    {
        name: "Uncommon object literal method name types",
        body: function() {
            var out = 'nothing';
            var obj = {
                "s1"() { return "s1"; },
                async "s2"() { return "s2"; },
                * "s3"() { return "s3"; },
                get "s4"() { return "s4"; },
                set "s4"(v) { out = "s4"; },

                0.1() { return 0.1; },
                async 0.2() { return 0.2; },
                * 0.3() { return 0.3; },
                get 0.4() { return 0.4; },
                set 0.4(v) { out = 0.4; },

                123() { return 123; },
                async 456() { return 456; },
                * 789() { yield 789; },
                get 123456() { return 123456; },
                set 123456(v) { out = 123456; },

                while() { return "while"; },
                async else() { return "else"; },
                * if() { return "if"; },
                get catch() { return "catch"; },
                set catch(v) { out = "catch"; },
            }

            assert.areEqual('s1', obj.s1(), "Method with string name");
            assert.areEqual(0.1, obj[0.1](), "Method with float name");
            assert.areEqual(123, obj[123](), "Method with numeric name");
            assert.areEqual('while', obj.while(), "Method with keyword name");
            
            assert.isTrue(obj.s2() instanceof Promise, "Async method with string name");
            assert.isTrue(obj[0.2]() instanceof Promise, "Async method with float name");
            assert.isTrue(obj[456]() instanceof Promise, "Async method with numeric name");
            assert.isTrue(obj.else() instanceof Promise, "Async method with keyword name");

            assert.areEqual('s3', obj.s3().next().value, "Generator method with string name");
            assert.areEqual(0.3, obj[0.3]().next().value, "Generator method with float name");
            assert.areEqual(789, obj[789]().next().value, "Generator method with numeric name");
            assert.areEqual('if', obj.if().next().value, "Generator method with keyword name");

            assert.areEqual('s4', obj.s4, "Getter method with string name");
            assert.areEqual(0.4, obj[0.4], "Getter method with float name");
            assert.areEqual(123456, obj[123456], "Getter method with numeric name");
            assert.areEqual('catch', obj.catch, "Getter method with keyword name");

            obj.s4 = 123
            assert.areEqual('s4', out, "Setter method with string name");
            obj[0.4] = 123
            assert.areEqual(0.4, out, "Setter method with float name");
            obj[123456] = 123
            assert.areEqual(123456, out, "Setter method with numeric name");
            obj.catch = 123
            assert.areEqual('catch', out, "Setter method with keyword name");
        }
    },
    {
        name: "Regular function nested in a deferred method",
        body: function() {
            var obj = {
                m() {
                    function foo() { return 'foo'; }
                    return foo();
                }
            }

            assert.areEqual('foo', obj.m(), "Regular function nested in a deferred method should not be parsed as a method");
        }
    },
    {
        name: "Method with 'super' capture",
        body: function() {
            var obj = {
                m() { return () => super.bar(); }
            }
            Object.setPrototypeOf(obj, { bar() { return this; } });
            
            assert.areEqual(obj, obj.m()(), "Method should call lambda should call super method should return this captured from obj.m");
        }
    },
    {
        name: "Async lambda with parens",
        body: function() {
            var a = async() => { };
            
            assert.isTrue(a() instanceof Promise, "Async lambda with parens around formal parameters")
        }
    }
]

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
