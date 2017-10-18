//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

try
{
    // Issue Microsoft/ChakraCore#3900
    var o = { 0: 1, 1: 1 };
    JSON.parse('[0,0]', function () {
        this[1] = o;
    });
}
catch (ex)
{
    if (ex.message == "Out of stack space")
    {
        console.log("PASS");
    }
}