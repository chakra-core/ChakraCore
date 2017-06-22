//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var obj = 
{
    foo : function() {},
}

function test()
{
    obj.foo.apply();
}

function test1()
{
    test();
}

test1();
test1();
test1();

WScript.Echo("PASSED");