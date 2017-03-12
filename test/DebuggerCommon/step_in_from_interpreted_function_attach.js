//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Step in from Interpreted function
*/

function foo() {
    var a = "foo";
    var b = new Array();
    var c = arguments;
    bar(); /**loc(bp1):
        resume('step_into');
        resume('step_over');
        resume('step_over');
        resume('step_over');
        stack();
        locals(2) 
        **/
}

function bar() {
    var a = "bar";
    var b = [];
    var c = new Date();   
}

function Run() {
    bar();
    bar();
    //Only bar is JITted
    //foo is still interpreted
    //foo->bar
    foo();/**bp:enableBp('bp1')**/
    foo; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}

bar();
bar();
//Only bar is JITted
WScript.Attach(Run);
