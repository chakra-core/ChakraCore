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

            assert.areEqual(true, delete arr.non_indexed, "Deleting own property should succeed");
            assert.areEqual(undefined, arr.non_indexed, "arr.non_indexed has been deleted");

            assert.areEqual(true, delete arr.with_getter, "Deleting own property with a getter should succeed");
            assert.areEqual(undefined, arr.with_getter, "arr.with_getter has been deleted");
        }
    },
    {
        name: "Deleting of non-configurable non-indexed properties on Arrays",
        body: function () {
            var arr = [1,4,9,16];
            var id = 'id';
            Object.defineProperty(arr, id, { value: 17, configurable: false });

            assert.areEqual(false, delete arr[id]);
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

            assert.areEqual(false, delete arr.length, "delete of arr.length should fail (as noop)");
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

            assert.areEqual(true, delete arr[1], "delete of arr[1] should succeed");
            assert.areEqual(undefined, arr[1], "arr[1] value after delete should be undefined");
            assert.areEqual(4, arr.length, "the array's lenght should not change");

            assert.areEqual(true, delete arr[42], "delete of arr[42] (beyond the array bounds) should succeed");
            assert.areEqual(4, arr.length, "the array's length is unchanged");

            const last = arr.length - 1;
            assert.areEqual(true, delete arr[last], "delete of last element should succeed");
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

            assert.areEqual(true, delete arr[1], "delete of arr[1] should succeed");
            assert.areEqual("arr.proto", arr[1], "arr[1] after deleting should be picked up from the Array prototype");

            assert.areEqual(true, delete arr[4], "delete of arr[4] should succeed");
            assert.areEqual("obj.proto", arr[4], "arr[4] after deleting should be picked up from the Object prototype");
            assert.areEqual(5, arr.length, "arr.length after deleting of the last element");
        }
    },
    {
        name: "Deleting of properties on frozen Arrays",
        body: function () {
            var arr = [42];
            arr.foo = 'fourty-two';

            Object.freeze(arr);

            // indexed property
            assert.areEqual(false, delete arr[0], "delete arr[0] from frozen array");
            assert.throws(function () { 'use strict'; delete arr[0]; }, TypeError, 
                "Should throw on delete of non-indexed property in array", 
                "Calling delete on '0' is not allowed in strict mode");

            // non-indexed property
            assert.areEqual(false, delete arr.foo, "delete arr.foo from frozen array");
            assert.throws(function () { 'use strict'; delete arr.foo; }, TypeError, 
                "Should throw on delete of non-indexed property in array", 
                "Calling delete on 'foo' is not allowed in strict mode");
        }
    },
    {
        name: "Deleting of indexed properties on sealed Arrays",
        body: function () {
            var arr = [42];
            arr.foo = 'fourty-two';

            Object.seal(arr);

            // indexed property
            assert.areEqual(false, delete arr[0], "delete arr[0] from sealed array");
            assert.throws(function () { 'use strict'; delete arr[0]; }, TypeError, 
                "Should throw on delete of non-indexed property in array", 
                "Calling delete on '0' is not allowed in strict mode");

            // non-indexed property
            assert.areEqual(false, delete arr.foo, "delete arr.foo from sealed array");
            assert.throws(function () { 'use strict'; delete arr.foo; }, TypeError, 
                "Should throw on delete of non-indexed property in array", 
                "Calling delete on 'foo' is not allowed in strict mode");
        }
    },
];

testRunner.runTests(tests, { verbose: false /*so no need to provide baseline*/ });