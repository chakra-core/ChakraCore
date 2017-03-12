//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let/const variables declared at global scope
// where one is in a slot array doesn't cause both to be placed
// into a slot array slot (only 'a' should be).  This was causing
// an assert in TrackSlotArrayPropertyForDebugger since the slot array
// index for b was -1.
// Bug #222631.

try {
    throw "level1";
} catch(e) {	
    eval(""); 	
    with({  }) {
        let a = "level2";
        const b = "level2";
        var c = function f() {  				
            a += "level3";
            /**bp:locals(1)**/
        };
        c();
        a;
        b;/**bp:locals(1)**/
    }
}

WScript.Echo("PASSED");