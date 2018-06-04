//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Object Spread unit tests

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

function verifyPropertyDesc(obj, prop, desc, propName) {
    var actualDesc = Object.getOwnPropertyDescriptor(obj, prop);
    if (typeof propName === "undefined") { propName = prop; }
    assert.areEqual(desc.configurable, actualDesc.configurable, propName+"'s attribute: configurable");
    assert.areEqual(desc.enumerable, actualDesc.enumerable, propName+"'s attribute: enumerable");
    assert.areEqual(desc.writable, actualDesc.writable, propName+"'s attribute: writable");
}

var a = {i: 1, j: 2};
var b = {x: 3, y: 4, z: 5};
var c = {foo: 6};

var tests = [
    {
        name: "Test shallow cloning",
        body: function() {
            let aClone = {...a};
            assert.areEqual(1, aClone.i);
            assert.areEqual(2, aClone.j);
            assert.areEqual(2, Object.keys(aClone).length);
        }
    },
    {
        name: "Test Spread Object in parens",
        body: function() {
            let aClone = {...(a)};
            assert.areEqual(1, aClone.i);
            assert.areEqual(2, aClone.j);
            assert.areEqual(2, Object.keys(aClone).length);
        }
    },
    {
        name: "Test merging 2 objects",
        body: function() {
            let merged = {...a, ...b};
            assert.areEqual(1, merged.i);
            assert.areEqual(2, merged.j);
            assert.areEqual(3, merged.x);
            assert.areEqual(4, merged.y);
            assert.areEqual(5, merged.z);
            assert.areEqual(5, Object.keys(merged).length);
        }
    },
    {
        name: "Test merging default properties with another object",
        body: function() {
            let merged = {i: 1, ...c};
            assert.areEqual(1, merged.i);
            assert.areEqual(6, merged.foo);
            assert.areEqual(2, Object.keys(merged).length);
            assert.areEqual(1, Object.keys(c).length);

            // Order should not matter in this case
            merged = {...c, i: 1};
            assert.areEqual(1, merged.i);
            assert.areEqual(6, merged.foo);
            assert.areEqual(2, Object.keys(merged).length);
            assert.areEqual(1, Object.keys(c).length);
        }
    },
    {
        name: "Test overrides",
        body: function() {
            let over = {i: 10, j: 11, ...a};
            assert.areEqual(1, over.i);
            assert.areEqual(2, over.j);
            assert.areEqual(2, Object.keys(over).length);

            over = {...a, i: 10, j: 11};
            assert.areEqual(10, over.i);
            assert.areEqual(11, over.j);
            assert.areEqual(2, Object.keys(over).length);

            over = {...a, ...{i: 10, j: 11}};
            assert.areEqual(10, over.i);
            assert.areEqual(11, over.j);
            assert.areEqual(2, Object.keys(over).length);

            let i = 10, j = 11;
            over = {...a, i, j};
            assert.areEqual(10, over.i);
            assert.areEqual(11, over.j);
            assert.areEqual(2, Object.keys(over).length);
        }
    },
    {
        name: "Getters in Object Literal should not be evaluated",
        body: function() {
            let getterExecuted = false;
            let objWithGetter = {get i() {getterExecuted = true;}, ...c};
            assert.areEqual(6, objWithGetter.foo);
            assert.isFalse(getterExecuted);
            assert.areEqual(2, Object.keys(objWithGetter).length);
            assert.isTrue(objWithGetter.hasOwnProperty("i"));
        }
    },
    {
        name: "Getters in Spread Object should be evaluated",
        body: function() {
            let getterExecuted = false;
            let obj = {i: 1, ...{get j() {getterExecuted = true; return 2;}}};
            assert.areEqual(1, obj.i);
            assert.isTrue(getterExecuted);
            assert.areEqual(2, obj.j);
            assert.areEqual(2, Object.keys(obj).length);
        }
    },
    {
        name: "Test Spread Object modifying itself",
        body: function() {
            let val = 1;
            let source = {get i() {val++; return 1;}, get j() {return val;}};
            let obj = {...source};
            assert.areEqual(1, obj.i);
            assert.areEqual(2, obj.j);
            assert.areEqual(2, Object.keys(obj).length);
        }
    },
    {
        name: "Test Spread Object modified by other Spread Object",
        body: function() {
            let a = {i: 1};
            let b = {get j() {a.i = 3; return 2;}};
            let obj = {...b, ...a};
            assert.areEqual(3, obj.i);
            assert.areEqual(2, obj.j);
            assert.areEqual(2, Object.keys(obj).length);
        }
    },
    {
        name: "Test multiple merges of same object",
        body: function() {
            let getterExecutions = 0;
            let objWithGetter = {get i() {getterExecutions++; return 1;}};
            let merged = {a: 2, ...objWithGetter, b: 3, ...objWithGetter};
            assert.areEqual(2, merged.a);
            assert.areEqual(3, merged.b);
            assert.areEqual(1, merged.i);
            assert.areEqual(3, Object.keys(merged).length);
            assert.areEqual(2, getterExecutions, "Getters should be executed twice, once for each `...`");
        }
    },
    {
        name: "Setters should not be called in Object Literal",
        body: function() {
            let setterExecuted = false;
            let objWithSetter = {set foo(v) {setterExecuted = true;}, ...c};
            assert.areEqual(6, objWithSetter.foo);
            assert.areEqual(1, Object.keys(objWithSetter).length);
            assert.isFalse(setterExecuted);
        }
    },
    {
        name: "Null and Undefined should be ignored",
        body: function() {
            let empty = {...null};
            assert.areEqual(0, Object.keys(empty).length);

            empty = {...undefined};
            assert.areEqual(0, Object.keys(empty).length);
        }
    },
    {
        name: "Test nesting",
        body: function() {
            let base = {name: "foo", prev: {}, num: 5};
            let derived = {...base, name: "bar", prev: {...base}};
            assert.areEqual("foo", base.name);
            assert.areEqual({}, base.prev);
            assert.areEqual(5, base.num);
            assert.areEqual(3, Object.keys(base).length);
            assert.areEqual("bar", derived.name);
            assert.areEqual("foo", derived.prev.name);
            assert.areEqual({}, derived.prev.prev);
            assert.areEqual(5, derived.prev.num);
            assert.areEqual(5, derived.num);
            assert.areEqual(3, Object.keys(derived).length);
        }
    },
    {
        name: "Test Spread with computed property names in Object Literal",
        body: function() {
            let obj = {[5]: 5, ["bar"]: 2, ...c};
            assert.areEqual(5, obj[5]);
            assert.areEqual(2, obj.bar);
            assert.areEqual(6, obj.foo);
            assert.areEqual(3, Object.keys(obj).length);
        }
    },
    {
        name: "Test Spread with functions in Object Literal",
        body: function() {
            let obj = {func() {return true;}, ...c};
            assert.areEqual(6, obj.foo);
            assert.isTrue(obj.hasOwnProperty("func"));
            assert.areEqual(2, Object.keys(obj).length);
        }
    },
    {
        name: "Property Descriptors from Spread should be default",
        body: function() {
            let obj = {...c};
            let defaultDesc = {
                configurable: true,
                enumerable: true,
                writable: true
            };
            verifyPropertyDesc(obj, "foo", defaultDesc);
        }
    },
    {
        name: "Copy only own properties",
        body: function() {
            let parent = {i: 1, j: 2};
            let child = Object.create(parent);
            child.i = 3;
            let obj = {...child};

            assert.areEqual(3, child.i);
            assert.areEqual(2, child.j);
            assert.areEqual(3, obj.i);
            assert.isFalse(obj.hasOwnProperty("j"));
        }
    },
    {
        name: "Spread includes symbols in properties",
        body: function() {
            let sym = Symbol("foo");
            let a = {};
            a[sym] = 1;
            let obj = {...a};
            assert.areEqual(1, obj[sym], "property with Symbol property name identifier should be copied over");
            assert.areEqual(1, Object.getOwnPropertySymbols(obj).length);
        }
    },
    {
        name: "Spread after assignment",
        body: function() {
            let temp = {};
            let obj = {...temp=a};
            assert.areEqual(2, Object.keys(obj).length);
            assert.areEqual(1, obj.i);
            assert.areEqual(2, obj.j);

            obj = {...temp=1};
            assert.areEqual(0, Object.keys(obj).length);
        }
    },
    {
        name: "Object Spread interacting with Array Spread",
        body: function() {
            let arr = [1, 2];
            let obj = {...[...arr, 3]};
            assert.areEqual(3, Object.keys(obj).length);
            assert.areEqual(1, obj[0]);
            assert.areEqual(2, obj[1]);
            assert.areEqual(3, obj[2]);
        }
    },
    {
        name: "Object Spread interacting with Numbers",
        body: function() {
            let obj = {...1}
            assert.areEqual(0, Object.keys(obj).length);
        }
    },
    {
        name: "Object Spread interacting with Functions",
        body: function() {
            let obj = {...function i() {return 1;}}
            assert.areEqual(0, Object.keys(obj).length);
        }
    },
    {
        name: "Object Spread interacting with Strings",
        body: function() {
            let obj = {..."edge"};
            assert.areEqual(4, Object.keys(obj).length);
            assert.areEqual("e", obj[0]);
            assert.areEqual("d", obj[1]);
            assert.areEqual("g", obj[2]);
            assert.areEqual("e", obj[3]);
        }
    },
    {
        name: "Test Proxy Object",
        body: function() {
            let proxy = new Proxy({i: 1, j: 2}, {});
            let obj = {...proxy};
            assert.areEqual(2, Object.keys(obj).length);
            assert.areEqual(1, obj.i);
            assert.areEqual(2, obj.j);
        }
    },
    {
        name: "Test Proxy Object with custom getter",
        body: function() {
            let handler = {get: function(obj, prop) {return obj[prop];}};
            let proxy = new Proxy({i: 1, j: 2}, handler);
            let obj = {...proxy};
            assert.areEqual(2, Object.keys(obj).length);
            assert.areEqual(1, obj.i);
            assert.areEqual(2, obj.j);
        }
    },
    {
        name: "Test Proxy Object with custom getter and setter",
        body: function() {
            let setterCalled = false;
            let handler = {
                get: function(obj, prop) {
                    return obj[prop];
                },
                set: function(obj, prop, value) {
                    setterCalled = true;
                }
            };
            let proxy = new Proxy({i: 1, j: 2}, handler);
            let obj = {...proxy};
            assert.areEqual(2, Object.keys(obj).length);
            assert.areEqual(1, obj.i);
            assert.areEqual(2, obj.j);
            assert.isFalse(setterCalled);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
