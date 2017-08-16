//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo( a = b ) 
{
    eval("");
    var b;
}

try
{
    foo();
    WScript.Echo( "FAILED");
}
catch( a )
{
    WScript.Echo( "PASSED");
}
