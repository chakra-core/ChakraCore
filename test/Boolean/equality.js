//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
}
var __loopvar1 = 4;
for (;;) {
  if (__loopvar1 === 4 + 9) {
    break;
  }
  __loopvar1 += 3;
  function v5() {
  }
  v5.prototype.method0 = function () {
    prop3 = [] instanceof Error == (test0.caller & 255);
  };
  var v6 = new v5();
  function v7() {
  }
  v7.prototype.method0 = function () {
  };
  var v8 = new v7();
  var v10 = new v7();
  function v16(v17) {
    v17.method0();
  }
  v16(v8);
  v16(v10);
  v16(v6);
}
if(prop3) {
  print("PASSED");
} else {
  print("FAILED");
}
