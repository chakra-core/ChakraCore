//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

Object.prototype.sort = function(){}

arr = [];

var passed = 1;
for (var i in arr)
{
    passed = 0;
}

if (passed)
{
    WScript.Echo("PASS");
}
