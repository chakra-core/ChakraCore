//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "#6179 - Assert in JavascriptArray::EntryReverse when length > Uint32Max for TypedArray with Array prototype",
        body: function () {
            var ua = new Uint32Array(0x10);
            ua.__proto__ = new Array(0xffffffff);
            ua.length = 0xffffffff*2;

            assert.throws(()=>ua.reverse(), TypeError, "Array#reverse tries to delete a property on the TypedArray but that throws");
        }
    },
    {
        name: "#6179 - Assert in JavascriptArray::EntryReverse when length > Uint32Max for TypedArray with own length property",
        body: function () {
            var ua = new Uint32Array(0x10);
            Object.defineProperty(ua, 'length', {value: 0xffffffff*2 });

            assert.throws(()=>Array.prototype.reverse.call(ua), TypeError, "Array#reverse tries to delete a property on the TypedArray but that throws");
        }
    },
    {
        name: "#6179 - Assert in JavascriptArray::EntryReverse when length > Uint32Max for an object with length property",
        body: function () {
            let getCount = 0;
            let setCount = 0;
            var ua = {
                length: 0xffffffff*2,
                set [0xffffffff*2-1](v) {
                    assert.areEqual(1, getCount, "1 === getCount");
                    assert.areEqual(0, setCount, "0 === setCount");
                    setCount++
                },
                get '0'() {
                    assert.areEqual(0, getCount, "0 === getCount");
                    assert.areEqual(0, setCount, "0 === setCount");
                    getCount++
                },
                get '1'() {
                    assert.areEqual(1, getCount, "1 === getCount");
                    assert.areEqual(1, setCount, "1 === setCount");
                    throw 123; 
                }
            };

            assert.throws(()=>Array.prototype.reverse.call(ua), 123, "Array#reverse will throw above when we try and get property '1'");

            assert.areEqual(1, getCount, "1 === getCount");
            assert.areEqual(1, setCount, "1 === setCount");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
