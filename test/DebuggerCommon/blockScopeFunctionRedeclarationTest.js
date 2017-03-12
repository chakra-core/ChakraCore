//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that redeclaration of a function works
// and doesn't throw an assert.
// Bug 325723.

function test0() {
    if (0) { }
    else {
        function test1(a) {
            return a;
        }
        function test1(a) {
            return a;
        }

        test1();
        /**bp:locals()**/
    }
};

test0();

WScript.Echo("PASSED")