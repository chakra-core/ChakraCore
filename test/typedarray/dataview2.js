//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const view = new DataView(new ArrayBuffer(50));
const setters = ["setFloat32", "setFloat64", "setInt16", "setInt32", "setInt8", "setUint16", "setUint32", "setUint8"];
const getters = ["getFloat32", "getFloat64", "getInt16", "getInt32", "getInt8", "getUint16", "getUint32", "getUint8"];

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
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });