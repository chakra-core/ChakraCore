//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var __counter = 0;
function test0() {
  __counter++;
  var obj0 = {};
  var arrObj0 = {};
  var func0 = function () {
  };
  var func1 = function () {
  };
  var func2 = function () {
    for (; func1(ui16[218361093] >= 0 ? ui16[218361093] : 0); func1((false ? arrObj0 : undefined))) {
    }
  };
  obj0.method1 = func0;
  arrObj0.method1 = func2;
  var ui16 = new Uint16Array();
  var uniqobj1 = [
    obj0,
    arrObj0
  ];
  var uniqobj2 = uniqobj1[__counter % uniqobj1.length];
  uniqobj2.method1();
  for (var _strvar5 of ui16) {
  }
}
test0();
test0();
test0();
WScript.Echo("Passed");
