//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Exploring Arguments object ES6
*/


function foo() {
    var map = new Map();
    map.set();
	WScript.Echo(map.size);
    map.forEach(function () {
        arguments; /**loc(bp1):
			stack();
			locals();
            **/
        WScript.Echo(arguments.callee.caller);
    })
}

function Run() {
    foo();
    foo();
    foo();/**bp:enableBp('bp1')**/
	foo; /**bp:disableBp('bp1')**/
}




WScript.Attach(Run)
