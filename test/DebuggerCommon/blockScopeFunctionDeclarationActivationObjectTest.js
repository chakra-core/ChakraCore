//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that function declaration bindings show correctly for the
// activation object case.

function test() {
    var a = 1;
    {
        a; /**bp:locals()**/
        function f() { }
        eval("f()");
        a; /**bp:locals()**/
    }
    a;/**bp:locals()**/
}

test();
WScript.Echo("PASSED")