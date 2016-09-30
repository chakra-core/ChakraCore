//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Dynamic attach with multiple inner functions, eval code and try/catch block

WScript.Echo("In Global");

eval("var inEvalFunc1 = function () { return 'inEvalFunc1'; }");

function foo() {
    var a = 10;
    var f1 = function () {
        /*          */
    }

    WScript.Echo("In function foo");

    function g() {
        return 10;
    }
    g();

    try {
        a = "asfdssd"
    }
    catch (t) {
        var s = function (a, b) {
            eval('');
            return a + b;
        }
    }
}
foo();

WScript.Attach(foo);
