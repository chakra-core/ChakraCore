//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var obj0 = {};
  var func0 = function () {
    return protoObj0.prop1;
  };
  var func1 = function () {
    if (false) {
      for (var _strvar3 of ary) {
      }
    }
    return func0();
  };
  obj0.method0 = func1;
  obj0.method1 = obj0.method0;
  var protoObj0 = Object(obj0);
  protoObj0.method0();
  var uniqobj8 = [protoObj0];
  var uniqobj9 = uniqobj8[0];
  uniqobj9.method0();
  protoObj0.method1();
}
test0();

WScript.Echo("Passed");
