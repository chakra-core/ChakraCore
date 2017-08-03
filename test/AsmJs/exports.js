//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function m() {
    "use asm";

    function f(a)
    {
        a = a|0;
        return a|0;
    }
    
    function g()
    {
        return 10;
    }
    // have more than 128 values
    return {
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
        f:f, f:g, f:f, f:g,
    };
}
print(m().f());