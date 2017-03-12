//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that the block scope slot array is present when attempting
// to retrieve it after the scope has escaped.  Also used for testing that
// bytecode deserialization properly recreates the block scope slot array
// for a function (when -forceserialized is passed).
// Bug #219563.

var g;

function test() {
    {
        let a = 0;
        {
            g = function inner() {
                a++; 
                a;/**bp:locals(1)**/
            }
        }
    }
}

test();
WScript.Attach(g);
WScript.Detach(g);
WScript.Echo("PASSED")