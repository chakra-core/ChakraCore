//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const tests = [
    {
        name : "name",
        body : function ()
        {
            assert.areEqual(Array.prototype.findLastIndex.name, "findLastIndex");
        }
    },
    {
        name : "length",
        body : function ()
        {
            assert.areEqual(Array.prototype.findLastIndex.length, 1);
        }
    },
    {
        name : "call with boolean",
        body : function ()
        {
            assert.areEqual(-1, Array.prototype.findLastIndex.call(true, () => {}));
            assert.areEqual(-1, Array.prototype.findLastIndex.call(false, () => {}));
        }
    },
    {
        name : "altered during loop",
        body : function ()
        {
            var arr = ["Shoes", "Car", "Bike"];
            var results = [];
            
            arr.findLastIndex(function(kValue) {
              if (results.length === 0) {
                arr.splice(1, 1);
              }
              results.push(kValue);
            });
            
            assert.areEqual(3, results.length, "predicate called three times");
            assert.areEqual("Bike", results[0]);
            assert.areEqual("Bike", results[1]);
            assert.areEqual("Shoes", results[2]);
            
            results = [];
            arr = ["Skateboard", "Barefoot"];
            arr.findLastIndex(function(kValue) {
              if (results.length === 0) {
                arr.unshift("Motorcycle");
                arr[0] = "Magic Carpet";
              }
            
              results.push(kValue);
            });
            
            assert.areEqual(2, results.length, "predicate called twice");
            assert.areEqual("Barefoot", results[0]);
            assert.areEqual("Magic Carpet", results[1]);
        }
    },
    {
        name : "not a constructor",
        body : function ()
        {
            assert.throws(function (){
                new Array.prototype.findLastIndex(() => {});
            }, TypeError)
        }
    },
    {
        name : "predicate call parameters",
        body : function ()
        {
            var arr = ["Mike", "Rick", "Leo"];
            var results = [];

            arr.findLastIndex(function(kValue, k, O) {
                results.push(arguments);
            });

            assert.areEqual(results.length, 3);

            var result = results[0];
            assert.areEqual("Leo", result[0]);
            assert.areEqual(2, result[1]);
            assert.areEqual(arr, result[2]);
            assert.areEqual(3, result.length);

            result = results[1];
            assert.areEqual("Rick", result[0]);
            assert.areEqual(1, result[1]);
            assert.areEqual(arr, result[2]);
            assert.areEqual(3, result.length);

            result = results[2];
            assert.areEqual("Mike", result[0]);
            assert.areEqual(0, result[1]);
            assert.areEqual(arr, result[2]);
            assert.areEqual(3, result.length);
        }
    },
    {
        name : "predicate call this non strict",
        body : function ()
        {
            var result;
            [1].findLastIndex(function(kValue, k, O) {
                result = this;
            });
            assert.areEqual(this, result);

            var o = {};
            [1].findLastIndex(function() {
                result = this;
            }, o);
            assert.areEqual(o, result);
        }
    },
    {
        name : "predicate call this strict",
        body : function ()
        {
            "use strict";
            var result;
            [1].findLastIndex(function(kValue, k, O) {
                result = this;
            });
            assert.areEqual(undefined, result);

            var o = {};
            [1].findLastIndex(function() {
                result = this;
            }, o);
            assert.areEqual(o, result);
        }
    },
    {
        name : "predicate called for each array property",
        body : function ()
        {
            var arr = [undefined, , , "foo"];
            var called = 0;

            arr.findLastIndex(function() {
                called++;
            });

            assert.areEqual(4, called);
        }
    },
    {
        name : "predicate is not callable throws",
        body : function ()
        {
            assert.throws(() => {
                [].findLastIndex({});
            }, TypeError);
            
            assert.throws(() => {
                [].findLastIndex(null);
            }, TypeError);
            
            assert.throws(() => {
                [].findLastIndex(undefined);
            }, TypeError);
            
            assert.throws(() => {
                [].findLastIndex(true);
            }, TypeError);
            
            assert.throws(() => {
                [].findLastIndex(1);
            }, TypeError);
            
            assert.throws(() => {
                [].findLastIndex("");
            }, TypeError);
            
            assert.throws(() => {
                [].findLastIndex(1);
            }, TypeError);
            
            assert.throws(() => {
                [].findLastIndex([]);
            }, TypeError);
            
            assert.throws(() => {
                [].findLastIndex(/./);
            }, TypeError);
        }
    },
    {
        name : "predicate not called on empty array",
        body : function ()
        {
            var called = false;
            var predicate = function() {
                called = true;
                return true;
            };

            var result = [].findLastIndex(predicate);
            assert.areEqual(-1, result);
            assert.areEqual(false, called);
        }
    },
    {
        name : "return abrupt from predicate call",
        body : function ()
        {
            class TestError extends Error { }

            var predicate = function() {
                throw new TestError();
            };
              
            assert.throws(function() {
                [1].findLastIndex(predicate);
            }, TestError);
        }
    },
    {
        name : "return abrupt from property",
        body : function ()
        {
            class TestError extends Error { }

            var o = {
                length: 1
            };
              
            Object.defineProperty(o, 0, {
                get: function() {
                    throw new TestError();
                }
            });
            
            assert.throws(function() {
                [].findLastIndex.call(o, function() {});
            }, TestError);
        }
    },
    {
        name : "return abrupt from this length as symbol",
        body : function ()
        {
            var o = {};
            o.length = Symbol(1);
            
            assert.throws(function() {
                [].findLastIndex.call(o, function() {});
            }, TypeError);
        }
    },
    {
        name : "return abrupt from this length",
        body : function ()
        {
            class TestError extends Error { }

            var o1 = {};
            Object.defineProperty(o1, "length", {
                get: function() {
                    throw new TestError();
                },
                configurable: true
            });

            assert.throws(function() {
                [].findLastIndex.call(o1);
            }, TestError);

            var o2 = {
                length: {
                    valueOf: function() {
                        throw new TestError();
                    }
                }
            };
            assert.throws(function() {
                [].findLastIndex.call(o2);
            }, TestError);
        }
    },
    {
        name : "return abrupt from this",
        body : function ()
        {
            assert.throws(function() {
                Array.prototype.findLastIndex.call(undefined, function() { });
            }, TypeError);
            
            assert.throws(function() {
                Array.prototype.findLastIndex.call(null, function() { });
            }, TypeError);
        }
    },
    {
        name : "return found value predicate result is true",
        body : function ()
        {
            var arr = ["Shoes", "Car", "Bike"];
            var called = 0;

            var result = arr.findLastIndex(function(val) {
                called++;
                return true;
            });

            assert.areEqual(2, result);
            assert.areEqual(1, called, "predicate was called once");

            called = 0;
            result = arr.findLastIndex(function(val) {
                called++;
                return val === "Bike";
            });

            assert.areEqual(1, called, "predicate was called three times");
            assert.areEqual(2, result);

            called = 0;
            result = arr.findLastIndex(function(val) {
                called++;
                return val === "Shoes";
            });

            assert.areEqual(3, called, "predicate was called three times");
            assert.areEqual(0, result);

            result = arr.findLastIndex(function(val) {
                return "string";
            });
            assert.areEqual(2, result, "coerced string");

            result = arr.findLastIndex(function(val) {
                return {};
            });
            assert.areEqual(2, result, "coerced object");

            result = arr.findLastIndex(function(val) {
                return Symbol("");
            });
            assert.areEqual(2, result, "coerced Symbol");

            result = arr.findLastIndex(function(val) {
                return 1;
            });
            assert.areEqual(2, result, "coerced number");

            result = arr.findLastIndex(function(val) {
                return -1;
            });
            assert.areEqual(2, result, "coerced negative number");
        }
    },
    {
        name : "return undefined if predicate returns false value",
        body : function ()
        {
            var arr = ["Shoes", "Car", "Bike"];
            var called = 0;

            var result = arr.findLastIndex(function(val) {
                called++;
                return false;
            });

            assert.areEqual(3, called, "predicate was called three times");
            assert.areEqual(-1, result);

            result = arr.findLastIndex(function(val) {
                return "";
            });
            assert.areEqual(-1, result, "coerced string");

            result = arr.findLastIndex(function(val) {
                return undefined;
            });
            assert.areEqual(-1, result, "coerced undefined");

            result = arr.findLastIndex(function(val) {
                return null;
            });
            assert.areEqual(-1, result, "coerced null");

            result = arr.findLastIndex(function(val) {
                return 0;
            });
            assert.areEqual(-1, result, "coerced 0");

            result = arr.findLastIndex(function(val) {
                return NaN;
            });
            assert.areEqual(-1, result, "coerced NaN");
        }
    },
    {
        name : "return last item",
        body : function ()
        {
            var arr = [ { value: "Shoes" }, { value: "Car" }, { value: "Shoes" }, { value: "Bike" } ];
            var called = 0;

            var result = arr.findLastIndex(function (val) {
                called++;
                return val.value === "Shoes";
            });

            assert.areEqual(2, result);
            assert.areEqual(2, called);
        }
    },
    {
        name : "length limit and behavior",
        body : function () {
            var arrZero = [];
            var arrOne = [1];
            var arrMax = [];
            arrMax.length = 2 ** 32 - 1;
            arrMax[2 ** 32 - 2] = 1;

            var result = arrZero.findLastIndex(function (x) {
                return x === 1
            });
            assert.areEqual(-1, result);

            result = arrOne.findLastIndex(function (x) {
                return x === 1
            });
            assert.areEqual(0, result);

            result = arrOne.findLastIndex(function (x) {
                return x === 2
            });
            assert.areEqual(-1, result);

            result = arrMax.findLastIndex(function (x) {
                return x === 1
            });
            assert.areEqual(2 ** 32 - 2, result);
        }
    }
]

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
