//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f() {
    return "f";
}

function g() {
    return "g";
}

// Step to var o statement, then step into each of the calls to f and g
// verifying that stepping into and out of calls contained in computed
// properties behaves correctly and the stack trace is sane.
// This case is at the global scope.
f; /**bp:resume('step_into');
     stack();resume('step_into');
     stack();resume('step_out');
     resume('step_into');

     stack();resume('step_into');
     stack();resume('step_out');

     stack();resume('step_into');
     stack();resume('step_out');

     stack();
     **/
var o = {
    [f()]: 1,
    [f() + g()]: 2
}

function test() {
    // Verify stepping and the stack again, as above, but in function scope.
    f; /**bp:resume('step_into');
         stack();resume('step_into');
         stack();resume('step_out');
         resume('step_into');

         stack();resume('step_into');
         stack();resume('step_out');

         stack();resume('step_into');
         stack();resume('step_out');

         stack();
         **/
    var o = {
        [f()]: 1,
        [f() + g()]: 2
    }
}

test();

WScript.Echo("passed");
