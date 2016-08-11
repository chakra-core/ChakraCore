//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "%Registry% has the correct shape",
        body: function () {
            var Registry = new Reflect.Loader().registry.constructor;

            var descriptor = Object.getOwnPropertyDescriptor(Registry, 'prototype');
            assert.isFalse(descriptor.writable, "%Registry%.prototype.writable === false");
            assert.isFalse(descriptor.enumerable, "%Registry%.prototype.enumerable === false");
            assert.isFalse(descriptor.configurable, "%Registry%.prototype.configurable === false");
            assert.areEqual('object', typeof descriptor.value, "typeof %Registry%.prototype === 'object'");

            assert.isTrue(Registry.__proto__ === Function.prototype, "%Registry%.__proto__ === %FunctionPrototype%");
        }
    },
    {
        name: "%RegistryPrototype% has the correct shape",
        body: function () {
            var Registry = new Reflect.Loader().registry.constructor;
            var RegistryPrototype = Registry.prototype;

            var descriptor = Object.getOwnPropertyDescriptor(RegistryPrototype, 'constructor');
            assert.isFalse(descriptor.writable, "%RegistryPrototype%.constructor.writable === false");
            assert.isFalse(descriptor.enumerable, "%RegistryPrototype%.constructor.enumerable === false");
            assert.isTrue(descriptor.configurable, "%RegistryPrototype%.constructor.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %RegistryPrototype%.constructor === 'function'");
            assert.isTrue(RegistryPrototype.constructor === Registry, "%RegistryPrototype%.constructor === %Registry%");
            
            var descriptor = Object.getOwnPropertyDescriptor(RegistryPrototype, Symbol.iterator);
            assert.isTrue(descriptor.writable, "%RegistryPrototype%[@@iterator].writable === true");
            assert.isFalse(descriptor.enumerable, "%RegistryPrototype%[@@iterator].enumerable === false");
            assert.isTrue(descriptor.configurable, "%RegistryPrototype%[@@iterator].configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %RegistryPrototype%[@@iterator] === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(RegistryPrototype, 'entries');
            assert.isTrue(descriptor.writable, "%RegistryPrototype%.entries.writable === true");
            assert.isFalse(descriptor.enumerable, "%RegistryPrototype%.entries.enumerable === false");
            assert.isTrue(descriptor.configurable, "%RegistryPrototype%.entries.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %RegistryPrototype%.entries === 'function'");
            
            assert.areEqual(RegistryPrototype.entries, RegistryPrototype[Symbol.iterator], "RegistryPrototype.entries === RegistryPrototype[@@iterator]");

            var descriptor = Object.getOwnPropertyDescriptor(RegistryPrototype, 'keys');
            assert.isTrue(descriptor.writable, "%RegistryPrototype%.keys.writable === true");
            assert.isFalse(descriptor.enumerable, "%RegistryPrototype%.keys.enumerable === false");
            assert.isTrue(descriptor.configurable, "%RegistryPrototype%.keys.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %RegistryPrototype%.keys === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(RegistryPrototype, 'values');
            assert.isTrue(descriptor.writable, "%RegistryPrototype%.values.writable === true");
            assert.isFalse(descriptor.enumerable, "%RegistryPrototype%.values.enumerable === false");
            assert.isTrue(descriptor.configurable, "%RegistryPrototype%.values.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %RegistryPrototype%.values === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(RegistryPrototype, 'get');
            assert.isTrue(descriptor.writable, "%RegistryPrototype%.get.writable === true");
            assert.isFalse(descriptor.enumerable, "%RegistryPrototype%.get.enumerable === false");
            assert.isTrue(descriptor.configurable, "%RegistryPrototype%.get.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %RegistryPrototype%.get === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(RegistryPrototype, 'set');
            assert.isTrue(descriptor.writable, "%RegistryPrototype%.set.writable === true");
            assert.isFalse(descriptor.enumerable, "%RegistryPrototype%.set.enumerable === false");
            assert.isTrue(descriptor.configurable, "%RegistryPrototype%.set.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %RegistryPrototype%.set === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(RegistryPrototype, 'has');
            assert.isTrue(descriptor.writable, "%RegistryPrototype%.has.writable === true");
            assert.isFalse(descriptor.enumerable, "%RegistryPrototype%.has.enumerable === false");
            assert.isTrue(descriptor.configurable, "%RegistryPrototype%.has.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %RegistryPrototype%.has === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(RegistryPrototype, 'delete');
            assert.isTrue(descriptor.writable, "%RegistryPrototype%.delete.writable === true");
            assert.isFalse(descriptor.enumerable, "%RegistryPrototype%.delete.enumerable === false");
            assert.isTrue(descriptor.configurable, "%RegistryPrototype%.delete.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %RegistryPrototype%.delete === 'function'");
            
            assert.isTrue(RegistryPrototype.__proto__ === Object.prototype, "%RegistryPrototype%.__proto__ === %ObjectPrototype%");
        }
    },
    {
        name: "%Registry% cannot be called as a function or as part of a construct expression",
        body: function () {
            var Registry = new Reflect.Loader().registry.constructor;
            
            assert.throws(() => { Registry(); }, TypeError, "%Registry% called without new keyword is type error", "Registry is not intended to be called as a function or as a constructor");
            assert.throws(() => { new Registry(); }, TypeError, "%Registry% called with new keyword is type error", "Registry is not intended to be called as a function or as a constructor");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
