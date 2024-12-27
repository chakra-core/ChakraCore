//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check
/// <reference path="../UnitTestFramework/UnitTestFramework.js" />

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const createObj = () => ({ "null": null, "undefined": undefined, something: 42 });

const tests = [
    {
        name: "`delete` should successfully delete from opt-chain",
        body() {
            const obj = createObj();

            assert.strictEqual(null, obj?.null);
            assert.isTrue(delete obj?.null);
            assert.strictEqual(undefined, obj?.null);

            assert.strictEqual(undefined, obj?.undefined);
            assert.isTrue(delete obj?.undefined);
            assert.strictEqual(undefined, obj?.undefined);

            assert.strictEqual(42, obj?.something);
            assert.isTrue(delete obj?.something);
            assert.strictEqual(undefined, obj?.something);
        }
    },
    {
        name: "`delete` should return `true` if opt-chain short-circuits",
        body() {
            const obj = createObj();

            assert.strictEqual(undefined, obj.doesNotExist);
            assert.strictEqual(undefined, obj.doesNotExist?.something);
            assert.isTrue(delete obj.doesNotExist?.something);
            assert.strictEqual(undefined, obj.doesNotExist?.something);

            assert.strictEqual(42, obj?.something);
            assert.strictEqual(undefined, obj?.something.doesNotExist);
            assert.isTrue(delete obj?.something.doesNotExist);
            assert.strictEqual(undefined, obj?.something.doesNotExist);

            assert.strictEqual(undefined, obj?.doesNotExist);
            assert.strictEqual(undefined, obj?.doesNotExist?.something);
            assert.isTrue(delete obj?.doesNotExist?.something);
            assert.strictEqual(undefined, obj?.doesNotExist?.something);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
