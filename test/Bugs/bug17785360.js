//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

arr = new Uint8Array(0x40000);
var obj = {x : 1.1};
function test2()
{
    return obj.x;
}
function test()
{
    function test1()
    {
        for(var i=0; i < arr.length; i++)
        {
            arr[i] = arr[i+1] = arr[i+2] = Math.floor(test2() / 4294967295 * 128), arr[i + 3] = 255;
        }
    }
    test1(arr);
}

test();
print("passed");