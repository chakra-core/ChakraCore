//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
 foo->bar->baz
 Stepping over baz in bar
 Stepping out to foo from bar
*/



function foo() {
    let x = 1;
    bar();
}

function bar() {
    let x = 2; 
    baz();/**loc(bp1):
        locals(1);
        resume('step_over')
        **/

    /*
        ;
        locals(1)
        resume('step_out');
        locals(1)

    */
    x;
}

function baz() {
    let x = 3;
    x++; 
    x;
}

function Run() {
    foo();
    foo();
    //debug JIT
    foo(); /**bp:enableBp('bp1')**/
    foo; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}

foo();
foo();
//JIT
WScript.Attach(Run);
