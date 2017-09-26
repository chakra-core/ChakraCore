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

function test3() {
  var IntArr0 = new Array();
  var VarArr0 = [];
  try {
  } catch (ex) {
  } finally {
    do {
      try {
        VarArr0.reverse();
      } catch (ex) {
        continue;
      } finally {
      }
    } while (IntArr0[5]);
  }
}

function test5() {
  for (let rwcjvd = 0, ljcyer = /x/g; rwcjvd; rwcjvd) {
    {
      if (/x/) {
        try {
        } catch (e) {
        }
      } else {
        throw "err" ;
        while (new Array.isArray(/x/g, 'u56DC') && 0) {
        }
      }
    }
  }
}

function test4() {
  var obj0 = {};
  var obj1 = {};
  var arrObj0 = {};
  var func0 = function () {
  };
  var func2 = function () {
  };
  obj0.method1 = func0;
  obj1.method1 = func2;
  var ary = Array();
  var protoObj1 = Object(obj1);
  while (typeof 11) {
    protoObj1.method1(obj0.method1(ary.unshift((Object.defineProperty(obj1, 'prop1', {})))));
    try {
    } catch (ex) {
      continue;
    } finally {
      obj1.prop1 = typeof arrObj0.length;
      break;
    }
  }
}

function test6() {
  var litObj0 = {};
  for (; 87587180 < typeof ('prop0' in litObj0); arrObj0()) {
    try {
      continue;
    }
    finally {
      var v1 = IntArr0();
      while (1) {
	WScript.Echo("help");
	if (num > 10) {
	WScript.Echo("help2");
	}
      }
    }
  }
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

test3();
test3();
test3();

test4();
test4();
test4();

test5();
test5();
test5();

test6();
test6();
test6();

WScript.Echo("passed");
