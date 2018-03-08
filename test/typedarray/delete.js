//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
    this.WScript.LoadScriptFile("util.js");
}

var tests = [
    {
        name: "Typed arrays support delete of non-indexed properties",
        body: function () {
            const ta = Int8Array.of(42);

            ta.non_indexed = 'whatever';
            assert.areEqual('whatever', ta.non_indexed, "ta.non_indexed is set to 'whatever'");

            delete ta.non_indexed;
            assert.areEqual(undefined, ta.non_indexed, "ta.non_indexed has been deleted");
        }
    },
    {
        name: "delete of nonconfigurable non-indexed properties on Typed arrays",
        body: function () {
            const ta = Int8Array.of(42);
            var id = 'id';
            Object.defineProperty(ta, id, { value: 17, configurable: false });

            delete ta[id];
            assert.areEqual(17, ta[id], "ta['id'] value after failed delete");

            assert.throws(function () { 'use strict'; delete ta[id]; }, TypeError, "Should throw on delete of indexed property in typed array", "Calling delete on 'id' is not allowed in strict mode");
        }
    },
    {
        name: "Typed arrays don't support delete of indexed properties",
        body: function () {
            const ta = Int8Array.of(42);

            delete ta[0];
            assert.areEqual(42, ta[0], "ta[0] value after failed delete");

            assert.throws(function () { 'use strict'; delete ta[0]; }, TypeError, "Should throw on delete of indexed property in typed array", "Calling delete on '0' is not allowed in strict mode");
        }
    }
];

testRunner.runTests(tests, { verbose: false /*so no need to provide baseline*/ });
