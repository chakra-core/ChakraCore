//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(x,y)
{
    const c = x + 1;
    const d = y + 1;
    x = (d + (c << 0))|0;
    x += (d + (c << 1))|0;
    x += (d + (c << 2))|0;
    x += (d + (c << 3))|0;
    x += (d + (c << 4))|0;
    return x;
}
print(foo(2,3));
print(foo(2,3));

print(foo(-12346578,3));
print(foo(2,12346578));