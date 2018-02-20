//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
function test0() {
  function leaf() {
    return 100;
  }
  var obj1 = {};
  var func0 = function () {
    return leaf();
  };
  var func2 = function () {
    if (!(func0.call() % -2147483646)) {
      arrObj0;
    }
  };
  obj1.method1 = func2;
  obj1.method1();
}
test0();
test0();
test0();
test0();
print('PASS'); //Test failure causes assertion
