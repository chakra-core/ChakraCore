//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// ES2018 Async Generator API tests -- verifies built-in API objects and properties

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const globObj = this;

function checkAttributes(name, o, p, a) {
    const desc = Object.getOwnPropertyDescriptor(o, p);

    const msgPrefix = "Property " + p.toString() + " on " + name + " is ";

    assert.isTrue(!!desc, msgPrefix + "not found; there is no descriptor");

    assert.areEqual(a.writable, desc.writable, msgPrefix + (a.writable ? "" : "not") + " writable");
    assert.areEqual(a.enumerable, desc.enumerable, msgPrefix + (a.enumerable ? "" : "not") + " enumerable");
    assert.areEqual(a.configurable, desc.configurable, msgPrefix + (a.configurable ? "" : "not") + " configurable");
}

function testMethod(object, name, objectName, methodName = name) {
    assert.isTrue(name in object, `${objectName} should have property ${name}`);
    const method = object[name];
    assert.areEqual(methodName, method.name, `${objectName}.${name}.name should equal ${methodName}`);
    assert.areEqual('function', typeof method, `${objectName}.${name} should be a function`);
    assert.isTrue(method.toString().includes("[native code]"), `${objectName}.${name} should be a tagged as [native code]`);
}

const tests = [
    {
        name: "Async Generator function with overwritten prototype",
        body: function () {
            async function* agf() {};
            var gfp = agf.prototype;
            assert.strictEqual(agf().__proto__, gfp, "Async Generator function uses prototype.");
            var obj = {};
            agf.prototype = obj;
            assert.strictEqual(agf().__proto__, obj, "Async Generator function uses overwritten prototype.");
            agf.prototype = 1;
            assert.areEqual(agf().__proto__.toString(), gfp.toString(), "Generator function falls back to original prototype.");
            if (agf().__proto__ === gfp) { assert.error("Original prototype should not be same object as gfp")}
            var originalGfp = agf().__proto__;
            assert.strictEqual(agf().__proto__, originalGfp, "Async Generator function falls back to original prototype.");
            agf.prototype = 0;
            assert.strictEqual(agf().__proto__, originalGfp, "Async Generator function falls back to original prototype.");
            agf.prototype = "hi";
            assert.strictEqual(agf().__proto__, originalGfp, "Async Generator function falls back to original prototype.");
            delete gfp.prototype;
            assert.strictEqual(agf().__proto__, originalGfp, "Async Generator function falls back to original prototype.");
        }
    },
    {
        name: "AsyncGeneratorFunction is not exposed on the global object",
        body: function () {
            assert.isFalse(globObj.hasOwnProperty("AsyncGeneratorFunction"), "Global object does not have property named AsyncGeneratorFunction");
        }
    },
    {
        name: "Async generator function instances have length and name properties",
        body: function () {
            async function* agf () {}
            assert.isTrue(agf.hasOwnProperty("length"), "Async generator function objects have a 'length' property");
            assert.isTrue(agf.hasOwnProperty("name"), "Async generator function objects have a 'name' property");
            checkAttributes("agf", agf, "length", { writable: false, enumerable: false, configurable: true });
            checkAttributes("agf", agf, "name", { writable: false, enumerable: false, configurable: true });
            assert.areEqual(0, agf.length, "Async generator function object's 'length' property matches the number of parameters (0)");
            assert.areEqual("agf", agf.name, "Async generator function object's 'name' property matches the function's name");
            async function* agf2(a, b, c) { }
            assert.areEqual(3, agf2.length, "Async generator function object's 'length' property matches the number of parameters (3)");
        }
    },
    {
        name: "Async generator functions are not constructors",
        body: function () {
            async function* agf () {}
            assert.throws(function () { new agf(); }, TypeError, "Async functions cannot be used as constructors", "Function is not a constructor");
        }
    },
    {
        name: "Async generator functions do not have arguments nor caller properties regardless of strictness",
        body: function () {
            async function* agf () {}
            assert.isFalse(agf.hasOwnProperty("arguments"), "Async function objects do not have an 'arguments' property");
            assert.isFalse(agf.hasOwnProperty("caller"), "Async function objects do not have a 'caller' property");

            // Test JavascriptFunction APIs that special case PropertyIds::caller and ::arguments
            Object.setPrototypeOf(agf, Object.prototype); // Remove Function.prototype so we don't find its 'caller' and 'arguments' in these operations
            assert.isFalse("arguments" in agf, "Has operation on 'arguments' property returns false initially");
            assert.areEqual(undefined, agf.arguments, "Get operation on 'arguments' property returns undefined initially");
            assert.areEqual(undefined, Object.getOwnPropertyDescriptor(agf, "arguments"), "No property descriptor for 'arguments' initially");
            assert.isTrue(delete agf.arguments, "Delete operation on 'arguments' property returns true");

            assert.areEqual(0, agf.arguments = 0, "Set operation on 'arguments' creates new property with assigned value");
            assert.isTrue("arguments" in agf, "Has operation on 'arguments' property returns true now");
            assert.areEqual(0, agf.arguments, "Get operation on 'arguments' property returns property value now");
            checkAttributes("agf", agf, "arguments", { writable: true, enumerable: true, configurable: true });
            assert.isTrue(delete agf.arguments, "Delete operation on 'arguments' property still returns true");
            assert.isFalse(agf.hasOwnProperty("arguments"), "'arguments' property is gone");

            assert.isFalse("caller" in agf, "Has operation on 'caller' property returns false initially");
            assert.areEqual(undefined, agf.caller, "Get operation on 'caller' property returns undefined initially");
            assert.areEqual(undefined, Object.getOwnPropertyDescriptor(agf, "caller"), "No property descriptor for 'caller' initially");
            assert.isTrue(delete agf.caller, "Delete operation on 'caller' property returns true");

            assert.areEqual(0, agf.caller = 0, "Set operation on 'caller' creates new property with assigned value");
            assert.isTrue("caller" in agf, "Has operation on 'caller' property returns true now");
            assert.areEqual(0, agf.caller, "Get operation on 'caller' property returns property value now");
            checkAttributes("agf", agf, "caller", { writable: true, enumerable: true, configurable: true });
            assert.isTrue(delete agf.caller, "Delete operation on 'caller' property still returns true");
            assert.isFalse(agf.hasOwnProperty("caller"), "'caller' property is gone");

            async function* agfstrict() { "use strict"; }

            assert.isFalse(agfstrict.hasOwnProperty("arguments"), "Strict mode async function objects do not have an 'arguments' property");
            assert.isFalse(agfstrict.hasOwnProperty("caller"), "Strict mode async function objects do not have a 'caller' property");

            Object.setPrototypeOf(agfstrict, Object.prototype); // Remove Function.prototype so we don't find its 'caller' and 'arguments' in these operations
            assert.isFalse("arguments" in agfstrict, "Has operation on 'arguments' property returns false initially for a strict mode async function");
            assert.areEqual(undefined, agfstrict.arguments, "Get operation on 'arguments' property returns undefined initially for a strict mode async function");
            assert.areEqual(undefined, Object.getOwnPropertyDescriptor(agfstrict, "arguments"), "No property descriptor for 'arguments' initially for a strict mode async function");
            assert.isTrue(delete agfstrict.arguments, "Delete operation on 'arguments' property returns true initially for a strict mode async function");

            assert.areEqual(0, agfstrict.arguments = 0, "Set operation on 'arguments' creates new property with assigned value for a strict mode async function");
            assert.isTrue("arguments" in agfstrict, "Has operation on 'arguments' property returns true now for a strict mode async function");
            assert.areEqual(0, agfstrict.arguments, "Get operation on 'arguments' property returns property value now for a strict mode async function");
            checkAttributes("agfstrict", agfstrict, "arguments", { writable: true, enumerable: true, configurable: true });
            assert.isTrue(delete agfstrict.arguments, "Delete operation on 'arguments' property still returns true for a strict mode async function");
            assert.isFalse(agfstrict.hasOwnProperty("arguments"), "'arguments' property is gone for a strict mode async function");

            assert.isFalse("caller" in agfstrict, "Has operation on 'caller' property returns false initially for a strict mode async function");
            assert.areEqual(undefined, agfstrict.caller, "Get operation on 'caller' property returns undefined initially for a strict mode async function");
            assert.areEqual(undefined, Object.getOwnPropertyDescriptor(agfstrict, "caller"), "No property descriptor for 'caller' initially for a strict mode async function");
            assert.isTrue(delete agfstrict.caller, "Delete operation on 'caller' property returns true initially for a strict mode async function");

            assert.areEqual(0, agfstrict.caller = 0, "Set operation on 'caller' creates new property with assigned value for a strict mode async function");
            assert.isTrue("caller" in agfstrict, "Has operation on 'caller' property returns true now for a strict mode async function");
            assert.areEqual(0, agfstrict.caller, "Get operation on 'caller' property returns property value now for a strict mode async function");
            checkAttributes("agfstrict", agfstrict, "caller", { writable: true, enumerable: true, configurable: true });
            assert.isTrue(delete agfstrict.caller, "Delete operation on 'caller' property still returns true for a strict mode async function");
            assert.isFalse(agfstrict.hasOwnProperty("caller"), "'caller' property is gone for a strict mode async function");
        }
    },
    {
        name: "Async Generator function instances have %AsyncGeneratorFunctionPrototype% as their prototype and it has the correct properties and prototype",
        body: function () {
            async function* agf () {}
            const asyncGeneratorFunctionPrototype = Object.getPrototypeOf(agf);

            assert.areEqual(Function.prototype, Object.getPrototypeOf(asyncGeneratorFunctionPrototype), "%AsyncGeneratorFunctionPrototype%'s prototype is Function.prototype");

            assert.isTrue(asyncGeneratorFunctionPrototype.hasOwnProperty("constructor"), "%AsyncGeneratorFunctionPrototype% has 'constructor' property");
            assert.isTrue(asyncGeneratorFunctionPrototype.hasOwnProperty(Symbol.toStringTag), "%AsyncGeneratorFunctionPrototype% has [Symbol.toStringTag] property");

            checkAttributes("%AsyncGeneratorFunctionPrototype%", asyncGeneratorFunctionPrototype, "constructor", { writable: false, enumerable: false, configurable: true });
            checkAttributes("%AsyncGeneratorFunctionPrototype%", asyncGeneratorFunctionPrototype, Symbol.toStringTag, { writable: false, enumerable: false, configurable: true });

            assert.areEqual("AsyncGeneratorFunction", asyncGeneratorFunctionPrototype[Symbol.toStringTag], "%AsyncGeneratorFunctionPrototype%'s [Symbol.toStringTag] property is 'asyncGeneratorFunction'");

            assert.isTrue(asyncGeneratorFunctionPrototype.hasOwnProperty("prototype"), "%AsyncGeneratorFunctionPrototype% has a 'prototype' property");
        }
    },
    {
        name : "Async Generator function instances have the correct prototype object",
        body () {
            async function* agf () {}
            const prototype = agf.prototype;
            testMethod(prototype, "next", "AsyncGenerator instance prototype");
            testMethod(prototype, "throw", "AsyncGenerator instance prototype");
            testMethod(prototype, "return", "AsyncGenerator instance prototype")
            assert.isTrue('constructor' in prototype, "AsyncGenerator instance prototype has prototype propert");
            assert.areEqual('', prototype.constructor.name, "AsyncGenerator instance prototype constructor property is anonymous");
        }
    },
    {
        name: "%AsyncGeneratorFunction% constructor is the value of the constructor property of %AsyncGeneratorFunctionPrototype% and has the specified properties and prototype",
        body: function () {
            async function* agf () {}
            const asyncGeneratorFunctionPrototype = Object.getPrototypeOf(agf);
            const asyncGeneratorFunctionConstructor = asyncGeneratorFunctionPrototype.constructor;
            const asyncGeneratorPrototype = asyncGeneratorFunctionConstructor.prototype.prototype;

            assert.areEqual(Function, Object.getPrototypeOf(asyncGeneratorFunctionConstructor), "%AsyncGeneratorFunction%'s prototype is Function");

            assert.isTrue(asyncGeneratorFunctionConstructor.hasOwnProperty("name"), "%AsyncGeneratorFunction% has 'name' property");
            assert.isTrue(asyncGeneratorFunctionConstructor.hasOwnProperty("length"), "%AsyncGeneratorFunction% has 'length' property");
            assert.isTrue(asyncGeneratorFunctionConstructor.hasOwnProperty("prototype"), "%AsyncGeneratorFunction% has 'prototype' property");

            checkAttributes("%AsyncGeneratorFunction%", asyncGeneratorFunctionConstructor, "name", { writable: false, enumerable: false, configurable: true });
            checkAttributes("%AsyncGeneratorFunction%", asyncGeneratorFunctionConstructor, "length", { writable: false, enumerable: false, configurable: true });
            checkAttributes("%AsyncGeneratorFunction%", asyncGeneratorFunctionConstructor, "prototype", { writable: false, enumerable: false, configurable: false });

            testMethod(asyncGeneratorPrototype, "next", "%AsyncGeneratorPrototype%");
            testMethod(asyncGeneratorPrototype, "throw", "%AsyncGeneratorPrototype%");
            testMethod(asyncGeneratorPrototype, "return", "%AsyncGeneratorPrototype%");
            //print(asyncGeneratorPrototype.constructor);

            assert.areEqual("AsyncGeneratorFunction", asyncGeneratorFunctionConstructor.name, "%AsyncGeneratorFunction%'s 'name' property is 'AsyncGeneratorFunction'");
            assert.areEqual(asyncGeneratorFunctionPrototype, asyncGeneratorFunctionConstructor.prototype, "%AsyncGeneratorFunction%'s 'prototype' property is %AsyncGeneratorFunction%.prototype");
            assert.areEqual(1, asyncGeneratorFunctionConstructor.length, "%AsyncGeneratorFunction%'s 'length' property is 1");
        }
    },
    {
        name : "%AsyncGenerator% object shape",
        body () {
            async function* agf () {}
            const asyncGenerator = agf();
            testMethod(asyncGenerator, "next", "%AsyncGeneratorPrototype%");
            testMethod(asyncGenerator, "throw", "%AsyncGeneratorPrototype%");
            testMethod(asyncGenerator, "return", "%AsyncGeneratorPrototype%");
        }
    },
    {
        name: "Anonymous Async Generator function",
        body: function () {
            const asyncGeneratorFunctionPrototype = Object.getPrototypeOf(async function* () { });
            const asyncGeneratorFunction = asyncGeneratorFunctionPrototype.constructor;

            let agf = new asyncGeneratorFunction('return await 1;');

            assert.areEqual(asyncGeneratorFunctionPrototype, Object.getPrototypeOf(agf), "Async function created by %AsyncGeneratorFunction% should have the same prototype as syntax declared async functions");

            assert.areEqual("anonymous", agf.name, "asyncGeneratorFunction constructed async function's name is 'anonymous'");
            assert.areEqual("async function* anonymous(\n) {return await 1;\n}", agf.toString(), "toString of asyncGeneratorFunction constructed function is named 'anonymous'");

            agf = new asyncGeneratorFunction('a', 'b', 'c', 'await a; await b; await c;');
            assert.areEqual("async function* anonymous(a,b,c\n) {await a; await b; await c;\n}", agf.toString(), "toString of asyncGeneratorFunction constructed function is named 'anonymous' with specified parameters");
        }
    },
    {
        name: "Other forms of Async Generator",
        body: function () {
            const obj = {
                async *oagf() {}
            }
            class cla {
                async *cagf() {}
                static async *scagf() {}
            }

            const instance = new cla();

            const asyncGeneratorFunctionPrototype = Object.getPrototypeOf(async function* () { });

            assert.areEqual(asyncGeneratorFunctionPrototype, Object.getPrototypeOf(instance.cagf), "Async generator class method should have the same prototype as async generator function");
            assert.areEqual(asyncGeneratorFunctionPrototype, Object.getPrototypeOf(cla.scagf), "Async generator class static method should have the same prototype as async generator function");
            assert.areEqual(asyncGeneratorFunctionPrototype, Object.getPrototypeOf(obj.oagf), "Async generator object method should have the same prototype as async generator function");
        }
    },
    {
        name: "Attempt to use Generator methods with AsyncGenerator",
        body() {
            function* gf () {}
            async function* agf() {}

            const ag = agf();
            const g = gf();

            assert.throws(()=>{g.next.call(ag)}, TypeError, "Generator.prototype.next should throw TypeError when called on an Async Generator");
            assert.throws(()=>{g.throw.call(ag)}, TypeError, "Generator.prototype.throw should throw TypeError when called on an Async Generator");
            assert.throws(()=>{g.return.call(ag)}, TypeError, "Generator.prototype.return should throw TypeError when called on an Async Generator");
        }
    }
];


// Cannot easily verify behavior of async generator functions in conjunction with UnitTestFramework.js
// due to callback nature of their execution. Instead, verification of behavior is
// found in async-generator-functionality.js test.

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
