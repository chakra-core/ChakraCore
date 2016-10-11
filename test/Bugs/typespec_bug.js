//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var f = 2147483647;
var func1 = function (argMath3) {
    if ( f < argMath3)
    {
        f++;
    }
};
func1(3);
func1(4);
func1(4702209150613300000);
if (f == 2147483648)
{
    WScript.Echo("Passed");
}
else
{
    WScript.Echo("Fail");
}
