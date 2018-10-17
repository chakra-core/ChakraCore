//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

//setup code
let constructionCount = 0;
let functionCallCount = 0;
var classConstructorCount = 0; // var not let, so it can be accessed from other context
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

const crossContext = WScript.LoadScript("\
class A {                                \
    constructor(arg1, arg2) {            \
        this[arg1] = arg2;               \
        this.localVal = this.protoVal;   \
        ++other.classConstructorCount;   \
    }                                    \
}                                        \
A.prototype.protoVal = 2;                \
this.A2 = A;                             \
", "samethread");
crossContext.other = this;

const trapped = new Proxy(a, {
    construct: function (x, y, z) {
        ++constructionCount;
        return Reflect.construct(x, y, z);
    }
});

const trappedClass = new Proxy(A, {
    construct: function (x, y, z) {
        ++constructionCount;
        return Reflect.construct(x, y, z);
    }
});

const evalTrappedClass = new Proxy(A, {
    construct: function (x, y, z) {
        return eval("++constructionCount; Reflect.construct(x, y, z);");
    }
});

const withTrappedClass = new Proxy(A, {
    construct: function (x, y, z) {
        with (Reflect) {
            ++constructionCount;
            return construct(x, y, z);
        }
    }
});

const noTrap = new Proxy(a, {});
const noTrapClass = new Proxy(A, {});
const noTrapClassCrossContext = new Proxy(crossContext.A2, {});

const boundObject = {};

const boundFunc = a.bind(boundObject, "prop-name");
boundFunc.prototype = {}; // so we can extend from it
const boundClass = A.bind(boundObject, "prop-name");
boundClass.prototype = {};

const boundTrapped = trapped.bind(boundObject, "prop-name");
const boundUnTrapped = noTrap.bind(boundObject, "prop-name");
const boundTrappedClass = trappedClass.bind(boundObject, "prop-name");
const boundUnTrappedClass = noTrapClass.bind(boundObject, "prop-name");
const boundUnTrappedClassCrossContext = noTrapClassCrossContext.bind(boundObject, "prop-name");

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

// flags
const ConstructionMode = {
    useReflect: 1,
    useEval: 2,
    useWith: 4,
};

function testImpl(ctor, constructionMode, expectedPrototype, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount) {
    const useReflect = (constructionMode & 1) === 1;
    constructionCount = 0;
    functionCallCount = 0;
    classConstructorCount = 0;
    let obj;
    switch (constructionMode) {
        case 0:
            obj = new ctor("prop-value");
            break;
        case 1:
            obj = Reflect.construct(ctor, ["prop-value"], newTarget);
            break;
        case 2:
            eval('obj = new ctor("prop-value");');
            break;
        case 3:
            eval('obj = Reflect.construct(ctor, ["prop-value"], newTarget);');
            break;
        case 4:
            with ({}) { obj = new ctor("prop-value"); }
            break;
        case 5:
            with (Reflect) { obj = construct(ctor, ["prop-value"], newTarget); }
            break;
        case 6:
            eval('with ({}) { obj = new ctor("prop-value"); }');
            break;
        case 7:
            eval('with (Reflect) { obj = construct(ctor, ["prop-value"], newTarget); }');
            break;
        default:
            throw new Error("unrecognized mode");
    }
    assert.areNotEqual(boundObject, obj, "bound function should ignore bound this when constructing");
    assert.areEqual("prop-value", obj["prop-name"], "bound function should keep bound arguments when constructing");
    assert.areEqual(expectedConstructionCount, constructionCount, `proxy construct trap should be called ${expectedConstructionCount} times`);
    assert.areEqual(expectedFunctionCallCount, functionCallCount, `base function-style constructor should be called ${expectedFunctionCallCount} times`);
    assert.areEqual(expectedClassConstructorCount, classConstructorCount, `base class constructor should be called ${expectedClassConstructorCount} times`);
    assert.strictEqual(expectedPrototype.prototype, obj.__proto__, useReflect ? "bound function should use explicit newTarget if provided" : "constructed object should be instance of original function");
    assert.areEqual(expectedPrototype.prototype.protoVal, obj.localVal, "prototype should be available during construction");
}

function test(ctor, expectedPrototype, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount) {
    testImpl(ctor, 0, expectedPrototype, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount);
    testImpl(ctor, 1, newTarget, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount);
    testImpl(ctor, 2, expectedPrototype, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount);
    testImpl(ctor, 3, newTarget, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount);
    testImpl(ctor, 4, expectedPrototype, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount);
    testImpl(ctor, 5, newTarget, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount);
    testImpl(ctor, 6, expectedPrototype, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount);
    testImpl(ctor, 7, newTarget, expectedConstructionCount, expectedFunctionCallCount, expectedClassConstructorCount);
}

const tests = [
    {
        name : "Construct trapped bound proxy around function",
        body : function() {
            test(boundTrapped, a, 1, 1, 0);
        }
    },
    {
        name : "Construct bound proxy around function",
        body : function() {
            test(boundUnTrapped, a, 0, 1, 0);
        }
    },
    {
        name : "Construct trapped bound proxy around class",
        body : function() {
            test(boundTrappedClass, A, 1, 0, 1);
        }
    },
    {
        name : "Construct bound proxy around class",
        body : function() {
            test(boundUnTrappedClass, A, 0, 0, 1);
        }
    },
    {
        name : "Construct bound proxy around class with cross-context construction",
        body : function() {
            test(boundUnTrappedClassCrossContext, crossContext.A2, 0, 0, 1);
        }
    },
    {
        name: "Trapped bound proxy around class using eval",
        body: function () {
            test(evalTrappedClass.bind(boundObject, "prop-name"), A, 1, 0, 1);
        }
    },
    {
        name: "Trapped bound proxy around class using with",
        body: function () {
            test(withTrappedClass.bind(boundObject, "prop-name"), A, 1, 0, 1);
        }
    },
    {
        name : "Construct bound function",
        body : function() {
            test(boundFunc, a, 0, 1, 0);
        }
    },
    {
        name : "Construct bound class",
        body : function() {
            test(boundClass, A, 0, 0, 1);
        }
    },
    {
        name : "Construct class extending bound function",
        body : function() {
            test(ExtendsBoundFunc, ExtendsBoundFunc, 0, 1, 0);
        }
    },
    {
        name : "Construct class extending bound class",
        body : function() {
            test(ExtendsBoundClass, ExtendsBoundClass, 0, 0, 1);
        }
    },
    {
        name : "Construct bound class that extends a function",
        body : function() {
            test(boundClassExtendsFunc, ExtendsFunc, 0, 1, 0);
        }
    },
    {
        name : "Construct bound class that extends another class",
        body : function() {
            test(boundClassExtendsClass, ExtendsClass, 0, 0, 1);
        }
    },
    {
        name : "Construct bound proxy around proxy",
        body : function() {
            const baseProxies = [
                { proxy: trapped, func: true, trap: true },
                { proxy: trappedClass, func: false, trap: true },
                { proxy: noTrap, func: true, trap: false },
                { proxy: noTrapClass, func: false, trap: false }
            ];

            for (const { proxy, func, trap } of baseProxies) {
                const p1 = new Proxy(proxy, {});
                const p2 = new Proxy(proxy, {
                    construct: function (x, y, z) {
                        ++constructionCount;
                        return Reflect.construct(x, y, z);
                    }
                });
                const b1 = p1.bind(boundObject, "prop-name");
                const b2 = p2.bind(boundObject, "prop-name");
                test(b1, func ? a : A, trap ? 1 : 0, func ? 1 : 0, func ? 0 : 1);
                test(b2, func ? a : A, 1 + (trap ? 1 : 0), func ? 1 : 0, func ? 0 : 1);
            }
        }
    },
    {
        name: "Construct built-in class",
        body: function () {
            const bound = Boolean.bind(boundObject, false);
            const noTrap = new Proxy(Boolean, {});
            const trap = new Proxy(Boolean, {
                construct: function (x, y, z) {
                    return Reflect.construct(x, y, z);
                }
            });

            function verify(obj, expectedNewTarget) {
                assert.isTrue(Boolean.prototype.valueOf.call(obj) === false, "Boolean should represent value false");
                assert.isFalse(obj === false, "Boolean is not a value type");
                assert.strictEqual(expectedNewTarget.prototype, obj.__proto__, "Object should get constructed with appropriate prototype");
            }

            verify(new bound(true), Boolean);
            verify(Reflect.construct(bound, [true], newTarget), newTarget);
            verify(new noTrap(false), Boolean);
            verify(eval("new noTrap(false)"), Boolean);
            verify(Reflect.construct(noTrap, [false], newTarget), newTarget);
            verify(new trap(false), Boolean);
            verify(Reflect.construct(trap, [false], newTarget), newTarget);
            verify(eval("Reflect.construct(trap, [false], newTarget)"), newTarget);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
