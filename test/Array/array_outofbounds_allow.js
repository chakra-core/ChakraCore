//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const ArrayPrototype = Array.prototype;
const at = ArrayPrototype.at;

const call = Function.call.bind(Function.call); // (fn: Function, thisArg: any, ...args: any[]) => any


const tests = [
    {
        name: "Allow out-of-bounds write",
        body: function () {
            let caught = false;
            try {
                let arr = new Array(2);
                arr[0] = 0;
                arr[1] = 1;
                arr[100] = 100;
                assert.areEqual(arr[100], 100, "Array out-of-bounds write should be allowed");
            } catch (e) {
                caught = true;
            }
            assert.isFalse(caught, "Array out-of-bounds write should be allowed");
        }
    },
    {
        name: "Allow out-of-bounds read",
        body: function () {
            let caught = false;
            try {
                let arr = new Array(2);
                arr[0] = 0;
                arr[1] = 1;
                assert.areEqual(arr[100], undefined, "Array out-of-bounds read should be allowed");
            } catch (e) {
                caught = true;
            }
            assert.isFalse(caught, "Array out-of-bounds read should be allowed");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
