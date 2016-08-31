//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Step out from JITted function
*/
foo();
foo();


function foo() {
    var a = "foo";
    var b = new Array();
    var c = arguments;
    bar();
}

function bar() {
    var a = "bar";
    var b = [];
    var c = new Date();
    c;  /**loc(bp1):
        stack();
        locals(1);
        resume('step_out')
        **/

    /*
       ;
        stack();
        locals(1)
    */
}

function Run() {
    foo();
    foo();
    //foo & bar are debug JITted
    foo();/**bp:enableBp('bp1')**/
    foo; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}

//foo();
//foo();
//Both foo and bar are JITted
WScript.Attach(Run);





