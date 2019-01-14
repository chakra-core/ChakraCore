//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var o = { x: 1 };
var run = false;

function invalidate() {
    // guard invalidation starts, stack walk happens
    o.x = 2;
}

function test(o)
{
    if (run)
    {
        invalidate();
    }

    var a = o.x;
    print(a);
    return o.x;
}

print(test(o));
run = true;
print(test(o));
