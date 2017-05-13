//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var IntArr1 = [];
  e = 0;
  c = 30;
  for (var v0 = 0; v0 < 4; ++v0) {
    e = c * 10;
    c += 1.1;
  }
  IntArr1[e] = 10;
  return IntArr1.length === 0;
}
test0();
if(!test0()) {
  print("FAILED");
} else {
  print("PASSED");
}
