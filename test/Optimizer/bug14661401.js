//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var loopInvariant = 44;
  var obj0 = {};
  
  while (typeof obj0.prop1) {
    var __loopvar1 = loopInvariant, __loopSecondaryVar1_1 = loopInvariant - 14;
    do {
      __loopvar1 += 4;
    } while (typeof obj0.prop1 !== typeof obj0.prop1);
    break;
  }
}
test0();
test0();
print("passed");