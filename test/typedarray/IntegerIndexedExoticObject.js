//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Verifies TypedArray builtin properties

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var TypedArrayCtors = [
    Int8Array,
    Uint8Array,
    Uint8ClampedArray,
    Int16Array,
    Uint16Array,
    Int32Array,
    Uint32Array,
    Float32Array,
    Float64Array
];

var tests = [
    {
        name: "Non-integer numeric index on TypedArray",
        body: function () {
            TypedArrayCtors.forEach(function(TA) {
                var o = new TA(10);
                Object.prototype[1.3] = 100;

                assert.areEqual(undefined, o[1.3], "");
                assert.areEqual(false, Reflect.has(o, '1.3'), "");

                var p = new Proxy(o, {});
                assert.areEqual(undefined, p[1.3], "");
                assert.areEqual(false, Reflect.has(p, '1.3'), "");

                delete Object.prototype[1.3];
            });
        }
    },
    {
        name: "Non-integer numeric index on Object with TypedArray.prototype as prototype",
        body: function () {
            TypedArrayCtors.forEach(function(TA) {
                var o = {};
                Object.setPrototypeOf(o, new TA(100));
                Object.prototype[1.3] = 100;

                assert.areEqual(undefined, o[1.3], "");
                assert.areEqual(false, Reflect.has(o, '1.3'), "");

                var p = new Proxy(o, {});
                assert.areEqual(undefined, p[1.3], "");
                assert.areEqual(false, Reflect.has(p, '1.3'), "");

                Object.setPrototypeOf(o, new Proxy(Object.getPrototypeOf(o), {}));
                assert.areEqual(undefined, o[1.3], "");
                assert.areEqual(false, Reflect.has(o, '1.3'), "");
                assert.areEqual(undefined, p[1.3], "");
                assert.areEqual(false, Reflect.has(p, '1.3'), "");

                delete Object.prototype[1.3];
            });
        }
    },
    {
        name: "NaN index on TypedArray",
        body: function () {
            TypedArrayCtors.forEach(function(TA) {
                var o = new TA(10);
                Object.prototype[NaN] = 100;

                assert.areEqual(undefined, o[NaN], "");
                assert.areEqual(false, Reflect.has(o, 'NaN'), "");

                delete Object.prototype[NaN];
            });
        }
    },
    {
        name: "Non-integer index on TypedArray with object prototype",
        body: function () {
            TypedArrayCtors.forEach(function(TA) {
                var arr = new TA();
                arr[1.2] = "xyz123"; // fails to assign (read below is undefined)

                Object.setPrototypeOf(arr, {
                    "1.1": "abc"
                });

                assert.areEqual(undefined, arr[1.1], "");
                assert.areEqual(undefined, arr[1.2], "");
            });
        }
    },
    {
        name: "Non-integer indexed method on TypedArray's object prototype",
        body: function () {
            TypedArrayCtors.forEach(function(TA) {
                var tests = new TA(3);
                Object.prototype[1.3] = function() { throw Error("OrdinaryGet called"); };

                var i = '1.3';
                assert.throws(()=>tests[i](), TypeError, "function defined in prototype should not be called", "Object doesn't support property or method '1.3'");

                delete Object.prototype[1.3];
            });
        }
    },
    {
        name: "Numeric indices on TypedArray",
        body: function () {
            var taProto = Object.getPrototypeOf(Uint8Array).prototype;
            var throwError = {configurable: true, get: function() { throw Error("OrdinaryGet called"); }  };

            TypedArrayCtors.forEach(function(TA) {
                var a = new TA(2);
                var props = [
                    "-0",
                    0.1,
                    "0.1",
                    0.000001,
                    "0.000001",
                    1.1,
                    "1.1",
                    Infinity,
                    "Infinity",
                    -Infinity,
                    "-Infinity",
                    -1,
                    "-1",
                    2,
                    "2",
                    3,
                    "3",
                    NaN,
                    "NaN"];

                var testProp = function(prop) {
                    Object.defineProperty(taProto, prop, throwError);

                    assert.areEqual(0, a[0], TA.name+" "+prop+" a[0]");
                    assert.areEqual(0, a[1], TA.name+" "+prop+" a[1]");
                    assert.areEqual(undefined, a[prop], TA.name+" "+prop+" a[prop]");

                    assert.areEqual(false, Reflect.has(a, prop), TA.name+" "+prop+" Reflect.has(a, prop)");
                    assert.areEqual(false, Reflect.defineProperty(a, prop, {value: 100, configurable: false, enumerable: true, writable: true}), TA.name+" "+prop+" Reflect.defineProperty");
                    assert.areEqual(false, Reflect.set(a, prop), TA.name+" "+prop+" Reflect.set(a, prop)");
                    assert.areEqual(false, Reflect.hasOwnProperty(a, prop), TA.name+" "+prop+" Reflect.hasOwnProperty(a, prop)");

                    delete taProto[prop];
                };

                props.forEach(testProp);
            });
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
