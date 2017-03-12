//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Stepping out from interpreted 
    function into debug JITted function 
*/



var callcount = 0;

function foo() {
    var _foo = "foo";
    var _fooargs = arguments;
    bar();
   
}

function bar() {
    var _bar = [];
    var _barargs = arguments;
    callcount++;
    if(callcount == 5 )
        baz();
}

//baz is not JITted
function baz() {   
    var x = 1;
    x++;
    x;/**loc(bp1):
        stack();
        locals(1);
        resume('step_out');
        stack();
        locals(1);
        resume('step_out');
        locals(1);
        **/
}

function Run() {
    foo();
    foo();
    //Debug JIT
    foo();/**bp:enableBp('bp1')**/
    foo; /**bp:disableBp('bp1')**/
    WScript.Echo('PASSED');
}

foo();
foo();
//JIT
WScript.Attach(Run);







