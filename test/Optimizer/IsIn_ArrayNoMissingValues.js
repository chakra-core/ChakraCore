//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = [1,2,3,4,5];

function foo(ary)
{
    var filled = 0;
    for (var ii = 0; ii < ary.length; ++ii)
    {
        if (ii in ary)
        {
            ++filled;
        }
    }
    return filled;
}

foo(a);
foo(a);
foo(a);
