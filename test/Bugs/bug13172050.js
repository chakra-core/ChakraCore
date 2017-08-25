//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo()
{
    function blah() {}
}

function bar(f)
{
    for (var i = 0; i < 1; i++)
    {
        if (f)
        {
            f();
        }
    }
}

bar(foo);
bar(foo);
bar();

WScript.Echo("PASSED");