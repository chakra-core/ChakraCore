//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var GiantPrintArray = [];
function makeArrayLength() {
}
var obj0 = {};
var obj1 = {};
var arrObj0 = {};
var func3 = function () {
  protoObj0._x = {};
  for (var v0 = 0; v0 < 3; v0++) {
    delete arrObj0.length;
    protoObj0.length = protoObj0._x;
  }
  GiantPrintArray.push(arrObj0.length);
};
obj0.method1 = func3;
obj1.method0 = obj0.method1;
obj1.method1 = obj1.method0;
arrObj0.length = makeArrayLength();
protoObj0 = arrObj0;
for (var _strvar13 in obj1) {
  obj0.method1();
}
var uniqobj3 = [obj1];
uniqobj3[0].method1();
WScript.Echo(GiantPrintArray);
