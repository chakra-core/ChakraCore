//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

'use strict';

if (this.WScript && this.WScript.LoadScriptFile) {
    this.WScript.LoadScriptFile("../UnitTestFramework/TrimStackTracePath.js");
}

var o = {};

function baz()
{
    o.p &= undefined;
}

function bar()
{
    var a = 1;
    baz();
}

function foo()
{
    var a = 1;
    var b = 1;
    bar();
}

try
{
    foo()
    foo()
    Object.defineProperty(o, 'p', {writable: false});
    foo();
}
catch(ex)
{
    WScript.Echo(TrimStackTracePath(ex.stack));
}
