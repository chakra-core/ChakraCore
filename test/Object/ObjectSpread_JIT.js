//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Object Spread JIT unit tests

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "Test JIT basic behavior",
        body: function() {
            function f() {
                return {...{a: 1}};
            }
            
            f();
            let obj = f();

            assert.areEqual(1, obj.a);
        }
    },
    {
        name: "Test JIT bailout",
        body: function() {
            const obj = { a: 2 };
            function f(x) {
                const a = obj.a;
                const unused = {...x};
                return a + obj.a;
            }

            // Train it that ...x is not reentrant, so it emits code that assumes the second obj.a matches the first
            const result = f({});
            assert.areEqual(4, result);

            // Now call with a getter and verify that it bails out when the previous assumption is invalidated
            const reentrantResult = f({ get b() { obj.a = 3; } });
            assert.areEqual(5, reentrantResult);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
