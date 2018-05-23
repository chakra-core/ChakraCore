//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

//setup code
let constructionCount = 0;
let functionCallCount = 0;
let classConstructorCount = 0;
function a(arg1, arg2) {
    this[arg1] = arg2;
    this.localVal = this.protoVal;
    ++functionCallCount;
}
a.prototype.protoVal = 1;

class A {
    constructor(arg1, arg2) {
        this[arg1] = arg2;
        this.localVal = this.protoVal;
        ++classConstructorCount;
    }
}
A.prototype.protoVal = 2;

const trapped = new Proxy(a, {
    construct: function (x, y, z) {
        ++constructionCount;
        return Reflect.construct(x, y, z);
    }
});

const noTrap = new Proxy(a, {});

const boundObject = {};

const boundFunc = a.bind(boundObject, "prop-name");
boundFunc.prototype = {}; // so we can extend from it
const boundClass = A.bind(boundObject, "prop-name");
boundClass.prototype = {};

const boundTrapped = trapped.bind(boundObject, "prop-name");
const boundUnTrapped = noTrap.bind(boundObject, "prop-name");

class newTarget {}
newTarget.prototype.protoVal = 3;

class ExtendsBoundFunc extends boundFunc {}
ExtendsBoundFunc.prototype.protoVal = 4;
class ExtendsBoundClass extends boundClass {}
ExtendsBoundClass.prototype.protoVal = 5;

class ExtendsFunc extends a {}
ExtendsFunc.prototype.protoVal = 6;
class ExtendsClass extends A {}
ExtendsClass.prototype.protoVal = 7;

const boundClassExtendsFunc = ExtendsFunc.bind(boundObject, "prop-name");
const boundClassExtendsClass = ExtendsClass.bind(boundObject, "prop-name");

function test(ctor, useReflect, expectedPrototype, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount) {
    constructionCount = 0;
    functionCallCount = 0;
    classConstructorCount = 0;
    const obj = useReflect ? Reflect.construct(ctor, ["prop-value"], newTarget) : new ctor("prop-value");
    assert.areNotEqual(boundObject, obj, "bound function should ignore bound this when constructing");
    assert.areEqual("prop-value", obj["prop-name"], "bound function should keep bound arguments when constructing");
    assert.areEqual(expectedConstructionCount, constructionCount, `proxy construct trap should be called ${expectedConstructionCount} times`);
    assert.areEqual(expectedFunctionCallCount, functionCallCount, `base function-style constructor should be called ${expectedFunctionCallCount} times`);
    assert.areEqual(expectedClassConstructorCount, classConstructorCount, `base class constructor should be called ${expectedClassConstructorCount} times`);
    assert.strictEqual(expectedPrototype.prototype, obj.__proto__, useReflect ? "bound function should use explicit newTarget if provided" : "constructed object should be instance of original function");
    assert.areEqual(expectedPrototype.prototype.protoVal, obj.localVal, "prototype should be available during construction");
}

const tests = [
    {
        name : "Construct trapped bound proxy with new",
        body : function() {
            test(boundTrapped, false, a, 1, 1, 0);
        }
    },
    {
        name : "Construct trapped bound proxy with Reflect",
        body : function() {
            test(boundTrapped, true, newTarget, 1, 1, 0);
        }
    },
    {
        name : "Construct bound proxy with new",
        body : function() {
            test(boundUnTrapped, false, a, 0, 1, 0);
        }
    },
    {
        name : "Construct bound proxy with Reflect",
        body : function() {
            test(boundUnTrapped, true, newTarget, 0, 1, 0);
        }
    },
    {
        name : "Construct bound function with Reflect",
        body : function() {
            test(boundFunc, true, newTarget, 0, 1, 0);
        }
    },
    {
        name : "Construct bound function with new",
        body : function() {
            test(boundFunc, false, a, 0, 1, 0);
        }
    },
    {
        name : "Construct bound class with new",
        body : function() {
            test(boundClass, false, A, 0, 0, 1);
        }
    },
    {
        name : "Construct bound class with Reflect",
        body : function() {
            test(boundClass, true, newTarget, 0, 0, 1);
        }
    },
    {
        name : "Construct class extending bound function with new",
        body : function() {
            test(ExtendsBoundFunc, false, ExtendsBoundFunc, 0, 1, 0);
        }
    },
    {
        name : "Construct class extending bound function with Reflect",
        body : function() {
            test(ExtendsBoundFunc, true, newTarget, 0, 1, 0);
        }
    },
    {
        name : "Construct class extending bound class with new",
        body : function() {
            test(ExtendsBoundClass, false, ExtendsBoundClass, 0, 0, 1);
        }
    },
    {
        name : "Construct class extending bound class with Reflect",
        body : function() {
            test(ExtendsBoundClass, true, newTarget, 0, 0, 1);
        }
    },
    {
        name : "Construct bound class that extends a function with new",
        body : function() {
            test(boundClassExtendsFunc, false, ExtendsFunc, 0, 1, 0);
        }
    },
    {
        name : "Construct bound class that extends a function with Reflect",
        body : function() {
            test(boundClassExtendsFunc, true, newTarget, 0, 1, 0);
        }
    },
    {
        name : "Construct bound class that extends another class with new",
        body : function() {
            test(boundClassExtendsClass, false, ExtendsClass, 0, 0, 1);
        }
    },
    {
        name : "Construct bound class that extends another class with Reflect",
        body : function() {
            test(boundClassExtendsClass, true, newTarget, 0, 0, 1);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
