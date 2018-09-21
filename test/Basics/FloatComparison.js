//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

// This tests the fast path for cmxx where either src is type specialized to float
var tests = [
    {
        name: "NaN equality test",
        body: function() {
            assert.isTrue(NaN !== NaN);
            assert.isTrue(NaN !== 0.5);
            assert.isTrue(0.5 !== NaN);
        
            assert.isTrue(NaN != NaN);
            assert.isTrue(NaN != 0.5);
            assert.isTrue(0.5 != NaN);
        
            assert.isFalse(NaN === NaN);
            assert.isFalse(NaN === 0.5);
            assert.isFalse(0.5 === NaN);
        
            assert.isFalse(NaN == NaN);
            assert.isFalse(NaN == 0.5);
            assert.isFalse(0.5 == NaN);

            assert.isFalse(NaN > 0.5);
            assert.isFalse(NaN >= 0.5);
            assert.isFalse(NaN < 0.5);
            assert.isFalse(NaN <= 0.5);
        }
    },
    {
        name: "Type coercion test",
        body: function() {
            assert.isTrue('0.5' == 0.5);
            assert.isFalse('0.5' === 0.5);
            assert.isFalse('NaN' == NaN);
            assert.isTrue('NaN' != NaN);
        }
    },
    {
        name: "int vs. float",
        body: function() {
            assert.isFalse(5 == 0.5);
            assert.isTrue(5 != 0.5);
            assert.isFalse(5 === 0.5);
            assert.isTrue(5 !== 0.5);
        }
    },
    {
        name: "object vs. float",
        body: function() {
            assert.isFalse({} == 0.5);
            assert.isFalse({} === 0.5);
            assert.isTrue({} != 0.5);
            assert.isTrue({} !== 0.5);
        
            assert.isFalse({} > 0.5);
            assert.isFalse({} >= 0.5);
            assert.isFalse({} < 0.5);
            assert.isFalse({} <= 0.5);
        }
    },
    {
        name: "No float type specialization when operands are not number",
        body: function() {
            function test0() {
                var func2 = function () {
                  return f32[1];
                };
                var f32 = new Float32Array();
                var f = 100;
                for (let i = 0; i < f; i++) {
                    var id41 = 'caller';
                    ({ 18: func2() === 'caller' });
                }
            }
            test0();
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
