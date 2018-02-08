//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(a, b = (function() {a;})())
{
    for (var ii = 0; ii < 200; ++ii)
    {
        var c, d = null;
        function bar()
        {
            c;
            d;
        };
        bar();
    }
};

foo();

WScript.Echo("Pass")
