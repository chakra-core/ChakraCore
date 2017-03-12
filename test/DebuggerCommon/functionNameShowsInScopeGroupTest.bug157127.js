//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Bug #157127: Tests that the function name displays properly in the locals window "value" column.
function foo() {
    var f1;
    function g() {
        x = f1; /**bp:locals()**/
    }
    g();
}
foo();
WScript.Echo("PASSED");