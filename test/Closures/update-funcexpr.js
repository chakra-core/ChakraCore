//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Functions can overwrite themselves",
        body: function () {
            function foo1() {
                foo1 = 42;
                assert.isTrue(typeof foo1 == "number", "foo1 should overwrite itself to a number");
                assert.areEqual(42, foo1, "value of foo1 after assignment");
            }
            foo1();

            function foo2() {
                foo2 &= 0;
                assert.isTrue(typeof foo2 == "number", "foo2 should overwrite itself to a number");
                assert.areEqual(0, foo2, "value of foo2 after assignment");
            }
            foo2();

            function foo3() {
                foo3 <<= 0;
                assert.isTrue(typeof foo3 == "number", "foo3 should overwrite itself to a number");
                assert.areEqual(0, foo3, "value of foo3 after assignment");
            }
            foo3();

            function foo4() {
                let x = foo4++;
                assert.isTrue(isNaN(x), "post-increment should return NaN");
                assert.isTrue(isNaN(foo4), "foo4 should overwrite itself");
            }
            foo4();

            function foo5() {
                ++foo5;
                assert.isTrue(isNaN(foo5), "foo5 should overwrite itself");
            }
            foo5();
        }
    },
    {
        name: "Function expressions cannot overwrite themselves",
        body: function () {
            (function foo1() {
                foo1 = 42;
                assert.isTrue(typeof foo1 == "function", "foo1 should not overwrite itself");
            })();

            (function foo2() {
                foo2 &= 0;
                assert.isTrue(typeof foo2 == "function", "foo2 should not overwrite itself");
            })();

            (function foo3() {
                foo3 <<= 0;
                assert.isTrue(typeof foo3 == "function", "foo3 should not overwrite itself");
            })();

            (function foo4() {
                let x = foo4++;
                assert.isTrue(isNaN(x), "post-increment should return NaN");
                assert.isTrue(typeof foo4 == "function", "foo4 should not overwrite itself");
            })();

            (function foo5() {
                ++foo5;
                assert.isTrue(typeof foo5 == "function", "foo5 should not overwrite itself");
            })();
        }
    }
];

testRunner.runTests(tests, { verbose: false /*so no need to provide baseline*/ });