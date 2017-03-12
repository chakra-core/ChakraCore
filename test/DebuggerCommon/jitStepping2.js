//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// OS 105921:Exprgen: Debugger: Difference in stack/locals between nonative and jit:
// step out/over needs to work as step-in
//
// When we STEP_OVER at the end of a script function, and the script function is a callback
// invoked again by a caller in native code, we should break at entry of the script function.
// Thus we should also bailout on STEP_OVER on entry of a jitted function.

WScript.Echo("pass"); // Only use .js.dbg.baseline

function bar() {
    var x = 0; /**bp:resume('step_out');resume('step_into');resume('step_over');stack();**/
}

var arr = [1, 2];
arr.forEach(function(){
    var a = 0;
    return bar();
});
