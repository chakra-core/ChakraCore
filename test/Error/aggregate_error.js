//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
   {
       name: "proptotype constructor",
       body: function ()
       {
            assert.areEqual(AggregateError, AggregateError.prototype.constructor);
       }
   },
   {
       name: "proptotype errors absent on protptype",
       body: function ()
       {
           assert.isFalse(AggregateError.prototype.hasOwnProperty("errors"));
       }
   },
   {
       name: "proptotype message",
       body: function ()
       {
           assert.areEqual("", AggregateError.prototype.message)
       }
   },
   {
        name: "proptotype name",
        body: function ()
        {
            assert.areEqual("AggregateError", AggregateError.prototype.name)
        }
    },
    {
        name: "prototype type",
        body: function ()
        {
            assert.areEqual("object", typeof AggregateError.prototype);
        }
    },
    {
        name: "prototype proto",
        body: function () {

            assert.areEqual(Error.prototype, Object.getPrototypeOf(AggregateError.prototype));
        }
    },
    {
        name: "name",
        body: function ()
        {
            assert.areEqual("AggregateError", AggregateError.name)
        }
    },
    {
        name: "length",
        body: function ()
        {
            assert.areEqual(2, AggregateError.length)
        }
    },
    {
        name: "errors iterable to list feilures",
        body: function ()
        {
            class TestError extends Error {}

            var case1 = {
                get [Symbol.iterator]() {
                    throw new TestError();
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case1);
            }, TestError);

            var case2 = {
                get [Symbol.iterator]() {
                    return {};
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case2);
            }, TypeError);

            var case3 = {
                [Symbol.iterator]() {
                    throw new TestError();
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case3);
            }, TestError);

            var case4 = {
                [Symbol.iterator]() {
                  return "a string";
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case4);
            }, TypeError);

            var case5 = {
                [Symbol.iterator]() {
                    return undefined;
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case5);
            }, TypeError);

            var case6 = {
                [Symbol.iterator]() {
                    return {
                        get next() {
                            throw new TestError();
                        }
                    }
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case6);
            }, TestError);

            var case7 = {
                [Symbol.iterator]() {
                    return {
                        get next() {
                            return {};
                        }
                    }
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case7);
            }, TypeError);

            var case8 = {
                [Symbol.iterator]() {
                    return {
                        next() {
                            throw new TestError();
                        }
                    }
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case8);
            }, TestError);

            var case9 = {
                [Symbol.iterator]() {
                    return {
                        next() {
                            return undefined;
                        }
                    }
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case9);
            }, TypeError);

            var case10 = {
                [Symbol.iterator]() {
                    return {
                        next() {
                            return "a string";
                        }
                    }
                }
            };
            assert.throws(() => {
                var obj = new AggregateError(case10);
            }, TypeError);

            var case11 = {
                [Symbol.iterator]() {
                    return {
                        next() {
                            return {
                                get done() {
                                    throw new TestError();
                                }
                            };
                        }
                    }
                }
            };

            assert.throws(() => {
                var obj = new AggregateError(case8);
            }, TestError);

        }
    },
    {
        name: "errors iterable to list",
        body: function ()
        {
            var count = 0;
            var values = [];
            var case1 = {
                [Symbol.iterator]() {
                    return {
                        next() {
                            count += 1;
                            return {
                                done: count === 3,
                                get value() {
                                    values.push(count)
                                }
                            };
                        }
                    };
                }
            };
            new AggregateError(case1);
            assert.areEqual(3, count)
            assert.areEqual([1, 2], values)
        }
    },
    {
        name: "message method prop cast",
        body: function ()
        {
            var case1 = new AggregateError([], 42);
            assert.areEqual("42", case1.message);

            var case2 = new AggregateError([], false);
            assert.areEqual("false", case2.message);

            var case3 = new AggregateError([], true);
            assert.areEqual("true", case3.message);

            var case4 = new AggregateError([], { toString() { return "string"; }});
            assert.areEqual("string", case4.message);

            var case5 = new AggregateError([], null);
            assert.areEqual("null", case5.message);
        }
    },
    {
        name: "message method prop",
        body: function ()
        {
            var obj = new AggregateError([], "42");
            assert.areEqual("42", obj.message);
        }
    },
    {
        name: "message tostring abrupt symbol",
        body: function ()
        {
            class TestError extends Error {}

            var case1 = Symbol();
            assert.throws(() => {
                new AggregateError([], case1);
            }, TypeError)

            var case2 = {
                [Symbol.toPrimitive]() {
                    return Symbol();
                },
                toString() {
                    throw new TestError();
                },
                valueOf() {
                    throw new TestError();
                }
            };

            assert.throws(() => {
                new AggregateError([], case2);
            }, TypeError)
        }
    },
    {
        name: "message tostring abrupt",
        body: function ()
        {
            class TestError extends Error { }

            var case1 = {
                [Symbol.toPrimitive]() {
                    throw new TestError();
                },
                toString() {
                    throw "toString called";
                },
                valueOf() {
                    throw "valueOf called";
                }
            };

            assert.throws(() => {
                new AggregateError([], case1);
            }, TestError);
            var case2 = {
                [Symbol.toPrimitive]: undefined,
                toString() {
                    throw new TestError();
                },
                valueOf() {
                    throw "valueOf called";
                }
            };
            assert.throws(() => {
                new AggregateError([], case2);
            }, TestError);

            var case3 = {
                [Symbol.toPrimitive]: undefined,
                toString: undefined,
                valueOf() {
                    throw new TestError();
                }
            };
            assert.throws(() => {
                new AggregateError([], case3);
            }, TestError);
        }
    },
    {
        name: "message undefined no prop",
        body: function ()
        {
            var case1 = new AggregateError([], undefined);
            assert.areEqual(false, Object.prototype.hasOwnProperty.call(case1, "message"))

            var case2 = new AggregateError([]);
            assert.areEqual(false, Object.prototype.hasOwnProperty.call(case2, "message"))
        }
    },
    {
        name: "newtarget is undefined",
        body: function ()
        {
            var obj = AggregateError([], "");

            assert.areEqual(AggregateError.prototype, Object.getPrototypeOf(obj));
            assert.isTrue(obj instanceof AggregateError);
        }
    },
    {
        name: "newtarget proto custom",
        body: function ()
        {
            var custom = { x: 42 };
            var newt = new Proxy(function () { }, {
                get(t, p) {
                    if (p === "prototype") {
                        return custom;
                    }

                    return t[p];
                }
            });

            var obj = Reflect.construct(AggregateError, [[]], newt);
            assert.areEqual(custom, Object.getPrototypeOf(obj));
            assert.areEqual(42, obj.x);
        }
    },
    {
        name: "newtarget proto fallback",
        body: function ()
        {
            const values = [
                undefined,
                null,
                42,
                false,
                true,
                Symbol(),
                "string",
                AggregateError.prototype,
            ];

            const NewTarget = new Function();

            for (const value of values) {
                const NewTargetProxy = new Proxy(NewTarget, {
                    get(t, p) {
                        if (p === "prototype") {
                            return value;
                        }
                        return t[p];
                    }
                });

                const error = Reflect.construct(AggregateError, [[]], NewTargetProxy);
                assert.areEqual(AggregateError.prototype, Object.getPrototypeOf(error));
            }
        }
    },
    {
        name: "newtarget proto",
        body: function ()
        {
            var obj = new AggregateError([]);
            assert.areEqual(AggregateError.prototype, Object.getPrototypeOf(obj));
        }
    },
    {
        name: "prop desc",
        body: function ()
        {
            assert.areEqual("function", typeof AggregateError);
        }
    },
    {
        name: "proto",
        body: function ()
        {
            var proto = Object.getPrototypeOf(AggregateError);
            assert.areEqual(Error, proto);
        }
    },
    {
        name: "order of args evaluation",
        body: function () {
            let sequence = [];
            const message = {
                toString() {
                    sequence.push(1);
                    return "";
                }
            };
            const errors = {
                [Symbol.iterator]() {
                    sequence.push(2);
                    return {
                        next() {
                            sequence.push(3);
                            return {
                                done: true
                            };
                        }
                    };
                }
            };

            new AggregateError(errors, message);
            assert.areEqual(3, sequence.length);
            assert.areEqual([1, 2, 3], sequence);
        }
    }
];
testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
