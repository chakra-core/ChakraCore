//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Step in from JITted function
*/
foo();
foo();

function foo() {
    var a = "foo";
    var b = new Array();
    var c = arguments;
    bar(); /**loc(bp1):
        resume('step_into');
        stack();
        resume('step_over')
        **/
    /*
        loc(bp1):
        resume('step_into');
        stack();
        resume('step_over');
    */
}

function bar() {
    var a = "bar";
    var b = [];
    var c = new Date();
    return function () {
        var x = 1; /**bp:locals(1);resume('step_out');stack()**/
    }
}

function Run() {
    foo();
    foo();
    //foo & bar are debug JITted
    //foo->bar
    foo();/**bp:enableBp('bp1')**/
    foo; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}


//Both foo and bar are JITted
WScript.Attach(Run);
WScript.Detach(Run);
WScript.Attach(Run);








