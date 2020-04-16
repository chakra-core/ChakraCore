//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const view = new DataView(new ArrayBuffer(50));
const zeroView = new DataView(new ArrayBuffer(0));
const smallView = new DataView(new ArrayBuffer(1));
const longView = new DataView(new ArrayBuffer(8));

const setters = ["setUint8", "setInt8", "setUint16", "setInt16", "setFloat32", "setInt32", "setUint32", "setFloat64"];
const getters = ["getUint8", "getInt8", "getUint16", "getInt16", "getFloat32", "getInt32", "getUint32", "getFloat64"];

const tests = [
    {
        name : "Set with negative index",
        body : function ()
        {
            for (let i = 0; i < setters.length; ++i)
            {
                assert.throws(()=>{ view[setters[i]](-1, 2); }, RangeError);
                assert.throws(()=>{ view[setters[i]](-1000000000, 2); }, RangeError);
            }
        }
    },
    {
        name : "Get with negative index",
        body : function ()
        {
            for (let i = 0; i < setters.length; ++i)
            {
                assert.throws(()=>{ view[getters[i]](-1); }, RangeError);
                assert.throws(()=>{ view[getters[i]](-1000000000); }, RangeError);
            }
        }
    },
    {
        name : "Set value in 0 buffer",
        body : function ()
        {
            for (let i = 0; i < setters.length; ++i)
            {
                assert.throws(()=>{ zeroView[setters[i]](0, 2); }, RangeError);
            }
        }
    },
    {
        name : "Get value in 0 buffer",
        body : function ()
        {
            for (let i = 0; i < setters.length; ++i)
            {
                assert.throws(()=>{ zeroView[getters[i]](0); }, RangeError);
            }
        }
    },
    {
        name : "Set value in 1 byte buffer",
        body : function ()
        {
            assert.doesNotThrow(()=>{ smallView[setters[0]](0, 3) }, "Storing int 8 in 1 byte array should not throw");
            assert.doesNotThrow(()=>{ smallView[setters[1]](0, 3) }, "Storing int 8 in 1 byte array should not throw");
            for (let i = 2; i < setters.length; ++i)
            {
                assert.throws(()=>{ smallView[setters[i]](0, 2); }, RangeError);
                assert.throws(()=>{ smallView[setters[i]](0, 2); }, RangeError);
            }
        }
    },
    {
        name : "Get value in 1 byte buffer",
        body : function ()
        {
            assert.doesNotThrow(()=>{ smallView[getters[0]](0) }, "Storing int 8 in 1 byte array should not throw");
            assert.doesNotThrow(()=>{ smallView[getters[1]](0) }, "Storing int 8 in 1 byte array should not throw");
            for (let i = 2; i < setters.length; ++i)
            {
                assert.throws(()=>{ smallView[getters[i]](0); }, RangeError);
                assert.throws(()=>{ smallView[getters[i]](0); }, RangeError);
            }
        }
    },
    {
        name : "Set undefined value and get value in 8 byte buffer",
        body : function ()
        {
            const results = [0, 0, 0, 0, NaN, 0, 0, NaN];
            for (let i = 0; i < setters.length; ++i)
            {
                assert.doesNotThrow(() => longView[setters[i]](0), "Set undefined value in byte array should not throw");
                const value = longView[getters[i]](0);
                assert.areEqual(results[i], value, "Result mismatch when get value from an undefined value");
            }
        }
    },
    {
        name : "Set undefined value with little endian and get value in 8 byte buffer",
        body : function ()
        {
            const results = [0, 0, 0, 0, 6.905458702346266e-41, 0, 0, 3.143e-319];
            for (let i = 2; i < setters.length; ++i)
            {
                assert.doesNotThrow(() => longView[setters[i]](0, undefined, true), "Set undefined value with little endian in byte array should not throw");
                const value = longView[getters[i]](0);
                assert.areEqual(results[i], value, "Result mismatch when get value from undefined value with little endian");
            }
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
