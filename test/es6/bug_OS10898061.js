//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo( a = b )  
{ 
    eval(""); 
    var b; 
} 

function bar( {a:a = b} )
{
    eval(""); 
    var b; 
} 


function test()
{
    try
    {
        // foo should throw a ReferenceError: 'b' is not defined.
        foo();
        return false;
    }
    catch( a )
    {}

    try
    {
        // bar should throw a ReferenceError: 'b' is not defined.
        bar({});
        return false;
    }
    catch( a )
    {}

    return true;
}

WScript.Echo(test() ? "PASSED" : "FAILED");
