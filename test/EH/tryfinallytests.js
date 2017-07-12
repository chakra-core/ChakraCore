//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
  var ui32 = new Uint32Array();
  for (var _strvar23 in ui32) {
    if (typeof _strvar23) {
      continue;
    }
    try {
    } catch (ex) {
      continue;
    } finally {
    }
    break;
  }
}

function test1() {
  var arrObj0 = {};
  if (arrObj0.length) {
    for (var _strvar0 of IntArr0) {
      if (typeof _strvar0) {
        continue;
      }
      try {
      } catch (ex) {
        continue;
      } finally {
      }
      break;
    }
  }
}

function test2() {
  var obj1 = {};
  var ary = Array();
  var i8 = new Int8Array();
  if (!(-530320868 >= ((obj1.method1) & 255))) {
    for (var _strvar1 of i8) {
    }
  }
  ary | 0;
}

test0();
test0();
test0();

test1();
test1();
test1();

test2();
test2();
test2();

WScript.Echo("passed");
