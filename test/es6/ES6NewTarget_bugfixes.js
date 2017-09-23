//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "OS4497597: ScopeInfo::FromScope() should increment scope symbol count to accomodate 'new.target'",
        body: function () {
            (function (){
                function f() {}
                eval("");
                () =>new.target;
            })();
            // Repro:
            // ASSERTION : (jscript\core\lib\Runtime\ByteCode\ScopeInfo.h, line 68)
            // Failure: (i >= 0 && i < symbolCount)
        }
    },
    {
        name: "OS5427497: Parser mistakes 'new.target' as in global function under -forceundodefer",
        body: function () {
            new.target;  // bug repro: SyntaxError: Invalid use of the 'new.target' keyword
        }
    },
    {
        name: "OS8806229: eval in default parameter of arrow function",
        body: function() {
            assert.doesNotThrow(()=>(function() { (a = eval(undefined)) => {}; }));
        }
    },
    {
        name: "[MSRC35208] parameter type confusion in eval",
        body: function ()
        {
            var proxy = new Proxy(eval, {});
            assert.areEqual(0, proxy("Math.sin(0)"));
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
