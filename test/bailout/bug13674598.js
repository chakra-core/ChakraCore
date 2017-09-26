//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// BailOutOnMulOverflow should restore the pre-op value of 'b'. If it does not, 'a' will be assigned the wrong value.
var a;
function foo()
{
    var b = 1073741824 | undefined;
    try
    {
        b *= 2;
    }
    catch(e)
    {

    }

    a = b;
}

foo();
foo();
foo();

WScript.Echo( a == 2147483648 ? "PASSED" : "FAILED");