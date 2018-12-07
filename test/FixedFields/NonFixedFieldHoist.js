//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(a,b){ return a + b; }
function bar(a,b){ return a - b; }

var obj = {};
obj.foo = foo;

function test(obj)
{
    var count = 0;

    for (var i = 0; i < 1000; i++)
    {
        count += obj.foo(10, i);
    }

    return count;
}

obj.foo = bar;
WScript.Echo(test(obj));
obj.foo = 10;
try
{
    WScript.Echo(test(obj));
}
catch(e)
{
    WScript.Echo(e);
}

var obj2 = {};
try
{
    WScript.Echo(test(obj2));
}
catch(e)
{
    WScript.Echo(e);
}
