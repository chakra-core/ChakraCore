//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {}
function test1() 
{ 
    var x="HELLO"; 
    var z= "test"; 
    var s = x+z;/**bp:locals()**/
}
WScript.Attach(test1);
WScript.Detach(test1);
WScript.Echo("PASSED");