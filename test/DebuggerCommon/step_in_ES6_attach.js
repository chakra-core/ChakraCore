//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    foo->bar->baz
    stepping into baz from bar
    Stepping out to foo from bar
    Observe locals
    Contains let/const variables
*/

function foo() {
    var a = "foo";
    let _a = "foo_let";
    const __a = "foo_const";
    bar();
}

function bar() {
    var a = "bar";
    let _a = "bar_let";
    const __a = "bar_const";
    baz(); /**loc(bp1):
        resume('step_into');
        resume('step_over');
        resume('step_over');
        locals(1);
        resume('step_out');
        locals(1);
        resume('step_out');
        locals(1)**/
}

function baz() {
    let x = 1;
    x++; 
}

function Run() {
    foo();
    foo();
    //Debug JIT 
    foo();/**bp:enableBp('bp1')**/
    foo; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}


//JIT
WScript.Attach(Run); 
WScript.Detach(Run);


