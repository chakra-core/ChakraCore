//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const o = {
    get prop() {
        assert.fail("Should not be called");
    }
}

var tests = [
    {
        name: "Object properties used as left-side for nullish operator",
        body () {
            const obj = {
                nullvalue: null,
                undefinedvalue: undefined,
                number: 123,
                zero: 0,
                falsevalue: false,
                emptystring: ''
            };
            const other = 'other';

            assert.areEqual(other, obj.nullvalue ?? other);
            assert.areEqual(other, obj.undefinedvalue ?? other);
            assert.areEqual(other, obj.notdefinedvalue ?? other);

            assert.areEqual(obj.number, obj.number ?? other);
            assert.areEqual(obj.zero, obj.zero ?? other);
            assert.areEqual(obj.falsevalue, obj.falsevalue ?? other);
            assert.areEqual(obj.emptystring, obj.emptystring ?? other);
        }
    },
    {
        name: "Primitive values used as left-side for nullish operator",
        body () {
            const other = 'other';

            assert.areEqual(other, null ?? other);
            assert.areEqual(other, undefined ?? other);

            assert.areEqual(123, 123 ?? other);
            assert.areEqual(0, 0 ?? other);
            assert.areEqual(false, false ?? other);
            assert.areEqual('', '' ?? other);
        }
    },
    {
        name: "Names used as left-side for nullish operator",
        body () {
            const nullvalue = null;
            const undefinedvalue = undefined;
            const number = 123;
            const zero = 0;
            const falsevalue = false;
            const emptystring = '';
            const other = 'other';

            assert.areEqual(other, nullvalue ?? other);
            assert.areEqual(other, undefinedvalue ?? other);

            assert.areEqual(number, number ?? other);
            assert.areEqual(zero, zero ?? other);
            assert.areEqual(falsevalue, falsevalue ?? other);
            assert.areEqual(emptystring, emptystring ?? other);
        }
    },
    {
        name: "Right-hand side is evaluated only if needed (short-circuiting)",
        body () {
            assert.areEqual('not null', 'not null' ?? o.prop);
        }
    },
    {
        name : "?? interaction with ||",
        body () {
            assert.areEqual(5, null ?? (5 || 4));
            assert.areEqual(5, 5 ?? (1 || 4));
            assert.areEqual(5, (null ?? 5) || 4);
            assert.areEqual(5, (5 ?? null) || 4);
            assert.areEqual(5, (5 ?? null) || o.prop);
        }
    },
    {
        name : "?? interaction with &&",
        body() {
            assert.areEqual(5, null ?? (2 && 5));
            assert.areEqual(5, (null ?? 2) && 5);
            assert.areEqual(5, 2 && (5 ?? o.prop ?? o.prop));
            assert.areEqual(5, 2 && (3 ?? o.prop) && 5);
            assert.areEqual(null, null && (null ?? 5));
        }  
    },
    {
        name: "?? cannot be used within || or && without parentheses",
        body() {
            function parse (code) {
                try {
                    eval ("function test() {" + code + "}");
                    return true;
                } catch {
                    return false;
                }
            }
            assert.isFalse(parse("a ?? b || c"));
            assert.isFalse(parse("a || b ?? c"));
            assert.isFalse(parse("a ?? b && c"));
            assert.isFalse(parse("a && b ?? c"));
            assert.isTrue(parse("a && (b ?? c)"));
            assert.isTrue(parse("(a && b) ?? c"));
            assert.isTrue(parse("a && (b ?? c)"));
            assert.isTrue(parse("a || (b ?? c)"));
            assert.isTrue(parse("(a || b) ?? c"));
            assert.isTrue(parse("a || (b ?? c)"));
            assert.isFalse(parse("a ?? (b ?? c) || a"));
            assert.isTrue(parse("(a ?? b ?? c) || a"));
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
