//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var log = Array(1000000);
var i = 0;

function test() {
    var cqjmyu;
    for (var wetavm = 0; wetavm < 1000; ++wetavm) {
        cqjmyu = new Uint16Array([1, 1, 1, 1, 1, 1, 1, 1, 1]);
        cqjmyu_0 = new Uint8ClampedArray(cqjmyu);
        cqjmyu_0[8] = "5";
        log[i++] = cqjmyu_0[0];
    }
    return cqjmyu[0];
}
for(var j =0;j<100;j++) test();
test();
test();
test();
test();
test();
test();
test();
test();
test();
test();
test();

var failed = false;
for(var k = 0; k < i; k++) {
    if(log[k] != 1) {
        WScript.Echo("failed at " + k);
        failed = true;
        break;
    }
}
if(!failed)
{
    WScript.Echo("PASSED");
}
