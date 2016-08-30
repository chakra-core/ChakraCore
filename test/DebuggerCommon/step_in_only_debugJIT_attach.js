//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Step in from JITted function
    Assuming no previous JIT during attach
*/



var callcount = 0;

function foo() {

    function bar() {
        var d = { y: 2 };
        callcount++;
        with (d) {
            y;/**loc(bp2):stack()**/
        }
        WScript.Echo(1);
    }
    bar();
    WScript.Echo(2);
}


function Run() {
    foo();
    foo();
    //Both foo and bar are debug JITted
    foo();/**bp:enableBp('bp2')**/
	foo; /**bp:disableBp('bp2')**/
}

WScript.Attach(Run);
WScript.Detach(Run);



