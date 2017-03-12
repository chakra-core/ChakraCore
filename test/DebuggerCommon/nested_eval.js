//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validation of the bug 594173. Calling eval in the debug eval raises the onAddChild to debugger which leads to hang.

function foo() {
    var i = 30;
    i++; /**bp:evaluate("var k = i; eval('k')")**/
}
foo();

WScript.Echo("Pass");
