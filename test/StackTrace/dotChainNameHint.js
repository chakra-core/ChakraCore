//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (typeof (WScript) != "undefined") {
    WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "OS#14711878: Throw SOE (not crash) in Parser::ConstructNameHint (OSS-Fuzz test case)",
        body: function ()
        {
            assert.throws(function () {
                // This block is derived directly from the OSS-Fuzz test case.
                const M = 1e6;
                var u;
                for (var i = 0; i < M; i++) {
                    u = u + ".prototype";
                }
                eval(u);
            }, Error, "Should throw SOE (not crash with SOE) in Parser::ConstructNameHint", "Out of stack space");
        }
    },
    {
        name: "OS#14711878: Throw SOE (not crash) in Parser::ConstructNameHint (more 'normal' test case)",
        body: function ()
        {
            assert.throws(function () {
                // There is nothing special about the names/patterns used in the above test case.
                // This bug is strictly about SOE caused by stack depth from chaining the dot `.` operator.
                const M = 1e6;
                var u = "foo"; // explicit name
                for (var i = 0; i < M; i++) {
                    u = u + ".a"; // not a special property
                }
                eval(u);
            }, Error, "Should throw SOE (not crash with SOE) in Parser::ConstructNameHint", "Out of stack space");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
