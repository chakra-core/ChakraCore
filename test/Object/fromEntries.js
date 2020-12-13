//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function verifyProperties(obj, property, value)
{
    const descriptor = Object.getOwnPropertyDescriptor(obj, property);
    assert.areEqual(value, obj[property], "Object.fromEntries should set correct valued");
    assert.isTrue(descriptor.enumerable, "Object.fromEntries should create enumerable properties");
    assert.isTrue(descriptor.configurable, "Object.fromEntries should create configurable properties");
    assert.isTrue(descriptor.writable, "Object.fromEntries should create writable properties");
    obj[property] = "other value";
    assert.areEqual("other value", obj[property], "should actually be able to write to properties created by Object.fromEntries");
    assert.doesNotThrow(()=>{"use strict"; delete obj[property];}, "deleting properties created by Object.fromEntries should not throw");
    assert.isUndefined(obj[property], "deleting properties created by Object.fromEntries should succeed");
}

function verifyObject(expected, actual)
{
    for (let i in actual)
    {
        assert.isTrue(expected.hasOwnProperty(i), "Object.fromEntries shouldn't create unexpected properties");
    }
    for (let i in expected)
    {
        verifyProperties(actual, i, expected[i]);
    }
}


const tests = [
    {
        name : "Object.fromEntries invalid parameters",
        body : function () {
            assert.throws(()=>{Object.fromEntries(null);}, TypeError, "Object.fromEntries throws when called with null parameter");
            assert.throws(()=>{Object.fromEntries(undefined);}, TypeError, "Object.fromEntries throws when called with undefined parameter");
            assert.throws(()=>{Object.fromEntries("something");}, TypeError, "Object.fromEntries throws when called with string literal parameter");
            assert.throws(()=>{Object.fromEntries(456);}, TypeError, "Object.fromEntries throws when called with number literal parameter");
            assert.throws(()=>{Object.fromEntries(Number());}, TypeError, "Object.fromEntries throws when called with Number Object parameter");
            assert.doesNotThrow(()=>{Object.fromEntries(String());}, "Object.fromEntries does not throw when called with String Object parameter with length 0");
            assert.throws(()=>{Object.fromEntries(String("anything"));}, TypeError, "Object.fromEntries throws when called with String Object parameter with length > 0");
            assert.throws(()=>{Object.fromEntries({});}, TypeError, "Object.fromEntries throws when called with Object literal");
            assert.throws(()=>{Object.fromEntries({a : "5", b : "10"});}, TypeError, "Object.fromEntries throws when called with Object literal");
        }
    },
    {
        name : "Object.fromEntries basic cases",
        body : function () {
            const obj1 = Object.fromEntries([["first", 50], ["second", 30], ["third", 60], ["fourth", 70]]);
            verifyObject({first : 50, second : 30, third : 60, fourth : 70}, obj1);
            const obj2 =  Object.fromEntries([Object("a12234"),Object("b2kls"),Object("c3deg")]);
            verifyObject({a : "1", b : "2", c : "3"}, obj2);
            function testArguments()
            {
                verifyObject(expected, Object.fromEntries(arguments));
            }
            const expected = { abc : "one", bcd : "two", hsa : "three"};
            testArguments(["abc", "one"], ["bcd", "two"], ["hsa", "three"]);
        }
    },
    {
        name : "Object.fromEntries with array-like object",
        body : function ()
        {
            const arrayLike = {0 : ["abc", "one"], 1 : ["bcd", "two"], 2 : ["hsa", "three"], length : 3, current : 0}
            assert.throws(()=>{ Object.fromEntries(arrayLike); }, TypeError, "Object.fromEntries throws when parameter has no iterator");
            arrayLike[Symbol.iterator] = function () {
                const array = this;
                return  {
                    next : function () {
                        const value = array[String(array.current)];
                        ++array.current;
                        return {
                            value : value,
                            done : array.length < array.current
                        };
                    }
                };
            };
            verifyObject({ abc : "one", bcd : "two", hsa : "three"}, Object.fromEntries(arrayLike));
        }
    },
    {
        name : "Object.fromEntries does not call setters",
        body : function () {
            let calledSet = false;
            Object.defineProperty(Object.prototype, "prop", {
                set : function () { calledSet = true; }
            });
            const obj = Object.fromEntries([["prop", 10]]);
            verifyProperties(obj, "prop", 10);
            assert.isFalse(calledSet, "Object.fromEntries should not call setters");
        }
    },
    {
        name : "Object.fromEntries iterates over generators",
        body : function () {
            function* gen1 ()
            {
                yield ["val1", 10];
                yield ["val2", 50];
                yield ["val3", 60, "other stuff"];
            }
            const obj = Object.fromEntries(gen1());
            verifyObject({val1 : 10, val2 : 50, val3: 60}, obj);
            let unreachable = false;
            function* gen2 ()
            {
                yield ["val1", 10];
                yield "val2";
                unreachable = true;
                yield ["val3", 60, "other stuff"];
            }
            assert.throws(()=>{Object.fromEntries(gen2())}, TypeError, "When generator provides invalid case Object.fromEntries should throw");
            assert.isFalse(unreachable, "Object.fromEntries does not continue after invalid case provided");
        }
    },
    {
        name : "Object.fromEntries accesses properties in correct order from generator",
        body : function () {
            const accessedProps = [];
            const handler = {
                get : function (target, prop, receiver) {
                    accessedProps.push(prop + Reflect.get(target, prop));
                    return Reflect.get(target, prop);
                }
            }

            function* gen () {
                yield new Proxy(["a", "b", "c"], handler);
                yield new Proxy(["e", "g", "h", "j"], handler);
            }
            const obj = Object.fromEntries(gen());
            verifyObject({a : "b", e : "g"}, obj);
            const expected = ["0a", "1b", "0e", "1g"];
            const len = accessedProps.length;
            assert.areEqual(4, len, "Object.fromEntries accesses correct number of properties");
            for (let i = 0; i < len; ++i)
            {
                assert.areEqual(expected[i], accessedProps[i], "Object.fromEntries accesses the correct properties");
            }
        }
    },
    {
        name : "Object.fromEntries accesses Proxy properties correctly",
        body : function () {
            const accessedProps = [];
            const handler = {
                get : function (target, prop, receiver) {
                    accessedProps.push(String(prop));
                    return Reflect.get(target, prop);
                },
                set : function () {
                    throw new Error ("Should not be called");
                }
            }
            let result;
            assert.doesNotThrow(()=>{result = Object.fromEntries(new Proxy([["a", 5], ["b", 2], ["c", 4]], handler)); });
            verifyObject({a : 5, b : 2, c : 4}, result);
            expected = ["Symbol(Symbol.iterator)", "length", "0", "length", "1", "length", "2", "length"];
            for (let i = 0; i < 3; ++i)
            {
                assert.areEqual(expected[i], accessedProps[i], "Object.fromEntries accesses the correct properties");
            }
        }
    },
    {
        name : "Object.fromEntries uses overridden array iterator",
        body : function () {
            let calls = 0;
            Array.prototype[Symbol.iterator] = function () {
                return {
                    next : function () {
                        switch (calls)
                        {
                            case 0:
                                calls = 1;
                                return { done : false, value : ["key", "value"]}
                            case 1:
                                calls = 2;
                                return { done : true, value : null }
                            case 2:
                            throw new Error ("Should not be reached");
                        }
                    }
                }
            }
            let result;
            assert.doesNotThrow(()=>{ result = Object.fromEntries([1, 2, 3, 4]);}, "Once iterator is done should not be called again");
            verifyObject({key : "value"}, result);
        }
    },
    {
        name : "Object.fromEntries properties",
        body : function () {
            assert.areEqual("fromEntries", Object.fromEntries.name, "Object.fromEntries.name should be 'fromEntries'");
            assert.areEqual(1, Object.fromEntries.length, "Object.fromEntries.length should be 1");
            const descriptor = Object.getOwnPropertyDescriptor(Object, "fromEntries");
            assert.isFalse(descriptor.enumerable, "Object.fromEntries should be enumerable");
            assert.isTrue(descriptor.writable, "Object.fromEntries should be writable");
            assert.isTrue(descriptor.configurable, "Object.fromEntries should be configurable");
            assert.doesNotThrow(()=>{"use strict"; delete Object.fromEntries.length;}, "Deleting Object.fromEntries.length should succeed");
            assert.areEqual(0, Object.fromEntries.length, "After deletion Object.fromEntries.length should be 0");
            assert.doesNotThrow(()=>{"use strict"; delete Object.fromEntries;}, "Deleting Object.fromEntries should succeed");
            assert.isUndefined(Object.fromEntries, "After deletion Object.fromEntries should be undefined");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
