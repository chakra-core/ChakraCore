//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/**exception(resume_break):resume('step_over');**/
//Switches:  -debuglaunch -targeted -dbgbaseline   -maxinterpretcount:1
function test0() {
    var obj0 = {};
    var arrObj0 = {};
    arrObj0.method0 = function () {
        arrObj0.prop0 += 1 /**bp(loc0):setFrame(1);evaluate('CollectGarbage();CollectGarbage();');**/
    }
    var l = 1;
    var m = 65536;
    var q = 1;
    l = (m * (1 - m));
    q ^= arrObj0.method0.call(obj0);
};
test0();
test0();

WScript.Echo("pass");
