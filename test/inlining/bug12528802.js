//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var obj = { foo : function() {} };

function bar(arg)
{
    obj.foo.apply(obj, arguments);
    let local;
    let baz = function() { local; };
}

function test()
{
    bar();
}

test();
test();
test();

WScript.Echo("PASSED");
