//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var __loopvar0 = 6;
  do {
    if (__loopvar0 >= 10) {
      break;
    }
    __loopvar0 += 2;
    var v0 = 0;
    var v1 = Math.clz32('caller');
    while (v0 < 5) {
      v1 = Math.clz32('caller');
      v0++;
    }
    b = Math.clz32('caller') + v1;
  } while (true);
  WScript.Echo(b);
}
test0();
test0();
