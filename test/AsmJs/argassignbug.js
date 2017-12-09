//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function m()
{
    "use asm"
    function a(x, y)
    {
        x = x|0
        y = y|0
        return (x + y)|0
    }
    function b()
    {
        var x = 1
        var y = 2
        return a(x, (x=y, y)|0)|0
    }
    return b
}

let result = m()()
print((result == 3) ? "Pass" : "Fail")
