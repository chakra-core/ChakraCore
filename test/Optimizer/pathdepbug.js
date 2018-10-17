//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
function test0() {
  var arrObj0 = {};
  var func0 = function () {
    for (; arrObj0.prop1; ) {
      __loopSecondaryVar4_0 = 2;
      break;
    }
    return 1;
  };
  var func2 = function () {
    var __loopvar4 = 8;
    for (;;) {
      if (__loopvar4 > 8) {
        break;
      }
      __loopvar4++;
      func0() >= 0 ? func0() : 0;
    }
  };
  return func2(func2());
}
test0();
test0();
print("Passed\n");
