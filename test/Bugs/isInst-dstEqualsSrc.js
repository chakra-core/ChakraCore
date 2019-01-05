//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var func0 = function () {
  prop0 = func0;
};

var func4 = function () {
  while (func0()) {
  }
};

function func8() {
  prop0 = prop0 instanceof Object;
}

function v19() {
  do {
  } while (func8());
}
  
function test0() {
  var obj0 = {};
  
  obj0.method1 = func4;
  obj0.method1();
  
  v19();
  print(prop0);
}
test0();
test0();
test0()
