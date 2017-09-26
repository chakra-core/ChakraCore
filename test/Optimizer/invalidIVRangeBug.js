//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var GiantPrintArray = [];
  var obj0 = {};
  var IntArr0 = [];
  var IntArr1 = [];
  var VarArr0 = [obj0];
  var e = -649211448;
  var f = 137044716;
  var protoObj1 = Object();
  function v0(v1) {
    var v4 = {};
    v4.a = v1;
    v4.a[1] = null;
  }
  GiantPrintArray.push(v0(IntArr0));
  for (var _strvar26 in VarArr0) {
    for (; IntArr1.push(); f++) {
    }
    for (var _strvar0 in IntArr0) {
      f = (e > _strvar0 & 255);
    }
  }
  protoObj1.prop5 = { prop3: !f };
  return protoObj1.prop5.prop3;
}

var x = test0();
x &= test0();
x &= test0();

if (x == true) {
  WScript.Echo("PASSED");
}
else {
  WScript.Echo("FAILED");
}
