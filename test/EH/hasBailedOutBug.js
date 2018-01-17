//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var shouldBailout = false;
function test0() {
  var obj0 = {};
  var func0 = function () {
  };
  var func1 = function () {
    (function () {
      'use strict';
      try {
        function func8() {
          obj0.prop2;
        }
        var uniqobj4 = func8();
      } catch (ex) {
        return 'somestring';
      } finally {
      }
      func0(ary.push(ary.unshift(Object.prototype.length = protoObj0)));
    }(shouldBailout ? (Object.defineProperty(Object.prototype, 'length', {
      get: function () {
      }
    })) : arguments));
  };
  var ary = Array();
  var protoObj0 = Object();
  ({
    prop7: shouldBailout ? (Object.defineProperty(obj0, 'prop2', {
      set: function () {
      }
    })) : Object
  });
  for (; func1(); ) {
  }
}
test0();
test0();
shouldBailout = true;
try {
  test0();
}
catch(ex) {
  print(ex);
}
