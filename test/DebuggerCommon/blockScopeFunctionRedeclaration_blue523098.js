//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
    (function () {
        function foo() {
            eval("");
        };
    })();
    {
        var x = 1;/**bp:locals();**/
        function bar() {}
        function bar() { return 1;}
    }
};
test0();

WScript.Echo("PASS");
