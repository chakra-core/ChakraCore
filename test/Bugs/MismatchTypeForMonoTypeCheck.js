//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var loopInvariant = 10;
  var obj0 = {};
  var arrObj0 = {};
  var litObj1 = { prop0: 0 };
  function func3 () {
    do {
      if (9 >= loopInvariant) {
        break;
      }
      this.prop0 = typeof 197985440;
    } while (false);
  };
  var func4 = function () {
    while (func3() >>> parseInt(typeof Number)) {
    }
  };
  obj0.method0 = func3;
  arrObj0.method1 = func4;
  var protoObj0 = Object(obj0);
  if (!protoObj0.method0.call(litObj1)) {
    arrObj0.method1();
  }
}
test0();
test0();
WScript.Echo('pass');
