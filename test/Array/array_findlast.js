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
            assert.areEqual(Array.prototype.findLast.name, "findLast");
        }
    },
    {
        name : "length",
        body : function ()
        {
            assert.areEqual(Array.prototype.findLast.length, 1);
        }
    },
    {
        name : "call with boolean",
        body : function ()
        {
            assert.areEqual(undefined, Array.prototype.findLast.call(true, () => {}));
            assert.areEqual(undefined, Array.prototype.findLast.call(false, () => {}));
        }
    },
    {
        name : "altered during loop",
        body : function ()
        {
            var arr = ["Shoes", "Car", "Bike"];
            var results = [];
            
            arr.findLast(function(kValue) {
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
            arr.findLast(function(kValue) {
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
                new Array.prototype.findLast(() => {});
            }, TypeError)
        }
    },
    {
        name : "predicate call parameters",
        body : function ()
        {
            var arr = ["Mike", "Rick", "Leo"];
            var results = [];

            arr.findLast(function(kValue, k, O) {
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
            [1].findLast(function(kValue, k, O) {
                result = this;
            });
            assert.areEqual(this, result);

            var o = {};
            [1].findLast(function() {
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
            [1].findLast(function(kValue, k, O) {
                result = this;
            });
            assert.areEqual(undefined, result);

            var o = {};
            [1].findLast(function() {
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

            arr.findLast(function() {
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
                [].findLast({});
            }, TypeError);
            
            assert.throws(() => {
                [].findLast(null);
            }, TypeError);
            
            assert.throws(() => {
                [].findLast(undefined);
            }, TypeError);
            
            assert.throws(() => {
                [].findLast(true);
            }, TypeError);
            
            assert.throws(() => {
                [].findLast(1);
            }, TypeError);
            
            assert.throws(() => {
                [].findLast("");
            }, TypeError);
            
            assert.throws(() => {
                [].findLast(1);
            }, TypeError);
            
            assert.throws(() => {
                [].findLast([]);
            }, TypeError);
            
            assert.throws(() => {
                [].findLast(/./);
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

            var result = [].findLast(predicate);
            assert.areEqual(undefined, result);
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
                [1].findLast(predicate);
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
                [].findLast.call(o, function() {});
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
                [].findLast.call(o, function() {});
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
                [].findLast.call(o1);
            }, TestError);

            var o2 = {
                length: {
                    valueOf: function() {
                        throw new TestError();
                    }
                }
            };
            assert.throws(function() {
                [].findLast.call(o2);
            }, TestError);
        }
    },
    {
        name : "return abrupt from this",
        body : function ()
        {
            assert.throws(function() {
                Array.prototype.findLast.call(undefined, function() { });
            }, TypeError);
            
            assert.throws(function() {
                Array.prototype.findLast.call(null, function() { });
            }, TypeError);
        }
    },
    {
        name : "return found value predicate result is true",
        body : function ()
        {
            var arr = ["Shoes", "Car", "Bike"];
            var called = 0;

            var result = arr.findLast(function(val) {
                called++;
                return true;
            });

            assert.areEqual("Bike", result);
            assert.areEqual(1, called, "predicate was called once");

            called = 0;
            result = arr.findLast(function(val) {
                called++;
                return val === "Bike";
            });

            assert.areEqual(1, called, "predicate was called three times");
            assert.areEqual("Bike", result);

            called = 0;
            result = arr.findLast(function(val) {
                called++;
                return val === "Shoes";
            });

            assert.areEqual(3, called, "predicate was called three times");
            assert.areEqual("Shoes", result);

            result = arr.findLast(function(val) {
                return "string";
            });
            assert.areEqual("Bike", result, "coerced string");

            result = arr.findLast(function(val) {
                return {};
            });
            assert.areEqual("Bike", result, "coerced object");

            result = arr.findLast(function(val) {
                return Symbol("");
            });
            assert.areEqual("Bike", result, "coerced Symbol");

            result = arr.findLast(function(val) {
                return 1;
            });
            assert.areEqual("Bike", result, "coerced number");

            result = arr.findLast(function(val) {
                return -1;
            });
            assert.areEqual("Bike", result, "coerced negative number");
        }
    },
    {
        name : "return undefined if predicate returns false value",
        body : function ()
        {
            var arr = ["Shoes", "Car", "Bike"];
            var called = 0;

            var result = arr.findLast(function(val) {
                called++;
                return false;
            });

            assert.areEqual(3, called, "predicate was called three times");
            assert.areEqual(undefined, result);

            result = arr.findLast(function(val) {
                return "";
            });
            assert.areEqual(undefined, result, "coerced string");

            result = arr.findLast(function(val) {
                return undefined;
            });
            assert.areEqual(undefined, result, "coerced undefined");

            result = arr.findLast(function(val) {
                return null;
            });
            assert.areEqual(undefined, result, "coerced null");

            result = arr.findLast(function(val) {
                return 0;
            });
            assert.areEqual(undefined, result, "coerced 0");

            result = arr.findLast(function(val) {
                return NaN;
            });
            assert.areEqual(undefined, result, "coerced NaN");
        }
    },
    {
        name : "return last item",
        body : function ()
        {
            var arr = [ { value: "Shoes" }, { value: "Car" }, { value: "Shoes" }, { value: "Bike" } ];
            var called = 0;

            var result = arr.findLast(function (val) {
                called++;
                return val.value === "Shoes";
            });

            assert.areEqual("Shoes", result.value);
            assert.areEqual(arr[2], result);
            assert.areEqual(2, called);
        }
    },
    {
        name : "length limit and behavior",
        body : function () {
            var arrZero = [];
            var arrOne = [{ value: 1 }];
            var arrMax = [];
            arrMax.length = 2 ** 32 - 1;
            var maxResult = arrMax[2 ** 32 - 2] = { value: 1 };

            var result = arrZero.findLast(function (x) {
                return x.value === 1
            });
            assert.areEqual(undefined, result);

            result = arrOne.findLast(function (x) {
                return x.value === 1
            });
            assert.areEqual(arrOne[0], result);

            result = arrOne.findLast(function (x) {
                return x.value === 2
            });
            assert.areEqual(undefined, result);

            result = arrMax.findLast(function (x) {
                return x.value === 1
            });
            assert.areEqual(maxResult, result);
        }
    }
]

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
