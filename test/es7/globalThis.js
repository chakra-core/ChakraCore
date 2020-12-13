//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var global = this;
var bar = 5;

const tests = [
    {
        name: "globalThis is the global",
        body() {
            assert.areEqual(global, globalThis, "globalThis should be the global object");
        }
    },
    {
        name: "globalThis has global properties",
        body() {
            assert.areEqual(Array, globalThis.Array, "globalThis should have all global properties");
            assert.areEqual(JSON, globalThis.JSON, "globalThis should have all global properties");
            assert.areEqual(Object, globalThis.Object, "globalThis should have all global properties");
            assert.areEqual(5, globalThis.bar, "globalThis should have all global properties");
            assert.areEqual(global, globalThis.global, "globalThis should have all global properties");
            assert.areEqual(global, globalThis.globalThis, "globalThis should have itself as a property");
        }
    },
    {
        name: "globalThis has correct attributes",
        body() {
            const descriptor = Object.getOwnPropertyDescriptor(globalThis, "globalThis");
            assert.isFalse(descriptor.enumerable, "globalThis should not be enumerable");
            assert.isTrue(descriptor.configurable, "globalThis should be configurable");
            assert.isTrue(descriptor.writable, "globalThis should be writable");

            assert.doesNotThrow(()=>{globalThis = 5;}, "Overwriting globalThis should not throw");
            assert.areEqual(5, global.globalThis, "Overwriting globalThis should succeed");
            assert.doesNotThrow(()=>{delete global.globalThis;}, "Deleting globalThis should not throw");
            assert.areEqual(undefined, global.globalThis, "Deleting globalThis should succeed");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
