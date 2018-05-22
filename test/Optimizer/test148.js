//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var obj0 = {};
  var func1 = function () {
    return 'caller';
  };
  var func3 = function () {
  };
  obj0.method1 = func1;
  var ary = Array();
  var __loopvar1 = 9;
  for (;;) {
    if (__loopvar1 > 9) {
      break;
    }
    __loopvar1++;
    var uniqobj12 = { 13: ((func3.call(obj0.method1(), 'caller')), func1()) };
  }
  '' + ary.slice();
}
test0();
test0();
print("passed");