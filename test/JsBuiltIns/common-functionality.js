//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

testRunner.runTests([
    {
        name: "Helpers should not show up in stack traces",
        body() {
            for (const builtin of [Array.prototype.forEach, Array.prototype.filter, Array.prototype.flatMap]) {
                assert.isTrue(typeof(builtin.name) === "string" && builtin.name.length > 0, `Test requires builtin.name to be set for ${builtin.toString()}`);
                try {
                    builtin.call([1, 2, 3], function callback() { throw new Error("error in callback") });
                    assert.isTrue(false, `Exception swallowed from callback for ${builtin.name}`);
                } catch (e) {
                    const frames = e.stack.split("\n");
                    assert.isTrue(/error in callback/.test(frames[0]), `Invalid first frame "${frames[0]}" for ${builtin.name}`);
                    assert.isTrue(/at callback \(/.test(frames[1]), `Invalid second frame "${frames[1]}" for ${builtin.name}`);
                    assert.isTrue(new RegExp(`at Array.prototype.${builtin.name} \\(native code\\)`, "i").test(frames[2]), `Invalid third frame "${frames[2]}" for ${builtin.name}`);
                    assert.isTrue(/at body \(/.test(frames[3]), `Invalid fourth frame "${frames[3]}" for ${builtin.name}`);
                }
            }
        }
    },
    {
        name: "(Existing) JsBuiltIns shouldn't be constructable",
        body() {
            for (const builtin of [
                Array.prototype.values,
                Array.prototype.entries,
                Array.prototype.keys,
                Array.prototype.indexOf,
                Array.prototype.forEach,
                Array.prototype.filter,
                Array.prototype.flat,
                Array.prototype.flatMap,
                Object.fromEntries,
            ]) {
                assert.isTrue(typeof(builtin.name) === "string" && builtin.name.length > 0, `Test requires builtin.name to be set for ${builtin.toString()}`);
                assert.throws(() => new builtin(), TypeError, `${builtin.name} should not be constructable (using new)`, "Function is not a constructor");
                assert.throws(() => Reflect.construct(builtin, []), TypeError, `${builtin.name} should not be constructable (using Reflect.construct target)`, "'target' is not a constructor");
                assert.throws(() => Reflect.construct(function(){}, [], builtin), TypeError, `${builtin.name} should not be constructable (using Reflect.construct newTarget)`, "'newTarget' is not a constructor");
            }
        }
    },
], { verbose: false });
