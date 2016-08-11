//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testEquality(v1, v2, str) {
    if (v1 !== v2) {
        print(`Fail: ${str}`);
    }
    else
    {
        print('Pass');
    }
}

var tests = [
    {
        name: "%ModuleStatus% has the correct shape",
        body: function () {
            var ModuleStatus = Reflect.Module.Status;
           
            var descriptor = Object.getOwnPropertyDescriptor(Reflect.Module, 'Status');
            assert.isFalse(descriptor.writable, "Reflect.Module.Status.writable === false");
            assert.isFalse(descriptor.enumerable, "Reflect.Module.Status.enumerable === false");
            assert.isTrue(descriptor.configurable, "Reflect.Module.Status.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof Reflect.Module.Status === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatus, 'prototype');
            assert.isFalse(descriptor.writable, "%ModuleStatus%.prototype.writable === false");
            assert.isFalse(descriptor.enumerable, "%ModuleStatus%.prototype.enumerable === false");
            assert.isFalse(descriptor.configurable, "%ModuleStatus%.prototype.configurable === false");
            assert.areEqual('object', typeof descriptor.value, "typeof %ModuleStatus%.prototype === 'object'");

            assert.areEqual(3, ModuleStatus.length, "%ModuleStatus%.length === 3");
            assert.isTrue(ModuleStatus.__proto__ === Function.prototype, "%ModuleStatus%.__proto__ === %FunctionPrototype%");
        }
    },
    {
        name: "%ModuleStatusPrototype% has the correct shape",
        body: function () {
            var ModuleStatusPrototype = Reflect.Module.Status.prototype;

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'constructor');
            assert.isFalse(descriptor.writable, "%ModuleStatusPrototype%.constructor.writable === false");
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.constructor.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.constructor.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %ModuleStatusPrototype%.constructor === 'function'");
            assert.isTrue(ModuleStatusPrototype.constructor === Reflect.Module.Status, "%ModuleStatusPrototype%.constructor === %ModuleStatus%");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'resolve');
            assert.isTrue(descriptor.writable, "%ModuleStatusPrototype%.resolve.writable === true");
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.resolve.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.resolve.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %ModuleStatusPrototype%.resolve === 'function'");
            assert.areEqual(2, descriptor.value.length, "%ModuleStatusPrototype%.resolve.length === 2");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'load');
            assert.isTrue(descriptor.writable, "%ModuleStatusPrototype%.load.writable === true");
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.load.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.load.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %ModuleStatusPrototype%.load === 'function'");
            assert.areEqual(1, descriptor.value.length, "%ModuleStatusPrototype%.load.length === 1");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'result');
            assert.isTrue(descriptor.writable, "%ModuleStatusPrototype%.result.writable === true");
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.result.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.result.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %ModuleStatusPrototype%.result === 'function'");
            assert.areEqual(1, descriptor.value.length, "%ModuleStatusPrototype%.result.length === 1");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'reject');
            assert.isTrue(descriptor.writable, "%ModuleStatusPrototype%.reject.writable === true");
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.reject.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.reject.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %ModuleStatusPrototype%.reject === 'function'");
            assert.areEqual(2, descriptor.value.length, "%ModuleStatusPrototype%.reject.length === 2");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'stage');
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.stage.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.stage.configurable === true");
            assert.areEqual('function', typeof descriptor.get, "typeof %ModuleStatusPrototype%.stage.get === 'function'");
            assert.isTrue(descriptor.set === undefined, "%ModuleStatusPrototype%.stage.set === undefined");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'originalKey');
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.originalKey.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.originalKey.configurable === true");
            assert.areEqual('function', typeof descriptor.get, "typeof %ModuleStatusPrototype%.originalKey.get === 'function'");
            assert.isTrue(descriptor.set === undefined, "%ModuleStatusPrototype%.originalKey.set === undefined");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'module');
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.module.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.module.configurable === true");
            assert.areEqual('function', typeof descriptor.get, "typeof %ModuleStatusPrototype%.module.get === 'function'");
            assert.isTrue(descriptor.set === undefined, "%ModuleStatusPrototype%.module.set === undefined");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'error');
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.error.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.error.configurable === true");
            assert.areEqual('function', typeof descriptor.get, "typeof %ModuleStatusPrototype%.error.get === 'function'");
            assert.isTrue(descriptor.set === undefined, "%ModuleStatusPrototype%.error.set === undefined");

            var descriptor = Object.getOwnPropertyDescriptor(ModuleStatusPrototype, 'dependencies');
            assert.isFalse(descriptor.enumerable, "%ModuleStatusPrototype%.dependencies.enumerable === false");
            assert.isTrue(descriptor.configurable, "%ModuleStatusPrototype%.dependencies.configurable === true");
            assert.areEqual('function', typeof descriptor.get, "typeof %ModuleStatusPrototype%.dependencies.get === 'function'");
            assert.isTrue(descriptor.set === undefined, "%ModuleStatusPrototype%.dependencies.set === undefined");
            
            assert.isTrue(ModuleStatusPrototype.__proto__ === Object.prototype, "%ModuleStatusPrototype%.__proto__ === %ObjectPrototype%");
        }
    },
    {
        name: "%ModuleStatus% cannot be called as a function",
        body: function () {
            assert.throws(() => { Reflect.Module.Status(); }, TypeError, "%ModuleStatus% called without new keyword is type error", "ModuleStatus: cannot be called without the new keyword");
        }
    },
    {
        name: "ModuleStatus instance getter methods",
        body: function () {
            let key = "something";
            let loader = new Reflect.Loader();
            let entry = new Reflect.Module.Status(loader, key);
            
            assert.areEqual(key, entry.originalKey, "entry.originalKey === 'something'");
            assert.isFalse(entry.error, "entry.error === false");
            assert.areEqual('fetch', entry.stage, "entry.stage === 'fetch'");
        }
    },
    {
        name: "%ModuleStatus%.prototype.result",
        body: function () {
            let key = "something";
            let loader = new Reflect.Loader();
            let entry = new Reflect.Module.Status(loader, key);
            
            entry.result().catch(err => {
                testEquality(RangeError.prototype, err.__proto__, "%ModuleStatus%.prototype.result returns promise rejected with RangeError if stage is undefined");
            });
            entry.result('invalid').catch(err => {
                testEquality(RangeError.prototype, err.__proto__, "%ModuleStatus%.prototype.result returns promise rejected with RangeError if stage is invalid");
            });
            entry.result('fetch').then(result => {
                testEquality(undefined, result, "%ModuleStatus%.prototype.result returns promise resolved with undefined if stage is 'fetch'");
            });
            entry.result('instantiate').then(result => {
                testEquality(undefined, result, "%ModuleStatus%.prototype.result returns promise resolved with undefined if stage is 'instantiate'");
            });
            entry.result('translate').then(result => {
                testEquality(undefined, result, "%ModuleStatus%.prototype.result returns promise resolved with undefined if stage is 'translate'");
            });
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
