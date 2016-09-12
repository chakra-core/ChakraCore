//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Bug 272122
function evalCall()
{
    var z = "local";
    a.eval.call("var x='ineval'; z;");
    function foo()
    {
        z;
    }
}
function assign()
{
    // creating a as an alias for 'this'
    a = this; 
    evalCall(); /**bp:locals()**/
}
WScript.Attach(assign);
WScript.Echo("PASSED");