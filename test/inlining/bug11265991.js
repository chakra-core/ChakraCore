//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var shouldBailout = false;
function test0() {
  var GiantPrintArray = [];
  var obj1 = {};
  var func1 = function () {
    var v0 = true;
    var v1 = function v2() {
      if (v0) {
        v0 = false;
        v2();
      }
      protoObj0.prop0 = i16;
      test0;
    };
    v1();
    function v5(v6) {
      var v9 = {};
      v9.a = v6;
      v9.a[1] = null;
    }
    GiantPrintArray.push(v5(ary));
    return shouldBailout ? (Object.defineProperty(protoObj0, 'prop0', {
      set: function () {
      }
    })) : Error();
  };
  var func2 = function () {
    for (var _strvar4 of ary) {
      Math.tan((func1()));
    }
  };
  var func3 = function () {
    func2(func2(func1()));
    func1();
  };
  obj1.method1 = func1;
  var ary = Array();
  var i16 = new Int16Array();
  var protoObj0 = Object();
  if (!(obj1.method1() + (func3()))) {
  }
  func1();
}
test0();
shouldBailout = true;
test0();
print("Passed");