//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo() {
    var x = 1; /**bp:dumpSourceList();**/
    WScript.Echo("foo");
}
// Attach twice, detach twice
WScript.Attach(foo);
WScript.Attach(foo);
WScript.Detach(foo);
WScript.Detach(foo);
WScript.Echo("Pass");
