//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that function declaration bindings show correctly for the
// global function case.

f;/**bp:locals()**/
{
    function f() { }
    f(); /**bp:locals()**/
}
f();/**bp:locals()**/

WScript.Echo("PASSED");