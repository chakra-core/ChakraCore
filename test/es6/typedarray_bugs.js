//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "TypedArray.prototype.filter species create order issue",
    body: function () {
        var a = new Int8Array(2);
        var speciesCalled = false;
        Object.defineProperty(a.constructor, Symbol.species, { get : function () { speciesCalled = true; return Int8Array; } });
        function mapFn() {
            throw new Error('Error from mapFn');
        }
        assert.throws(() => a.filter(mapFn), Error, 'Error should be thrown from the mapFn', 'Error from mapFn');
        assert.isFalse(speciesCalled, 'species should not be called as the mapFn throws error');
    }
  },
  {
    name: "TypedArray.prototype.join empty typedarray still evaluate the param",
    body: function () {
        var count = 0;
        var obj = { toString : function () { count++; return ','; } };
        var a = new Int8Array();
        a.join(obj);
        assert.areEqual(count, 1, "a's length is 0 but it should evaluate obj");
        
        count = 0;
        a = new Int8Array(1);
        a.join(obj);
        assert.areEqual(count, 1, "a'length is 1 but it should evaluate obj");
    }
  },
  {
    name: "TypedArray.prototype.join passing 'undefined' should not print that",
    body: function () {
        var a = new Int8Array([11, 22]);
        var ret = a.join(undefined);
        assert.areEqual(ret, "11,22", "join should not join the literal 'undefined' string while joining.");
    }
  },
  {
    name: "TypedArray.prototype.keys/entries/values accept only TypedArray object",
    body: function () {
        function test(fn) {
            var name = fn.name;
            assert.throws(() => fn(), TypeError, name + " function throws when no param passed", "'this' is not a typed array object");
            assert.throws(() => fn.call(), TypeError, name + " function throws when no param passed", "'this' is not a typed array object");
            assert.throws(() => fn.call({}), TypeError, name + " function throws when no TypedArray object passed", "'this' is not a typed array object");
            assert.throws(() => fn.call(new ArrayBuffer(1)), TypeError, name + " function throws when no TypedArray object passed", "'this' is not a typed array object");
        }
        test(Int8Array.prototype.keys);
        test(Int8Array.prototype.values);
        test(Int8Array.prototype.entries);
    }
  },
  
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
