//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validate the dynamic attach with forcedeferparse

eval("var inEvalFunc1 = function () { return 'inEvalFunc1'; }");

function foo() {
    var a = 10;
    function f1() {
        function f11() {
            return a;       /**bp:locals(1)**/
        }
        f11();
    }
    f1();

    function g() {
        return 10;
    }
    g();                  /**bp:locals(1)**/

    try {
        abc.def = 10;
    }
    catch (t) {
        var s = function (a, b) {
            eval('');
            return a + b;
        }
        t;                      /**bp:locals(1)**/
    }

    WScript.Echo("Pass");
}

CollectGarbage();

WScript.Attach(foo);
