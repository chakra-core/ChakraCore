//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Step over JITted function
    from an Interpreted function
*/


var callcount = 0;

function foo() {
    var _foo = "foo";
    callcount++;
    if (callcount == 5)
        bar();
    else
        baz();
        
}

function bar() {
    var _bar = [];
    baz();/**loc(bp1):
        resume('step_over');
        locals(1);
        resume('step_out');
        stack()
        **/
    _bar;
}

function baz(){
    var x = 3;
    x++; 
}

function Run() {
    foo();
    foo();
    //baz is now debug JITted
    foo(); /**bp:enableBp('bp1')**/
    foo; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}

foo();
foo();
//baz in JITted
WScript.Attach(Run);
WScript.Detach(Run);
