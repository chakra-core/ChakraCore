//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


let test = (function ()
{
    "use asm";
    function brbool(a, b, c) {
        a = a|0;
        b = b|0;
        c = c|0;
        var d = 0;
        var e = 0;
        d = (a|0) == (b|0);
        e = (d|0) == (c|0);
        if((d & e)|0)
        {
            return 1;
        }
        return 2;
    }
    return brbool;
})()
if(test(1,1,1) == 1 && test(1,2,3) == 2)
{
    print("PASS");
}
else
{
    print("FAIL");
}