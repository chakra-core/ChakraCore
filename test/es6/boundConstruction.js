//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

//setup code
let constructionCount = 0;
let functionCallCount = 0;
function a(arg1, arg2) {
    this[arg1] = arg2;
    ++functionCallCount;
}

const trapped = new Proxy(a, {
    construct: function (x, y, z) {
        ++constructionCount;
        return Reflect.construct(x, y, z);
    }
});

const noTrap = new Proxy(a, {});

const boundObject = {};

const boundFunc = a.bind(boundObject, "prop-name");

const boundTrapped = trapped.bind(boundObject, "prop-name");
const boundUnTrapped = noTrap.bind(boundObject, "prop-name");

const tests = [
    {
        name : "Construct trapped bound proxy with new",
        body : function() {
            constructionCount = 0;
            functionCallCount = 0;
            const obj = new boundTrapped("prop-value");
            assert.areNotEqual(boundObject, obj, "bound function should ignore bound this when constructing");
            assert.areEqual("prop-value", obj["prop-name"], "bound function should keep bound arguments when constructing");
            assert.areEqual(1, constructionCount, "bound proxy should be constructed once");
            assert.areEqual(1, functionCallCount, "bound proxy function should be called once");
            assert.strictEqual(a.prototype, obj.__proto__, "constructed object should be instance of original function");
        }
    },
    {
        name : "Construct trapped bound proxy with Reflect",
        body : function() {
            class newTarget {}
            constructionCount = 0;
            functionCallCount = 0;
            const obj = Reflect.construct(boundTrapped, ["prop-value-2"], newTarget);
            assert.areNotEqual(boundObject, obj, "bound function should ignore bound this when constructing");
            assert.strictEqual(newTarget.prototype, obj.__proto__, "bound function should use explicit newTarget if provided");
            assert.areEqual("prop-value-2", obj["prop-name"], "bound function should keep bound arguments when constructing");
            assert.areEqual(1, constructionCount, "bound proxy should be constructed once");
            assert.areEqual(1, functionCallCount, "bound proxy function should be called once");
        }
    },
    {
        name : "Construct bound proxy with new",
        body : function() {
            functionCallCount = 0;
            const obj = new boundUnTrapped("prop-value");
            assert.areNotEqual(boundObject, obj, "bound function should ignore bound this when constructing");
            assert.areEqual("prop-value", obj["prop-name"], "bound function should keep bound arguments when constructing");
            assert.areEqual(1, functionCallCount, "bound proxy function should be called once");
            assert.strictEqual(a.prototype, obj.__proto__, "constructed object should be instance of original function");
        }
    },
    {
        name : "Construct bound proxy with Reflect",
        body : function() {
            class newTarget {}
            functionCallCount = 0;
            const obj = Reflect.construct(boundUnTrapped, ["prop-value-2"], newTarget);
            assert.areNotEqual(boundObject, obj, "bound function should ignore bound this when constructing");
            assert.strictEqual(newTarget.prototype, obj.__proto__, "bound function should use explicit newTarget if provided");
            assert.areEqual("prop-value-2", obj["prop-name"], "bound function should keep bound arguments when constructing");
            assert.areEqual(1, functionCallCount, "bound proxy function should be called once");
        }
    },
    {
        name : "Construct bound function with Reflect",
        body : function() {
            class newTarget {}
            functionCallCount = 0;
            const obj = Reflect.construct(boundFunc, ["prop-value-2"], newTarget);
            assert.areNotEqual(boundObject, obj, "bound function should ignore bound this when constructing");
            assert.strictEqual(newTarget.prototype, obj.__proto__, "bound function should use explicit newTarget if provided");
            assert.areEqual("prop-value-2", obj["prop-name"], "bound function should keep bound arguments when constructing");
            assert.areEqual(1, functionCallCount, "bound function should be called once");
        }
    },
    {
        name : "Construct bound function with new",
        body : function() {
            functionCallCount = 0;
            const obj = new boundFunc("prop-value-2");
            assert.areNotEqual(boundObject, obj, "bound function should ignore bound this when constructing");
            assert.strictEqual(a.prototype, obj.__proto__, "bound function should use explicit newTarget if provided");
            assert.areEqual("prop-value-2", obj["prop-name"], "bound function should keep bound arguments when constructing");
            assert.areEqual(1, functionCallCount, "bound function should be called once");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" }); 
