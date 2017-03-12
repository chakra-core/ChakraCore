//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Step out from JITted function
*/


function Run() {
    var a = "foo"; 
    var b = new Array();
    var c = arguments;
    bar();   
}

function bar() {
    var a = "bar";
    var b = [];
    var c = new Date();
    c;  /**bp:
        stack();
        locals(1);
        resume('step_out');
        stack();
        locals(1)
        **/
}

//No JIT only attach
WScript.Attach(Run);
WScript.Detach(Run);
WScript.Attach(Run);

WScript.Echo('PASSED');



