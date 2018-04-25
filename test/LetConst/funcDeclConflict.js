//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function f1_f(){};
var f2_v = 123;
function f3_f(){};
var f4_v = 123;

let f5_l = 123;
const f6_c = 123;

var tests = [
    {
        name: "Top level function in script does not conflict with another top level function",
        body: function () {
            WScript.LoadScript("function f1_f(){};");
        }
    },
    {
        name: "Top level function in script does not conflict with same-named var",
        body: function () {
            WScript.LoadScript("function f2_v(){};");
        }
    },
    {
        name: "Top level function in eval does not conflict with another top level function",
        body: function () {
           eval("function f3_f(){};");
        }
    },
    {
        name: "Top level function in eval does not conflict with same-named var",
        body: function () {
            eval("function f4_v(){};");
        }
    },
    {
        name: "Top level function in script conflicts with top level let",
        body: function () {
            assert.throws(()=>WScript.LoadScript("function f5_l(){};"),  ReferenceError, "Let/Const redeclaration");
        }
    },
    {
        name: "Top level function in script conflicts with top level const",
        body: function () {
            assert.throws(()=>WScript.LoadScript("function f6_c(){};"),  ReferenceError, "Let/Const redeclaration");
        }
    },
    {
        name: "Top level function in eval conflicts with top level let",
        body: function () {
            assert.throws(()=>eval("function f5_l(){};"),  ReferenceError, "Let/Const redeclaration");
        }
    },
    {
        name: "Top level function in eval conflicts with top level const",
        body: function () {
            assert.throws(()=>eval("function f6_c(){};"),  ReferenceError, "Let/Const redeclaration");
        }
    },
    {
        name: "Top level function in eval conflicts with top level const, in a class",        
        body: function () {
            class C1
            {                
                static M()
                {
                    assert.throws(()=>eval("function f6_c(){};"),  ReferenceError, "Let/Const redeclaration");
                }
            }

            C1.M();
        }
    },
    {
        name: "Top level function in eval does not conflict with class level get",        
        body: function () {
            class C1
            {                
                get f7_l() {return "q";};

                static M()
                {
                    eval("function f7_l(){};");
                }
            }

            C1.M();
        }
    },
]


WScript.RegisterModuleSource("mod0.js", `
    import 'mod1.js';
    let qwer = 12;
`);

WScript.RegisterModuleSource("mod1.js",`
    // no redeclaration here since modules are not introducing global names.
    export function qwer(){};
`);

WScript.LoadScriptFile("mod0.js", "module");

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
