//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "OS17417473: hoisting a constant propertySymbol should not trigger assertion",
        body: function () {
            function opt() {
                let _1337;
                s = "";
                s = "" & s.hasOwnProperty("");
                var s = 0x1337;
                for (i =0; i < 1; i++) {
                    _1337 = s.hasOwnProperty("x");
                }
            }
            
            // trigger the full jit
            for (let i = 0; i < 100; i++) {
                opt();
            }
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
