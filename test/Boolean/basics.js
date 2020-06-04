//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const tests = [
    {
        name : "Simple",
        body() {
            var f = false;
            var t = true;
            assert.isTrue(!f, "!f should be true");
            assert.isTrue(!!!f, "!!!f should be true");
            assert.isTrue(t, "t should be true");
            assert.isTrue(!!t, "!!t should be true");   
        }
    },
    {
        name : "Equals",
        body() {
            var a = [true, false, new Boolean(true), new Boolean(false)];
            var b = [true, false, new Boolean(true), new Boolean(false), -1, 0, 1, 2, 1.0, 1.1, 0.0, +0, -0, 
                null, undefined, new Object(), "", "abc", "-1", "0", "1", "2", "true", "false", "t", "f",
                "True", "False", " 1.00 ", " 1. ", " +1.0 ", new Number(0), new Number(1)];

            var results = [true, false, true, false, false, false, true, false, true, false,
                false, false, false, false, false, false, false, false, false, false, true,
                false, false, false, false, false, false, false, true, true, true, false,
                true, false, true, false, true, false, true, false, false, false, false,
                true, true, true, false, false, false, true, false, false, true, false,
                false, false, false, false, false, false, false, false, false, false, true,
                false, true, false, false, false, false, false, true, false, true, false,
                false, false, false, false, false, false, false, false, false, false, true,
                false, false, false, false, false, false, false, true, true, true, false,
                false, false, true, false, false, false, true, false, false, false, false,
                true, true, true, false, false, false, true, false, false, true, false,
                false, false, false, false, false, false, false, false, false, false, false,
                false];

            for (var i = 0, k = 0; i < a.length; i++)
            {
                for (var j = 0; j < b.length; j++, k++)
                {
                    assert.areEqual(a[i] == b[j], results[k], `${a[i]} == ${b[j]} should evaluate to ${results[k]}`);
                }
            }
        }
    },
    {
        name : "Wrapped Object",
        body() {
            var f = new Boolean(false);
            assert.isTrue(f == false, "new Boolean(false) should == false");
            assert.isTrue(f !== false, "new Boolean(false) should !== false");
            assert.isTrue(!f == false, "!(new Boolean(false)) should == false");
        }
    },
    {
        name : "Boolean values generated with ! outside of a conditional",
        body() {
            var q = new Object();
            var tests = [-0.5, -1, 1, 2, 3, new Object(), q, [4,5,6], "blah", 'c', true];

            for (var x in tests) {
                assert.isFalse(!tests[x], `!${tests[x]} should evaluate to false`);
            }
            assert.isTrue(!0 && !false, "!0 && !false should evaluate to true");
        }
    },
    {
        name : "Value producing comparisons",
        body() {
            var lhs = [1, 2, 2, -1, 1, 0, 0,   0x70000000, 0];
            var rhs = [2, 1, 2, 2, -2, 0, 0.1, 0,          0x70000000];
            var results = [
                [true, true, false, false, false, true, false, true],
                [false, false, true, true, false, true, false, true],
                [false, true, false, true, true, false, true, false],
                [true, true, false, false, false, true, false, true],
                [false, false, true, true, false, true, false, true],
                [false, true, false, true, true, false, true, false],
                [true, true, false, false, false, true, false, true],
                [false, false, true, true, false, true, false, true],
                [true, true, false, false, false, true, false, true]
            ];
            for (var i = 0; i < 9; ++i)
            {
                assert.areEqual(lhs[i] < rhs[i],   results[i][0], `Expected ${lhs[i]} < ${rhs[i]} to equal ${results[i][0]}`);
                assert.areEqual(lhs[i] <= rhs[i],  results[i][1], `Expected ${lhs[i]} <= ${rhs[i]} to equal ${results[i][1]}`);
                assert.areEqual(lhs[i] > rhs[i],   results[i][2], `Expected ${lhs[i]} > ${rhs[i]} to equal ${results[i][2]}`);
                assert.areEqual(lhs[i] >= rhs[i],  results[i][3], `Expected ${lhs[i]} >= ${rhs[i]} to equal ${results[i][3]}`);
                assert.areEqual(lhs[i] == rhs[i],  results[i][4], `Expected ${lhs[i]} == ${rhs[i]} to equal ${results[i][4]}`);
                assert.areEqual(lhs[i] != rhs[i],  results[i][5], `Expected ${lhs[i]} != ${rhs[i]} to equal ${results[i][5]}`);
                assert.areEqual(lhs[i] === rhs[i], results[i][6], `Expected ${lhs[i]} === ${rhs[i]} to equal ${results[i][6]}`);
                assert.areEqual(lhs[i] !== rhs[i], results[i][7], `Expected ${lhs[i]} !== ${rhs[i]} to equal ${results[i][7]}`);
            }
        }
    },
    {
        name: "Assignment to a property on a boolean without a setter in sloppy mode should be ignored",
        body: function ()
        {
            var flag = true;
            flag.a = 12;
            assert.areEqual(undefined, flag.a);
        }
    },
    {
        name: "Assignment to a property on a boolean without a setter in strict mode should throw an error",
        body: function ()
        {
            var flag = true;
            assert.throws(function() { "use strict"; flag.a = 1; }, TypeError, "Assigning to a property of a number should throw a TypeError.", "Assignment to read-only properties is not allowed in strict mode");
        }
    },
    {
        name: "Assignment to a property on a boolean without a setter in sloppy mode should be ignored",
        body: function ()
        {
            var flag = true;
            flag['a'] = 12;
            assert.areEqual(undefined, flag.a);
        }
    },
    {
        name: "Assignment to a property on a boolean without a setter in strict mode should throw an error",
        body: function ()
        {
            var flag = true;
            assert.throws(function() { "use strict"; flag['a'] = 1; }, TypeError, "Assigning to a property of a number should throw a TypeError.", "Assignment to read-only properties is not allowed in strict mode");
        }
    },
    {
        name: "Assignment to an index on a boolean without a setter in sloppy mode should be ignored",
        body: function ()
        {
            var flag = true;
            flag[66] = 12;
            assert.areEqual(undefined, flag.a);
        }
    },
    {
        name: "Assignment to an index on a boolean without a setter in strict mode should throw an error",
        body: function ()
        {
            var flag = true;
            assert.throws(function() { "use strict"; flag[66] = 1; }, TypeError, "Assigning to a property of a number should throw a TypeError.", "Assignment to read-only properties is not allowed in strict mode");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
