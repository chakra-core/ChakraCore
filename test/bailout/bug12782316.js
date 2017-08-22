//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var o1 = {};
var o2 = Object.create(o1);
o2.x = 1;

function foo( arg ) 
{
    this.caller;
    arg.x && o1.y + o2.y; 
    arg.x;
};

foo(o2);
foo(o2);
Object.defineProperty(o2, 'y', { get: function () {} });
foo(o2);
o1.x = {};
foo(o1);
Object.defineProperty(o1, 'y', { set: function () {} });
foo(o2)

WScript.Echo("PASSED");