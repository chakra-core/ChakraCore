//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("../UnitTestFramework/UnitTestFramework.js");

const tests = [
    {
        name: "Array.prototype.sort basic properties",
        body () {
            assert.areEqual(1, Array.prototype.sort.length, "Array.prototype.sort should have length of 1");
            assert.areEqual("sort", Array.prototype.sort.name, "Array.prototype.sort should have name 'sort'");
            const desc = Object.getOwnPropertyDescriptor(Array.prototype, "sort");
            assert.isTrue(desc.writable, "Array.prototype.sort.writable === true");
            assert.isFalse(desc.enumerable, "Array.prototype.sort.enumerable === false");
            assert.isTrue(desc.configurable, "Array.prototype.sort.configurable === true");
            assert.areEqual('function', typeof desc.value, "typeof Array.prototype.sort === 'function'");
            assert.throws(()=>{[].sort("not a function");}, TypeError);
            assert.throws(()=>{[].sort(null);}, TypeError);
            assert.doesNotThrow(()=>{[].sort(undefined);}, "Array.prototype.sort with undefined sort parameter does not throw");
        }
    },
    {
        name: "Array.prototype.sort basic sort cases with arrays of numbers",
        body () {
            const arrayOne = [120, 5, 8, 4, 6, 9, 9, 10, 2, 3];
            assert.areEqual([10,120,2,3,4,5,6,8,9,9], arrayOne.sort(undefined), "Array.sort with default comparator should sort based on string ordering");
            const result = [2,3,4,5,6,8,9,9,10,120];
            assert.areEqual(result, arrayOne.sort((x, y) => x - y ), "Array.sort with numerical comparator should sort numerically");
            assert.areEqual(result, arrayOne, "Array.sort should sort original array as well as return value");
            assert.areEqual(result, arrayOne.sort((x, y) => { return (x > y) ? 1 : ((x < y) ? -1 : 0); }), "1/-1/0 numerical comparison should be the same as x - y");
            const arrayTwo = [25, 8, 7, 41];
            assert.areEqual([41,25,8,7], arrayTwo.sort((a, b) => b - a), "Array.sort with (a,b)=>b-a should sort descending");
            assert.areEqual([7,8,25,41], arrayTwo.sort((a, b) => a - b), "Array.sort with (a,b)=>a-b should sort ascending");
            assert.areEqual([1, 1.2, 4 , 4.8, 12], [ 1, 1.2, 12, 4.8, 4 ].sort((a, b) => a - b), "Array.sort with numerical comparator handles floats correctly");
        }
    },
    {
        name : "Array.prototype.sort basic sort cases with arrays of strings",
        body () {
            const stringsOnes = new Array("some", "sample", "strings", "for", "testing");
            const result = ["for", "sample", "some",  "strings", "testing"];
            assert.areEqual(result, stringsOnes.sort(), "Array.sort with no comparison function sorts strings alphabetically");
            assert.areEqual(result, stringsOnes.sort((a, b) => a - b), "Array.sort with numerical comparison function doesn't re-order strings");
            assert.areEqual(["some", "sample", "strings", "for", "testing"],
                ["some", "sample", "strings", "for", "testing"].sort((a, b) => {a - b}), "Array.sort with numerical comparison function doesn't re-order strings");
        }
    },
    {
        name : "Array.prototype.sort with a compare function with side effects",
        body () {
            let xyz = 5;
            function setXyz() { xyz = 10; return 0;}
            [].sort (setXyz);
            assert.areEqual(5, xyz, "Array.sort does not call compare function when length is 0");
            [1].sort (setXyz)
            assert.areEqual(5, xyz, "Array.sort does not call compare function when length is 1");
            [undefined, undefined, undefined, undefined].sort(setXyz);
            assert.areEqual(5, xyz, "Array.sort does not call compare function when all elements are undefined");
            [5, undefined, , undefined].sort(setXyz);
            assert.areEqual(5, xyz, "Array.sort does not call compare function when only one element is defined");
            [1, 2, undefined].sort(setXyz);
            assert.areEqual(10, xyz, "Array.sort calls compare function if there is > 1 defined element");
        }   
    },
    {
        name : "Array.prototype.sort with various objects",
        body () {
            const cases = {
                "array like object" : {obj : {0 : 8, 1: 41, 2: 25, 3 : 7, length : 4 }, length : 4, result : "25,41,7,8", resultTwo : "41,25,8,7"},
                "empty array": {obj : [],  length : 0, result : ""},
                "sparse array": {obj : (function(){
                    const arr = [5, undefined];
                    arr[7] = 3;
                    arr[10] = undefined;
                    return arr;})(),  length : 11, result : "3,5,,,,,,,,,", resultTwo : "5,3,,,,,,,,,"},
                "array with one undefined": {obj : [undefined],  length : 1, result : ""},
                "array with one null": {obj : (function(){
                    var o = [0];
                    delete o[0];
                    return o;
                })(),  length : 1, result : ""},
                "array with two undefined": {obj: [undefined, undefined],  length : 2, result : ","},
                "array with multiple undefined": {obj : [undefined,,undefined,undefined,,,,undefined],  length : 8, result : ",,,,,,,"},
                "array with multiple null": {obj : (function () {
                    var o = [7,,5,2,,,6];
                    for (var i = 0; i < o.length; i++) {
                        delete o[i];
                    }
                    return o;
                })(),  length : 7, result : ",,,,,,"},
                "array with mixed undefined and null": {obj : (function () {
                    var o = [undefined,1,,9,,3,8,undefined];
                    delete o[0];
                    return o;
                })(), result : "1,3,8,9,,,,",  length : 8, resultTwo : "9,8,3,1,,,,"},
                "empty object": {obj : { length: 0 }, length : 0, result : ""},
                "object with one undefined": {obj : { 0: undefined, length: 1}, length : 1, result : ""},
                "object with one missing": { obj: {length: 1}, length : 1, result : ""},
                "object with undefined, missing": { obj : {0: undefined,length: 2}, length : 2, result :","},
                "object with multiple undefined": {
                    obj: {0: undefined,3: undefined,7: undefined,8: undefined,length: 10}
                    , length: 10, result :",,,,,,,,,"},
                "adhoc object": {obj : {0: 7, 2: 5, 3: 2, 6: 6, length: 10}, length : 10, result :"2,5,6,7,,,,,,", resultTwo : "7,6,5,2,,,,,,"}
            }
            function testObj(name, item) {
                if (! Array.isArray(item.obj)) {
                    item.obj.sort = Array.prototype.sort;
                    item.obj.join = Array.prototype.join;
                    item.obj.toString = Array.prototype.toString;
                }
                if (!("resultTwo" in item)) { item.resultTwo = item.result}
                assert.areEqual(item.result, item.obj.sort().toString(), "Array.prototype.sort with default compare of " + name + " should produce expected result");
                assert.areEqual(item.resultTwo, item.obj.sort((a, b) => b - a).toString(), "Array.prototype.sort with numerical reverse compare of " + name + " should produce expected result");
                assert.areEqual(item.length, item.obj.length, "Array.prototype.sort of " + name + " should not alter length");
            }
            for (let i in cases) {
                testObj(i, cases[i]);
            }
        }
    },
    {
        name : "Array.prototype.sort with un-sortable objects",
        body () {
            const obj = {0 : 20, 1 : 15, 2 : 11};
            Array.prototype.sort.call(obj);
            assert.isTrue (obj[0] === 20 && obj[1] === 15 && obj[2] === 11, "Array.prototype.sort called on an object without a length does nothing");
            assert.doesNotThrow(()=>{Array.prototype.sort.call(obj, function () { throw new Error("Do not call this")})}, "Array.prototype.sort called on an object without a length does not call compare method");
            assert.areEqual(Object(200), Array.prototype.sort.call(200, function () { throw new Error("Do not call this")}), "Array.prototype.sort called on on a number returns Object(number)");
            const str = "any string";
            assert.throws(()=>{Array.prototype.sort.call("any string")}, TypeError); // Array.prototype.sort called on a string throws
            assert.areEqual("any string", str, "Array.prototype.sort did not alter string before throwing");
        }
    },
    {
        name : "Array.prototype.sort prototype lookups",
        body () {
            for (let i = 0; i < 20; i = i+4) {
                Object.prototype[i] = "o"+i;
            }

            for (let i = 0; i < 20; i = i+3) {
                Array.prototype[i] = "p"+i;
            }
            Array.prototype[14] = undefined;
            Object.prototype[2] = undefined;
            const arrayOne = [23,14, undefined, 17];

            arrayOne[10] = 5;
            arrayOne[11] = 22;
            arrayOne[12] = undefined;
            arrayOne[13] = 20;
            const resultOne = [14,17,20,22,23,5,"o4","o8","p6","p9",,,"p12",];
            resultOne.length = 14;
            //print (resultOne);
            //print (arrayOne.sort());
            compareSparseArrays(resultOne, arrayOne.sort(), "Array.prototype.sort pulls values from Object.prototype and Array.prototype");
            compareSparseArrays(resultOne, arrayOne, "Array.prototype.sort pulls values from Object.prototype and Array.prototype");

            const arrayTwo = new Array (3);
            const resultTwo = ["p0",,];
            resultTwo.length = 3;
            compareSparseArrays(resultTwo, arrayTwo.sort(), "Array.prototype.sort pulls values from Array.prototype");
            compareSparseArrays(resultTwo, arrayTwo, "Array.prototype.sort pulls values from Array.prototype");

            Array.prototype[0] = 0;
            Array.prototype[1] = 0;
            Array.prototype[2] = 0;
            Array.prototype[5] = 10;
            Array.prototype[6] = 1;
            Array.prototype[7] = 15;

            const arrayThree = new Array (8);
            arrayThree[0] = 1;
            arrayThree[1] = 2;
            arrayThree[2] = 3;
            const resultThree = [1,1,10,15,2,3,"o4","p3"];
            compareSparseArrays(resultThree, arrayThree.sort(), "Array.prototype.sort pulls values from Object.prototype and Array.prototype");
            compareSparseArrays(resultThree, arrayThree, "Array.prototype.sort pulls values from Object.prototype and Array.prototype");

            const arrayFour = new Array(8)
            arrayFour[1] = 1;
            arrayFour[5] = undefined;
            const resultFour = [0,0,1,1,15,"o4","p3",undefined];
            compareSparseArrays(resultFour, arrayFour.sort(), "Array.prototype.sort pulls values from Object.prototype and Array.prototype");
            compareSparseArrays(resultFour, arrayFour, "Array.prototype.sort pulls values from Object.prototype and Array.prototype");

            Array.prototype[12]=10;
            const arrayFive = new Array(8)
            arrayFive[1] = 1;
            const resultFive = [0,0,1,1,10,15,"o4","p3"];
            compareSparseArrays(resultFive, arrayFive.sort(), "Array.prototype.sort pulls values from Object.prototype and Array.prototype");
            compareSparseArrays(resultFive, arrayFive, "Array.prototype.sort pulls values from Object.prototype and Array.prototype");
        }
    },
    {
        //Test that bug 5719 is fixed https://github.com/Microsoft/ChakraCore/issues/5719
        name : "Array-like object does not pull values from Array.prototype",
        body () {
            const arrayLike = {
                length : 4
            }
            Array.prototype.sort.call(arrayLike);
            assert.areEqual("o0", arrayLike[0], "Array.prototype.sort pulls values form Object.prototype for array-like object");
            assert.areNotEqual("p3", arrayLike[1], "Array.prototype.sort does not pull values form Array.prototype for array-like object");
        }
    },
    {
        name : "Array.prototype.sort with edited prototypes and compare function side effects",
        body () {
            const arrayOne = new Array(2);
            arrayOne[0] = 12;
            arrayOne[1] = 10;
            const resultOne = [10, "test"];
            compareSparseArrays(resultOne, arrayOne.sort((x,y) => { arrayOne[0] = "test"; return x - y; }), "Compare function set element effects array");
            compareSparseArrays(resultOne, arrayOne, "Compare function side effect effects array");

            const arrayTwo = new Array(3);
            arrayTwo[0] = 12;
            arrayTwo[2] = 10;
            const resultTwo = [0,0,10];
            compareSparseArrays(resultTwo, arrayTwo.sort((x, y) => { delete arrayTwo[0]; return x - y; }), "Compare function delete element effects array");
            compareSparseArrays(resultTwo, arrayTwo, "Compare function delete element effects array");
        }
    }
];

//assert.areEqual does not work for directly comparing sparse arrays
function compareSparseArrays(arrayOne, arrayTwo, message) {
    const len = arrayOne.length;
    assert.areEqual (len, arrayTwo.length, message + " lengths are not the same");
    for (let i = 0; i < len; ++i) {
        assert.areEqual (arrayOne[i], arrayTwo[i], message + " property " + i + " is not the same");
    }
}

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
