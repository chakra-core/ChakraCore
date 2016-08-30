//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Inspecting frames - Map ES6
*/

function foo() {
    let map = new Map();
    map.set();
    bar(map);
    function bar(map) {
        var x = 1; /**loc(bp1):
        evaluate('map',3);
        evaluate('map.set()', 2);
        setFrame(1);
        evaluate('map.size==2');
        evaluate('map.clear()');
        stack();
        **/
    }

    map; /**loc(bp2):evaluate('map', 3)**/
    WScript.Echo('PASSED');
}

function Run() {
    foo();
    foo()
    foo()/**bp:enableBp('bp1');enableBp('bp2')**/;
	foo; /**bp:disableBp('bp1')**/
}

WScript.Attach(Run);
WScript.Detach(Run);
WScript.Attach(Run);
