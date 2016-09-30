//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that function declaration bindings show correctly for the
// slot array case.

function test() {
    var a = 1;
    {
        a; /**bp:locals()**/
        function f() { }
        function g() {
            f();
        }
        g();
        a; /**bp:locals()**/
    }
    a;/**bp:locals()**/
}

test();
WScript.Echo("PASSED")