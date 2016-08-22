//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "%Loader% has the correct shape",
        body: function () {
            var Loader = Reflect.Loader;
           
            var descriptor = Object.getOwnPropertyDescriptor(Reflect, 'Loader');
            assert.isFalse(descriptor.writable, "Reflect.Loader.writable === false");
            assert.isFalse(descriptor.enumerable, "Reflect.Loader.enumerable === false");
            assert.isTrue(descriptor.configurable, "Reflect.Loader.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof Reflect.Loader === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(Loader, 'prototype');
            assert.isFalse(descriptor.writable, "Loader.prototype.writable === false");
            assert.isFalse(descriptor.enumerable, "Loader.prototype.enumerable === false");
            assert.isFalse(descriptor.configurable, "Loader.prototype.configurable === false");
            assert.areEqual('object', typeof descriptor.value, "typeof Loader.prototype === 'object'");
            
            var descriptor = Object.getOwnPropertyDescriptor(Loader, 'resolve');
            assert.isFalse(descriptor.writable, "Loader.resolve.writable === false");
            assert.isFalse(descriptor.enumerable, "Loader.resolve.enumerable === false");
            assert.isFalse(descriptor.configurable, "Loader.resolve.configurable === false");
            assert.areEqual('symbol', typeof descriptor.value, "typeof Loader.resolve === 'symbol'");
            assert.areEqual('Symbol(Reflect.Loader.resolve)', new Object(descriptor.value).toString(), "descriptor.value === Symbol(Reflect.Loader.resolve)");
            
            var descriptor = Object.getOwnPropertyDescriptor(Loader, 'fetch');
            assert.isFalse(descriptor.writable, "Loader.fetch.writable === false");
            assert.isFalse(descriptor.enumerable, "Loader.fetch.enumerable === false");
            assert.isFalse(descriptor.configurable, "Loader.fetch.configurable === false");
            assert.areEqual('symbol', typeof descriptor.value, "typeof Loader.fetch === 'symbol'");
            assert.areEqual('Symbol(Reflect.Loader.fetch)', new Object(descriptor.value).toString(), "descriptor.value === Symbol(Reflect.Loader.fetch)");
            
            var descriptor = Object.getOwnPropertyDescriptor(Loader, 'translate');
            assert.isFalse(descriptor.writable, "Loader.translate.writable === false");
            assert.isFalse(descriptor.enumerable, "Loader.translate.enumerable === false");
            assert.isFalse(descriptor.configurable, "Loader.translate.configurable === false");
            assert.areEqual('symbol', typeof descriptor.value, "typeof Loader.translate === 'symbol'");
            assert.areEqual('Symbol(Reflect.Loader.translate)', new Object(descriptor.value).toString(), "descriptor.value === Symbol(Reflect.Loader.translate)");
            
            var descriptor = Object.getOwnPropertyDescriptor(Loader, 'instantiate');
            assert.isFalse(descriptor.writable, "Loader.instantiate.writable === false");
            assert.isFalse(descriptor.enumerable, "Loader.instantiate.enumerable === false");
            assert.isFalse(descriptor.configurable, "Loader.instantiate.configurable === false");
            assert.areEqual('symbol', typeof descriptor.value, "typeof Loader.instantiate === 'symbol'");
            assert.areEqual('Symbol(Reflect.Loader.instantiate)', new Object(descriptor.value).toString(), "descriptor.value === Symbol(Reflect.Loader.instantiate)");

            assert.isTrue(Loader.__proto__ === Function.prototype, "Loader.__proto__ === %FunctionPrototype%");
        }
    },
    {
        name: "%LoaderPrototype% has the correct shape",
        body: function () {
            var Loader = Reflect.Loader;
            var LoaderPrototype = Loader.prototype;

            var descriptor = Object.getOwnPropertyDescriptor(LoaderPrototype, 'constructor');
            assert.isFalse(descriptor.writable, "%LoaderPrototype%.constructor.writable === false");
            assert.isFalse(descriptor.enumerable, "%LoaderPrototype%.constructor.enumerable === false");
            assert.isTrue(descriptor.configurable, "%LoaderPrototype%.constructor.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %LoaderPrototype%.constructor === 'function'");
            assert.isTrue(LoaderPrototype.constructor === Loader, "%LoaderPrototype%.constructor === %Loader%");
            
            var descriptor = Object.getOwnPropertyDescriptor(LoaderPrototype, 'import');
            assert.isTrue(descriptor.writable, "%LoaderPrototype%.import.writable === true");
            assert.isFalse(descriptor.enumerable, "%LoaderPrototype%.import.enumerable === false");
            assert.isTrue(descriptor.configurable, "%LoaderPrototype%.import.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %LoaderPrototype%.import === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(LoaderPrototype, 'resolve');
            assert.isTrue(descriptor.writable, "%LoaderPrototype%.resolve.writable === true");
            assert.isFalse(descriptor.enumerable, "%LoaderPrototype%.resolve.enumerable === false");
            assert.isTrue(descriptor.configurable, "%LoaderPrototype%.resolve.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %LoaderPrototype%.resolve === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(LoaderPrototype, 'load');
            assert.isTrue(descriptor.writable, "%LoaderPrototype%.load.writable === true");
            assert.isFalse(descriptor.enumerable, "%LoaderPrototype%.load.enumerable === false");
            assert.isTrue(descriptor.configurable, "%LoaderPrototype%.load.configurable === true");
            assert.areEqual('function', typeof descriptor.value, "typeof %LoaderPrototype%.load === 'function'");

            var descriptor = Object.getOwnPropertyDescriptor(LoaderPrototype, 'registry');
            assert.isFalse(descriptor.enumerable, "%LoaderPrototype%.registry.enumerable === false");
            assert.isTrue(descriptor.configurable, "%LoaderPrototype%.registry.configurable === true");
            assert.areEqual('function', typeof descriptor.get, "typeof %LoaderPrototype%.registry.get === 'function'");
            assert.isTrue(descriptor.set === undefined, "%LoaderPrototype%.registry.set === undefined");

            var descriptor = Object.getOwnPropertyDescriptor(LoaderPrototype, Symbol.toStringTag);
            assert.isFalse(descriptor.writable, "%LoaderPrototype%[@@toStringTag].writable === false");
            assert.isFalse(descriptor.enumerable, "%LoaderPrototype%[@@toStringTag].enumerable === false");
            assert.isTrue(descriptor.configurable, "%LoaderPrototype%[@@toStringTag].configurable === true");
            assert.areEqual('string', typeof descriptor.value, "typeof %LoaderPrototype%[@@toStringTag] === 'string'");
            assert.areEqual('Object', descriptor.value, "%LoaderPrototype%[@@toStringTag] === 'Object'");
            
            assert.isTrue(LoaderPrototype.__proto__ === Object.prototype, "%LoaderPrototype%.__proto__ === %ObjectPrototype%");
        }
    },
    {
        name: "%Loader% cannot be called as a function",
        body: function () {
            var Loader = Reflect.Loader;
            
            assert.throws(() => { Loader(); }, TypeError, "Loader called without new keyword is type error", "Loader: cannot be called without the new keyword");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
