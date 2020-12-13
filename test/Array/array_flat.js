//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testFlat(input, output, depth)
{
    assert.areEqual(output, Array.prototype.flat.call(input, depth));
}

function testFlatMap(input, output, mappingFunction, thisArg)
{
    assert.areEqual(output, Array.prototype.flatMap.call(input, mappingFunction, thisArg));
}

const tests = [
    {
        name : "properties",
        body : function ()
        {
            assert.areEqual("flat", Array.prototype.flat.name, "Array.prototype.flat name should be flat");
            assert.areEqual("flatMap", Array.prototype.flatMap.name, "Array.prototype.flatMap name should be flatMap");
            assert.areEqual(0, Array.prototype.flat.length, "Array.prototype.flat length should be 0");
            assert.areEqual(1, Array.prototype.flatMap.length, "Array.prototype.flatMap length should be 1");
        }
    },
    {
        name : "flatten arrays",
        body : function ()
        {
            testFlat([2, 3, [4, 5]], [2, 3, 4, 5]);
            testFlat([2, 3, [4, [5, 6]]], [2, 3, 4, [5, 6]]);
            testFlat([2, 3, [4, [5, 6]]], [2, 3, 4, 5, 6], 2);
            testFlat([], []);
            testFlat([[], [], 1], [1]);
            const typedArr = new Int32Array(3);
            const typedArr2 = new Int32Array(3);
            typedArr[0] = 5;
            typedArr[1] = 6;
            typedArr[2] = 3;
            typedArr2[0] = 5;
            typedArr2[1] = 6;
            typedArr2[2] = 3;
            testFlat(typedArr, typedArr2);
        }
    },
    {
        name : "flatMap arrays",
        body : function ()
        {
            testFlatMap([2, 3, 4, 5], [2, 4, 3, 6, 4, 8, 5, 10], function (a) { return [a, a * 2]});
            const thisArg = { count : 0 };
            testFlatMap([2, 3, 4], [2, 3, 3, 4, 4, 5], function (a) { this.count += a; return [ a, a + 1]}, thisArg);
            testFlatMap([2, 3, 4], [[2], [3], [4]], function (a) { return [[a]]});
            assert.areEqual(9, thisArg.count);

            assert.throws(()=>{[2, 3].flatMap("Not Callable")});
            assert.throws(()=>{[2, 3].flatMap(class NotCallable {})});
        }
    },
    {
        name : "flatMap abnormal this",
        body : function ()
        {
            "use strict";
            testFlatMap([2, 3], [null, null], function () { return [this]}, null);
            testFlatMap([2, 3], [undefined, undefined], function () { return [this]}, undefined);
            testFlatMap([2, 3], [undefined, undefined], function () { return [this]});
            testFlatMap([2, 3], ["", ""], function () { return [this]}, "");
            testFlatMap([2, 3], ["Test", "Test"], function () { return [this]}, "Test");
            const boo = {};
            testFlatMap([2, 3], [boo, boo], function () { return [this]}, boo);
        }
    },
    {
        name : "Proxied Array",
        body : function ()
        {
            let getCount = 0, hasCount = 0;
            const handler = {
                get : function (t, p, r) { ++getCount; return Reflect.get(t, p, r); },
                has : function (t, p, r) { ++hasCount; return Reflect.has(t, p, r); }
            }
            const prox = new Proxy ([2, [3, 5]], handler);
            testFlat(prox, [2, 3, 5]);
            assert.areEqual(4, getCount); // length and constructor are also accessed hence count 2 higher than length
            assert.areEqual(2, hasCount);
            const prox2 = new Proxy ([2, 3, 5], handler);
            testFlatMap(prox2, [2, 4, 3, 6, 5, 10], function (a) { return [a, a * 2]});
            assert.areEqual(9, getCount);  // length and constructor are also accessed hence count 2 higher than length
            assert.areEqual(5, hasCount);
        }
    },
    {
        name : "Invalid object",
        body : function ()
        {
            assert.throws(() => {Array.prototype.flat.call(null)}, TypeError);
            assert.throws(() => {Array.prototype.flat.call(undefined)}, TypeError);
            assert.throws(() => {Array.prototype.flatMap.call(null)}, TypeError);
            assert.throws(() => {Array.prototype.flatMap.call(undefined)}, TypeError);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
