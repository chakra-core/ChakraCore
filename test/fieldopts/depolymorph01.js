//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var obj0 = {};
  var arrObj0 = {};
  var litObj0 = { prop1: 3.14159265358979 };
  var func1 = function (argMath1 = func0(), argMath3) {
    protoObj0.prop0 = ++this.prop0;
    this.prop0 = argMath3;
  };
  obj0.method0 = func1;
  obj0.method1 = obj0.method0;
  var IntArr0 = Array();
  var protoObj0 = Object.create(obj0);
  prop0 = 1;
  protoObj0.method0.call(litObj0, arrObj0);
  while (prop0) {
    protoObj0.method1(arrObj0);
    obj0.method0(obj0);
    prop0 = IntArr0.shift();
  }
  obj0.method0('');
}
test0();
test0();
test0();
WScript.Echo('pass');
