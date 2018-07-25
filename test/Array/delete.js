//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Deleting of configurable non-indexed properties on Arrays",
        body: function () {
            var arr = [1,4,9,16];

            arr.non_indexed = 'whatever';
            Object.defineProperty(arr, 'with_getter', { get: function() { return 'with getter'; }, configurable: true });

            assert.areEqual('whatever', arr.non_indexed, "arr.non_indexed is set to 'whatever'");
            assert.areEqual('with getter', arr.with_getter, "arr.with_getter is set to 'with getter'");

            var res = delete arr.non_indexed;
            assert.areEqual(true, res, "Deleting own property should succeed");
            assert.areEqual(undefined, arr.non_indexed, "arr.non_indexed has been deleted");

            var res = delete arr.with_getter;
            assert.areEqual(true, res, "Deleting own property with a getter should succeed");
            assert.areEqual(undefined, arr.with_getter, "arr.with_getter has been deleted");
        }
    },
    {
        name: "Deleting of non-configurable non-indexed properties on Arrays",
        body: function () {
            var arr = [1,4,9,16];
            var id = 'id';
            Object.defineProperty(arr, id, { value: 17, configurable: false });

            var res = delete arr[id];
            assert.areEqual(false, res);
            assert.areEqual(17, arr[id], "arr['id'] value after failed delete");

            assert.throws(function () { 'use strict'; delete arr[id]; }, TypeError, 
                "Should throw on delete of non-indexed property in array", 
                "Calling delete on 'id' is not allowed in strict mode");
        }
    },
    {
        name: "Deleting of the 'length' property on Arrays",
        body: function () {
            var arr = [1,4,9,16];

            var res = delete arr.length;
            assert.areEqual(false, res, "delete of arr.length should fail (as noop)");
            assert.areEqual(4, arr.length, "arr.length after attempting to delete it");

            assert.throws(function () { 'use strict'; delete arr.length; }, TypeError, 
                "Should throw on delete of 'length' property in array", 
                "Calling delete on 'length' is not allowed in strict mode");
            assert.areEqual(4, arr.length, "arr.length after attempting to delete it in strict mode");
        }
    },
    {
        name: "Deleting of indexed properties on Arrays",
        body: function () {
            var arr = [1,4,9,16];

            var res = delete arr[1];
            assert.areEqual(true, res, "delete of arr[1] should succeed");
            assert.areEqual(undefined, arr[1], "arr[1] value after delete should be undefined");
            assert.areEqual(4, arr.length, "the array's lenght should not change");

            res = delete arr[42];
            assert.areEqual(true, res, "delete of arr[42] (beyond the array bounds) should succeed");
            assert.areEqual(4, arr.length, "the array's length is unchanged");

            const last = arr.length - 1;
            res = delete arr[last];
            assert.areEqual(true, res, "delete of last element should succeed");
            assert.areEqual(undefined, arr[last], "arr[last] value after delete should be undefined");
            assert.areEqual(4, arr.length, "the array's lenght should not change");
        }
    },
    {
        name: "Deleting of indexed properties on Arrays that are also set on prototypes",
        body: function () {
            Object.prototype[4]  = "obj.proto";
            Array.prototype[1]   = "arr.proto";
            var arr = [1,4,9,16,25];

            var res = delete arr[1];
            assert.areEqual(true, res, "delete of arr[1] should succeed");
            assert.areEqual("arr.proto", arr[1], "arr[1] after deleting should be picked up from the Array prototype");

            var res = delete arr[4];
            assert.areEqual(true, res, "delete of arr[4] should succeed");
            assert.areEqual("obj.proto", arr[4], "arr[4] after deleting should be picked up from the Object prototype");
            assert.areEqual(5, arr.length, "arr.length after deleting of the last element");
        }
    },
];

testRunner.runTests(tests, { verbose: false /*so no need to provide baseline*/ });