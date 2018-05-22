//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function func2() {
  for (; Object.prop1;) {}
}
var ary = [1];
var uic8 = new Uint8ClampedArray();
let i = 0;
while (ary[uic8[1] >= 0 ? uic8[1] : 0]) {
  func2();
  if(i++ > 1000) break;
}
print("passed");